/*! Print control plots that were selected to synchronize produced tree-toople.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <set>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <memory>
#include <boost/filesystem.hpp>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TText.h>
#include <TLine.h>
#include <TPad.h>
#include <Rtypes.h>
#include <TError.h>

#include "RootExt.h"
#include "EventIdentifier.h"

#include "AnalysisTools/Run/include/program_main.h"
#include "AnalysisTools/Instruments/include/SyncPlotsConfig.h"

struct Arguments {
    REQ_ARG(std::string, config);
    REQ_ARG(std::string, channel);
    REQ_ARG(std::string, sample);
    REQ_ARG(std::vector<std::string>, group);
    REQ_ARG(std::vector<std::string>, file);
    REQ_ARG(std::vector<std::string>, tree);
    OPT_ARG(std::vector<std::string>, preSelection, std::vector<std::string>());
    OPT_ARG(double, badThreshold, 0.01);
};

namespace {
template<typename T, bool is_integral = std::is_integral<T>::value>
struct BadEventCheck {
    static bool isBadEvent(T first, T second, double badThreshold)
    {
        if(first == second) return false;
        if(first == 0 || second == 0) return true;
        const auto delta = first - second;
        return std::abs<T>(delta/second) >= badThreshold;
    }
};

template<typename T>
struct BadEventCheck<T, true> {
    static bool isBadEvent(T first, T second, double /*badThreshold*/)
    {
        return first != second;
    }
};
}

namespace analysis {
class EventSync {
public:
    static constexpr size_t N = 2;

    using EventSet = std::set<EventIdentifier> ;
    using EventVector = std::vector<EventIdentifier>;
    using EventToEntryMap = std::map<EventIdentifier, size_t>;
    using EntryPair = std::pair<size_t, size_t>;
    using EventToEntryPairMap = std::map<EventIdentifier, EntryPair>;
    using SelectorFn = std::function<bool(size_t)>;
    using SelectorFnArray = std::array<SelectorFn, N>;
    using Hist = TH1F;
    using HistPtr = std::shared_ptr<Hist>;
    using Hist2D = TH2F;
    using Hist2DPtr = std::shared_ptr<Hist2D>;

    EventSync(const Arguments& _args) :
        args(_args), config(args.config()), channel(args.channel()), sample(args.sample()),
        isFirstPage(true), isLastDraw(false)
    {
        if(args.group().size() != N || args.file().size() != N || args.tree().size() != N
                || args.preSelection().size() > N )
            throw exception("Invalid number of arguments");

        std::cout << channel << " " << sample << std::endl;
        for(size_t n = 0; n < N; ++n) {
            groups[n] = args.group().at(n);
            rootFileNames[n] = args.file().at(n);
            treeNames[n] = args.tree().at(n);
            if(args.preSelection().size() > n)
                preSelections[n] = args.preSelection().at(n);
            std::cout << groups[n] << "  " << rootFileNames[n] << "  " << treeNames[n] << std::endl;
            trees[n] = LoadTree(rootFiles[n], rootFileNames[n], treeNames[n], preSelections[n]);
        }

        file_name = "PlotsDiff_" + channel + "_" + sample + "_" + groups[0] + "_" + groups[1] + ".pdf";
        gErrorIgnoreLevel = kWarning;
    }

    ~EventSync()
    {
        for(size_t n = 0; n < N; ++n)
            trees[n].reset();
        tmpRootFile.reset();
        boost::filesystem::remove(tmpName);
    }

    void Run()
    {
        CollectEvents();

        for(size_t k = 0; k < config.GetEntries().size(); ++k) {
            const SyncPlotEntry& entry = config.GetEntries().at(k);
            isLastDraw = k + 1 == config.GetEntries().size();

            std::array<std::vector<int>, N> vars_int;
            std::array<std::vector<double>, N> vars_double;
            std::string selection_label;
            for(size_t n = 0; n < N; ++n) {
                if(!entry.conditions[n].always_true) {
                    if(entry.conditions[n].is_integer)
                        vars_int[n] = CollectValuesEx<int>(*trees[n], entry.conditions[n].entry);
                    else
                        vars_double[n] = CollectValuesEx<double>(*trees[n], entry.conditions[n].entry);
                    if(!selection_label.size())
                        selection_label = ToString(entry.conditions[n]);
                }
            }

            const auto selector = [&](size_t groupId, size_t n) -> bool {
                if(entry.conditions[groupId].always_true) return true;
                if(entry.conditions[groupId].is_integer)
                    return entry.conditions[groupId].pass_int(vars_int[groupId].at(n));
                return entry.conditions[groupId].pass_double(vars_double[groupId].at(n));
            };

            SelectorFnArray selectors;
            selectors[0] = std::bind(selector, 0, std::placeholders::_1);
            selectors[1] = std::bind(selector, 1, std::placeholders::_1);

            drawHistos(entry.names, entry.n_bins, entry.x_range.min(), entry.x_range.max(), selectors, selection_label);
        }
    }

private:
    std::shared_ptr<TTree> LoadTree(std::shared_ptr<TFile>& file, const std::string& fileName,
                           const std::string& treeName, const std::string& preSelection)
    {
        file = std::shared_ptr<TFile>(new TFile(fileName.c_str(), "READ"));
        std::shared_ptr<TTree> tree(root_ext::ReadObject<TTree>(*file, treeName));
        if(!tree)
            throw exception("File '%1%' is empty.") % fileName;
        if(!tmpRootFile) {
            tmpName = boost::filesystem::unique_path("%%%%-%%%%.root").native();
            tmpRootFile = std::shared_ptr<TFile>(new TFile(tmpName.c_str(), "RECREATE"));
        }
        tmpRootFile->cd();
        tree = std::shared_ptr<TTree>(tree->CopyTree(preSelection.c_str()));
        tree->SetBranchStatus("*", 0);
        return tree;
    }

    void drawHistos(const std::array<std::string, N>& var_names, size_t nbins, double xmin, double xmax,
                    const SelectorFnArray& selectors, const std::string& selection_label)
    {
        try {

            using DataTypePair = std::pair<EDataType, EDataType>;
            using FillMethodPtr = void (EventSync::*)(const std::array<std::string, N>&, const SelectorFnArray&,
                                  std::array<HistPtr, N>&, std::array<HistPtr, N>&, std::array<HistPtr, N>&, Hist2D&);
            using FillMethodMap = std::map<DataTypePair, FillMethodPtr>;

            const auto createHist = [&](const std::string& name) -> HistPtr {
                return HistPtr(new Hist(name.c_str(), "", static_cast<int>(nbins), xmin, xmax));
            };
            const auto createHist0 = [&](const std::string& suffix) -> HistPtr {
                return createHist("H" + groups[0] + var_names[0] + suffix);
            };
            const auto createHist1 = [&](const std::string& suffix) -> HistPtr {
                return createHist("H" + groups[1] + var_names[0] + suffix);
            };
            const auto create2DHist = [&](const std::string& name) -> Hist2DPtr {
                return Hist2DPtr(new Hist2D(name.c_str(), "", static_cast<int>(nbins), xmin, xmax, 61, -1.525, 1.525));
            };

            std::array<HistPtr, N> H_all = { createHist0("all"), createHist1("all") };
            std::array<HistPtr, N> H_common = { createHist0("common"), createHist1("common") };
            std::array<HistPtr, N> H_diff = { createHist0("diff"), createHist1("diff") };
            auto H_0vs1 = create2DHist("H" + groups[0] + "_vs_" + groups[1] + var_names[0]);

            std::array<EDataType, N> data_types;
            for(size_t n = 0; n < N; ++n) {
                auto branch = trees[n]->GetBranch(var_names[n].c_str());
                if (!branch)
                    throw exception("%1% Branch '%2%' is not found.") % groups[n] % var_names[n];
                TClass* t_class;
                branch->GetExpectedType(t_class, data_types[n]);
                if(t_class)
                    throw exception("Branches with complex objects are not supported for branch '%1%'.") % var_names[n];
            }

            static const FillMethodMap fillMethods = {
                { { kFloat_t, kFloat_t }, &EventSync::FillAllHistograms<Float_t, Float_t> },
                { { kDouble_t, kDouble_t }, &EventSync::FillAllHistograms<Double_t, Double_t> },
                { { kDouble_t, kFloat_t }, &EventSync::FillAllHistograms<Double_t, Float_t> },
                { { kFloat_t, kDouble_t }, &EventSync::FillAllHistograms<Float_t, Double_t> },
                { { kFloat_t, kUInt_t }, &EventSync::FillAllHistograms<Float_t, UInt_t> },
                { { kInt_t, kInt_t }, &EventSync::FillAllHistograms<Int_t, Int_t> },
                { { kInt_t, kUInt_t }, &EventSync::FillAllHistograms<Int_t, UInt_t> },
                { { kUInt_t, kInt_t }, &EventSync::FillAllHistograms<UInt_t, Int_t> },
                { { kInt_t, kDouble_t }, &EventSync::FillAllHistograms<Int_t, Double_t> },
                { { kDouble_t, kInt_t }, &EventSync::FillAllHistograms<Double_t, Int_t> },
                { { kULong64_t, kULong64_t }, &EventSync::FillAllHistograms<ULong64_t, ULong64_t> },
                { { kBool_t, kBool_t }, &EventSync::FillAllHistograms<Bool_t, Bool_t> },
                { { kBool_t, kChar_t }, &EventSync::FillAllHistograms<Bool_t, Char_t> },
                { { kBool_t, kInt_t }, &EventSync::FillAllHistograms<Bool_t, Int_t> },
            };

            const auto typePair = DataTypePair(data_types[0], data_types[1]);
            if(!fillMethods.count(typePair))
                throw exception("Unknown branch type combination (%1%, %2%) for branch ('%3%', '%4%').")
                    % data_types[0] % data_types[1] % var_names[0] % var_names[1];
            FillMethodPtr fillMethod = fillMethods.at(typePair);
            (this->*fillMethod)(var_names, selectors, H_all, H_common, H_diff, *H_0vs1);

            DrawSuperimposedHistograms(H_all, selection_label, var_names, "all");
            DrawSuperimposedHistograms(H_common, selection_label, var_names, "common");
            DrawSuperimposedHistograms(H_diff, selection_label, var_names, "different");
            Draw2DHistogram(H_0vs1, selection_label, var_names);

        } catch(std::runtime_error& e){
            std::cerr << "WARNING: " << e.what() << std::endl;
        }
    }

    static std::string MakeTitle(const std::array<std::string, N>& var_names,
                                 const std::string& event_subset, const std::string& selection_label)
    {
        std::ostringstream ss_title;
        ss_title << var_names[0];
        if(var_names[1] != var_names[0])
            ss_title << " (" << var_names[1] << ")";
        ss_title << ", " << event_subset << " events";
        if(selection_label.size())
            ss_title << ", pre-selection: " << selection_label;
        return ss_title.str();
    }

    void DrawSuperimposedHistograms(const std::array<HistPtr, N>& hists, const std::string& selection_label,
                                    const std::array<std::string, N>& var_names, const std::string& event_subset)
    {
        const std::string title = MakeTitle(var_names, event_subset, selection_label);
        hists[0]->SetTitle(title.c_str());
        hists[0]->GetYaxis()->SetTitle("Events");
        hists[0]->GetXaxis()->SetTitle(var_names[0].c_str());
        hists[0]->SetLineColor(1);
        hists[0]->SetMarkerColor(1);
        hists[0]->SetStats(0);

        hists[1]->SetLineColor(2);
        hists[1]->SetMarkerColor(2);
        hists[1]->SetStats(0);

        TPad pad1("pad1", "", 0, 0.2, 1, 1);
        TPad pad2("pad2", "", 0, 0, 1, 0.2);

        pad1.Draw();
        pad2.Draw();

        pad1.cd();

        const double max = std::max(hists[0]->GetMaximum(), hists[1]->GetMaximum());
        hists[0]->GetYaxis()->SetRangeUser(0, max * 1.1);
        hists[0]->Draw("hist");
        hists[1]->Draw("histsame");
        DrawTextLabels({static_cast<size_t>(hists[0]->Integral(0, hists[0]->GetNbinsX() + 1)),
                       static_cast<size_t>(hists[1]->Integral(0, hists[1]->GetNbinsX() + 1))});

        pad2.cd();

        // Draw the ratio of the historgrams
        HistPtr HDiff(dynamic_cast<Hist*>(hists[1]->Clone("HDiff")));
        HDiff->Divide(hists[0].get());
        HDiff->GetYaxis()->SetRangeUser(0.9,1.1);
        HDiff->GetYaxis()->SetNdivisions(3);
        HDiff->GetYaxis()->SetLabelSize(0.1f);
        HDiff->GetYaxis()->SetTitleSize(0.1f);
        HDiff->GetYaxis()->SetTitleOffset(0.5);
        HDiff->GetYaxis()->SetTitle("Ratio");
        HDiff->GetXaxis()->SetNdivisions(-1);
        HDiff->GetXaxis()->SetTitle("");
        HDiff->GetXaxis()->SetLabelSize(0.0001f);
        HDiff->SetMarkerStyle(7);
        HDiff->SetMarkerColor(2);
        HDiff->Draw("histp");

        TLine line;
        line.DrawLine(HDiff->GetXaxis()->GetXmin(), 1, HDiff->GetXaxis()->GetXmax(), 1);
        PrintCanvas(title);
        pad1.Clear();
        pad2.Clear();
        canvas.Clear();
    }

    void Draw2DHistogram(Hist2DPtr H_0vs1, const std::string& selection_label,
                         const std::array<std::string, N>& var_names)
    {
        const std::string title = MakeTitle(var_names, "common", selection_label);
        H_0vs1->SetTitle(title.c_str());
        std::array<std::string, N> names;
        for(size_t n = 0; n < N; ++n)
            names[n] = var_names[n] + "_" + groups[n];
        std::ostringstream y_name;
        y_name << "(" << names[1] << " - " << names[0] << ")/" << names[1];
        H_0vs1->GetXaxis()->SetTitle(names[1].c_str());
        H_0vs1->GetYaxis()->SetTitle(y_name.str().c_str());
        H_0vs1->SetStats(0);

        TPad pad1("pad1","", 0, 0, 1, 1);
        pad1.Draw();
        pad1.cd();
        H_0vs1->Draw("colz");
        const size_t n_events = static_cast<size_t>(H_0vs1->Integral(0, H_0vs1->GetNbinsX() + 1,
                                                                     0, H_0vs1->GetNbinsY() + 1));
        DrawTextLabels({n_events, n_events});

        PrintCanvas(title + " 2D", isLastDraw);
        pad1.Clear();
        canvas.Clear();
    }

    void DrawTextLabels(std::array<size_t, N> n_events)
    {
        std::array<Color_t, N> colors = { 1, 2 };
        std::array<double, N> x_pos = { .23, .53 };
        for(size_t n = 0; n < N; ++n) {
            TText text;
            text.SetTextColor(colors[n]);
            text.SetTextSize(.04f);
            text.DrawTextNDC(x_pos[n], .84, (groups[n] + " : " + ToString(n_events[n])).c_str());
        }
    }

    template<typename VarType0, typename VarType1>
    void FillAllHistograms(const std::array<std::string, N>& var_names, const SelectorFnArray& selectors,
                           std::array<HistPtr, N>& H_all, std::array<HistPtr, N>& H_common,
                           std::array<HistPtr, N>& H_diff, Hist2D& H_0vs1)
    {
        const std::vector<VarType0> values0 = CollectValues<VarType0>(*trees[0], var_names[0]);
        const std::vector<VarType1> values1 = CollectValues<VarType1>(*trees[1], var_names[1]);
        FillCommonHistograms(var_names[0], values0, values1, selectors, H_common, H_0vs1);
        FillInclusiveHistogram(values0, selectors[0], *H_all[0]);
        FillInclusiveHistogram(values1, selectors[1], *H_all[1]);
        FillExclusiveHistogram(values0, values1, selectors, H_diff);
    }

    template<typename VarType0, typename VarType1>
    void FillCommonHistograms(const std::string& var, const std::vector<VarType0>& values0,
                              const std::vector<VarType1>& values1, const SelectorFnArray& selectors,
                              std::array<HistPtr, N>& H_common, Hist2D& hist2D)
    {
        std::cout << var << " bad events:\n";
        for(const auto& event_entry_pair : common_event_to_entry_pair_map) {
            const size_t entry0 = event_entry_pair.second.first;
            const size_t entry1 = event_entry_pair.second.second;
            if(!selectors[0](entry0) || !selectors[1](entry1))
                continue;
            const VarType0& value0 = values0.at(entry0);
            const VarType1& value1 = values1.at(entry1);
            H_common[0]->Fill(value0);
            H_common[1]->Fill(value1);
            const double y_value = value1 != VarType1(0)
                                 ? double(static_cast<VarType0>(value1) - value0) / value1 : -value0;
            const auto diff = static_cast<VarType0>(value1) - value0;
            if (BadEventCheck<VarType0>::isBadEvent(value0, static_cast<VarType0>(value1), args.badThreshold())) {
                std::cout << event_entry_pair.first.GetLegendString() << " = " << event_entry_pair.first << ", "
                          << groups[1] << " = " << value1 << ", " << groups[0] << " = " << value0 << ", "
                          << groups[1] <<  " - " << groups[0] << " = " << diff << std::endl;
            }
            hist2D.Fill(value1, y_value);
        }
    }

    template<typename VarType>
    void FillInclusiveHistogram(const std::vector<VarType>& values, const SelectorFn& selector, Hist& histogram)
    {
        for(size_t n = 0; n < values.size(); ++n) {
            if(selector(n))
                histogram.Fill(values.at(n));
        }
    }

    template<typename VarType0, typename VarType1>
    void FillExclusiveHistogram(const std::vector<VarType0>& values0, const std::vector<VarType1>& values1,
                                const SelectorFnArray& selectors, std::array<HistPtr, N>& H_diff)
    {
        for (const auto& event_entry_pair : common_event_to_entry_pair_map){
            const size_t entry0 = event_entry_pair.second.first;
            const size_t entry1 = event_entry_pair.second.second;
            if(selectors[0](entry0) && !selectors[1](entry1)){
                const auto& value0 = values0.at(entry0);
                H_diff[0]->Fill(value0);
            }
            if(!selectors[0](entry0) && selectors[1](entry1)){
                const auto& value1 = values1.at(entry1);
                H_diff[1]->Fill(value1);
            }
        }
        for(const auto& event_entry : group_events_only_map[0]) {
            if(selectors[0](event_entry.second)) {
                const auto& value = values0.at(event_entry.second);
                H_diff[0]->Fill(value);
            }
        }
        for(const auto& event_entry : group_events_only_map[1]) {
            if(selectors[1](event_entry.second)) {
                const auto& value = values1.at(event_entry.second);
                H_diff[1]->Fill(value);
            }
        }
    }

    void CollectEvents()
    {
        std::array<EventSet, N> event_sets, group_events_only;
        for(size_t n = 0; n < N; ++n) {
            events[n] = CollectEventIds(*trees[n], config.GetIdBranches(n));
            event_sets[n] = EventSet(events[n].begin(), events[n].end());
        }

        EventSet intersection;
        for(size_t n = 0; n < N; ++n)
            EventSetDifference(event_sets[n % N], event_sets[(n+1) % N], group_events_only[n % N]);
        EventSetIntersection(event_sets[0], event_sets[1], intersection);

        std::array<EventToEntryMap, N> event_to_entry_map;
        for(size_t n = 0; n < N; ++n) {
            FillEventToEntryMap(events[n], event_to_entry_map[n]);
            std::cout << "# " << groups[n] << " events = " << events[n].size() << ", " << "# " << groups[n]
                      << " unique events = " << event_sets[n].size() << std::endl;
            ReportDuplicatedEvents(events[n], event_sets[n], groups[n]);
        }
        std::cout << "# common events = " << intersection.size() << std::endl;


        std::cout << groups[0] << " events" << std::endl;
        for(const auto& event_entry : event_to_entry_map[0]) {
            if(intersection.count(event_entry.first)) {
                common_event_to_entry_pair_map[event_entry.first] =
                        std::pair<size_t, size_t>(event_entry.second, event_to_entry_map[1].at(event_entry.first));
            }
            if(group_events_only[0].count(event_entry.first)) {
                group_events_only_map[0][event_entry.first] = event_entry.second;
                std::cout << event_entry.first.GetLegendString() << " = " << event_entry.first << std::endl;
            }
        }

        std::cout << groups[1] << " events" << std::endl;
        for(const auto& event_entry : event_to_entry_map[1]) {
            if(group_events_only[1].count(event_entry.first)){
                group_events_only_map[1][event_entry.first] = event_entry.second;
                std::cout << event_entry.first.GetLegendString() << " = " << event_entry.first << std::endl;
            }
        }
    }

    static void EventSetIntersection(const EventSet& first_set, const EventSet& second_set, EventSet& intersection_set)
    {
        const size_t max_intersection_size = std::max(first_set.size(), second_set.size());
        EventVector intersection_vector(max_intersection_size);
        const auto iter = std::set_intersection(first_set.begin(), first_set.end(),
                                                second_set.begin(), second_set.end(),
                                                intersection_vector.begin());
        intersection_vector.resize(static_cast<size_t>(iter - intersection_vector.begin()));
        intersection_set.clear();
        intersection_set.insert(intersection_vector.begin(), intersection_vector.end());
    }

    static void EventSetDifference(const EventSet& first_set, const EventSet& second_set, EventSet& diff_set)
    {
        EventVector diff_vector(first_set.size());
        const auto iter = std::set_difference(first_set.begin(), first_set.end(),
                                              second_set.begin(), second_set.end(),
                                              diff_vector.begin());
        diff_vector.resize(static_cast<size_t>(iter - diff_vector.begin()));
        diff_set.clear();
        diff_set.insert(diff_vector.begin(), diff_vector.end());
    }

    static void ReportDuplicatedEvents(const EventVector& event_vec, const EventSet& event_set,
                                       const std::string& name)
    {
        if(event_vec.size() == event_set.size()) return;
        EventSet processed_events;
        std::cout << name << " duplicated events:\n";
        for(const auto& evt : event_vec) {
            if(processed_events.count(evt))
                std::cout << evt << "\n";
            else
                processed_events.insert(evt);
        }
        std::cout << std::endl;
    }

    void FillEventToEntryMap(const EventVector& events, EventToEntryMap& event_to_entry_map) const
    {
        for(size_t n = 0; n < events.size(); ++n) {
             event_to_entry_map[events[n]] = n;
        }
    }

    template<typename VarType>
    std::vector<VarType> CollectValues(TTree& tree, const std::string& name)
    {
        EnableBranch(tree, name, true);
        std::vector<VarType> result;
        VarType value;
        tree.SetBranchAddress(name.c_str(), &value);
        const Long64_t N = tree.GetEntries();
        for(Long64_t n = 0; n < N;++n) {
            if(tree.GetEntry(n) < 0)
                throw exception("error while reading tree.");
            result.push_back(value);
        }
        EnableBranch(tree, name, false);
        return result;
    }

    template<typename VarType>
    std::vector<VarType> CollectValuesEx(TTree& tree, const std::string& name)
    {
        std::vector<VarType> result;

        using CollectMethodPtr = void (EventSync::*)(TTree&, const std::string&, std::vector<VarType>&);
        using CollectMethodMap = std::map<EDataType, CollectMethodPtr>;

        static const CollectMethodMap collectMethods = {
            { kInt_t, &EventSync::CollectAndConvertValue<Int_t, VarType> },
            { kUInt_t, &EventSync::CollectAndConvertValue<UInt_t, VarType> },
            { kULong64_t, &EventSync::CollectAndConvertValue<ULong64_t, VarType> },
            { kFloat_t, &EventSync::CollectAndConvertValue<Float_t, VarType> },
            { kDouble_t, &EventSync::CollectAndConvertValue<Double_t, VarType> },
            { kChar_t, &EventSync::CollectAndConvertValue<Char_t, VarType> },
            { kBool_t, &EventSync::CollectAndConvertValue<Bool_t, VarType> }
        };

        TBranch* branch = tree.GetBranch(name.c_str());
        if (!branch)
            throw exception("Branch '%1%' not found.") % name;
        TClass *branch_class;
        EDataType branch_type;
        branch->GetExpectedType(branch_class, branch_type);
        if(branch_class)
            throw exception("Branches with complex objects are not supported for branch '%1%'.") % name;

        if(!collectMethods.count(branch_type))
            throw exception("Branch '%1%' has unsupported type %2%.") % name % branch_type;

        auto collectMethod = collectMethods.at(branch_type);
        (this->*collectMethod)(tree, name, result);
        return result;
    }

    template<typename InputType, typename OutputType>
    void CollectAndConvertValue(TTree& tree, const std::string& name, std::vector<OutputType>& result)
    {
        const auto original_result = CollectValues<InputType>(tree, name);
        result.resize(original_result.size());
        std::copy(original_result.begin(), original_result.end(), result.begin());
    }

    EventVector CollectEventIds(TTree& tree, const std::vector<std::string>& idBranches)
    {
        using IdType = EventIdentifier::IdType;
        const auto run = CollectValuesEx<IdType>(tree, idBranches.at(0));
        const auto lumi = CollectValuesEx<IdType>(tree, idBranches.at(1));
        const auto evt = CollectValuesEx<IdType>(tree, idBranches.at(2));
        std::vector<IdType> sampleIds;
        if(idBranches.size() > 3)
            sampleIds = CollectValuesEx<IdType>(tree, idBranches.at(3));

        EventVector events;
        for(size_t n = 0; n < evt.size(); ++n) {
            const IdType sampleId = sampleIds.size() ? sampleIds.at(n) : EventIdentifier::Undef_id;
            events.emplace_back(run.at(n), lumi.at(n), evt.at(n), sampleId);
        }
        return events;
    }

    void EnableBranch(TTree& tree, const std::string& name, bool enable)
    {
        UInt_t n_found = 0;
        tree.SetBranchStatus(name.c_str(), enable, &n_found);
        if(n_found != 1)
            throw exception("Branch '%1%' is not found.") % name;
    }

    void PrintCanvas(const std::string& page_name, bool isLastPage = false)
    {
        std::ostringstream print_options, output_name;
        print_options << "Title:" << page_name;
        output_name << file_name;
        if(isFirstPage && !isLastPage)
            output_name << "(";
        else if(isLastPage && !isFirstPage)
            output_name << ")";
        isFirstPage = false;
        canvas.Print(output_name.str().c_str(), print_options.str().c_str());

    }

private:
    Arguments args;
    SyncPlotConfig config;
    std::string channel, sample;
    std::array<std::string, N> groups, rootFileNames, treeNames, preSelections;
    std::array<std::shared_ptr<TFile>, N> rootFiles;
    std::array<std::shared_ptr<TTree>, N> trees;

    std::string tmpName;
    std::shared_ptr<TFile> tmpRootFile;

    std::array<EventVector, N> events;
    std::array<EventToEntryMap, N> group_events_only_map;
    EventToEntryPairMap common_event_to_entry_pair_map;

    TCanvas canvas;
    std::string file_name;
    bool isFirstPage, isLastDraw;
};

} // namespace analysis

PROGRAM_MAIN(analysis::EventSync, Arguments)

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
#include <boost/filesystem.hpp>

#include "AnalysisTools/Run/include/program_main.h"
#include "../include/SyncPlotsConfig.h"

struct Arguments {
    REQ_ARG(std::string, configFile);
    REQ_ARG(std::string, channelName);
    REQ_ARG(std::string, sampleName);
    REQ_ARG(std::string, myGroup);
    REQ_ARG(std::string, myRootFile);
    REQ_ARG(std::string, myTTreeName);
    REQ_ARG(std::string, group);
    REQ_ARG(std::string, groupRootFile);
    REQ_ARG(std::string, groupTTreeName);
    OPT_ARG(std::string, myPreSelection, "");
    OPT_ARG(std::string, groupPreSelection, "");
};

namespace analysis {
class Print_SyncPlots {
public:
    using EventSet = std::set<EventIdentifier> ;
    using EventVector = std::vector<EventIdentifier>;
    using EventToEntryMap = std::map<EventIdentifier, size_t>;
    using EntryPair = std::pair<size_t, size_t>;
    using EventToEntryPairMap = std::map<EventIdentifier, EntryPair>;

    Print_SyncPlots(const Arguments& args)
        : config(args.configFile()), channel(args.channelName()), sample(args.sampleName()), myGroup(args.myGroup()),
          myRootFile(args.myRootFile()), myTree(args.myTTreeName()),
          group(args.group()), groupRootFile(args.groupRootFile()), groupTree(args.groupTTreeName()),
          isFirstPage(true), isLastDraw(false)
    {
        std::cout << channel << " " << sample << std::endl;
        std::cout << myGroup << "  " << myRootFile << "  " << myTree << std::endl;
        std::cout << group << "  " << groupRootFile << "  " << groupTree << std::endl;

        Tmine = LoadTree(Fmine, myRootFile, myTree, args.myPreSelection());
        Tother = LoadTree(Fother, groupRootFile, groupTree, args.groupPreSelection());
        file_name = "PlotsDiff_" + channel + "_" + sample + "_" + myGroup + "_" + group + ".pdf";
        gErrorIgnoreLevel = kWarning;
    }

    ~Print_SyncPlots()
    {
        Tmine = std::shared_ptr<TTree>();
        Tother = std::shared_ptr<TTree>();
        Ftmp = std::shared_ptr<TFile>();
        boost::filesystem::remove(tmpName);
    }

    void Run()
    {
        CollectEvents(*Tmine, *Tother);

        for(size_t n = 0; n < config.GetEntries().size(); ++n) {
            const SyncPlotEntry& entry = config.GetEntries().at(n);
            isLastDraw = n + 1 == config.GetEntries().size();

            if(entry.my_condition.always_true && entry.other_condition.always_true) {
                drawHistos(entry.my_name, entry.other_name, entry.n_bins, entry.x_range.min(), entry.x_range.max());
                continue;
            }

            std::vector<int> my_vars_int, other_vars_int;
            std::vector<double> my_vars_double, other_vars_double;
            if(!entry.my_condition.always_true) {
                if(entry.my_condition.is_integer)
                    my_vars_int = CollectValuesEx<int>(*Tmine, entry.my_condition.entry);
                else
                    my_vars_double = CollectValuesEx<double>(*Tmine, entry.my_condition.entry);
            }
            if(!entry.other_condition.always_true) {
                if(entry.other_condition.is_integer)
                    other_vars_int = CollectValuesEx<int>(*Tother, entry.other_condition.entry);
                else
                    other_vars_double = CollectValuesEx<double>(*Tother, entry.other_condition.entry);
            }

            std::ostringstream selection_label;
            if(!entry.my_condition.always_true)
                selection_label << entry.my_condition;
            else
                selection_label << entry.other_condition;

            const auto my_selector = [&](size_t n) -> bool {
                if(entry.my_condition.always_true) return true;
                if(entry.my_condition.is_integer) return entry.my_condition.pass_int(my_vars_int.at(n));
                return entry.my_condition.pass_double(my_vars_double.at(n));
            };
            const auto other_selector = [&](size_t n) -> bool {
                if(entry.other_condition.always_true) return true;
                if(entry.other_condition.is_integer) return entry.other_condition.pass_int(other_vars_int.at(n));
                return entry.other_condition.pass_double(other_vars_double.at(n));
            };

            drawHistos(entry.my_name, entry.other_name, entry.n_bins, entry.x_range.min(), entry.x_range.max(),
                       my_selector, other_selector, selection_label.str());
        }
    }

private:
    std::shared_ptr<TTree> LoadTree(std::shared_ptr<TFile>& file, const std::string& fileName,
                           const std::string& treeName, const std::string& preSelection)
    {
        file = std::shared_ptr<TFile>(new TFile(fileName.c_str(), "READ"));
        std::shared_ptr<TTree> tree((TTree*)file->Get(treeName.c_str()));
        if(!tree)
            throw exception("File '%1%' is empty.") % fileName;
        if(!Ftmp) {
            tmpName = boost::filesystem::unique_path("%%%%-%%%%.root").native();
            Ftmp = std::shared_ptr<TFile>(new TFile(tmpName.c_str(), "RECREATE"));
        }
        Ftmp->cd();
        tree = std::shared_ptr<TTree>(tree->CopyTree(preSelection.c_str()));
        tree->SetBranchStatus("*", 0);
        return tree;
    }

    void drawHistos(const std::string& mine_var, const std::string& other_var, int nbins, double xmin, double xmax)
    {
        const auto SelectAll = [](size_t) -> bool { return true; };
        drawHistos(mine_var, other_var, nbins, xmin, xmax, SelectAll, SelectAll, "");
    }

    template<typename MySelector, typename OtherSelector>
    void drawHistos(const std::string& mine_var, const std::string& other_var, int nbins, double xmin, double xmax,
                    const MySelector& my_selector, const OtherSelector& other_selector,
                    const std::string& selection_label)
    {
        try {

            using Hist = TH1F;
            using HistPtr = std::shared_ptr<Hist>;
            using Hist2D = TH2F;
            using Hist2DPtr = std::shared_ptr<Hist2D>;
            using DataTypePair = std::pair<EDataType, EDataType>;
            using FillMethodPtr = void (Print_SyncPlots::*)(const std::string&, const std::string&, const MySelector&,
                                  const OtherSelector&, Hist&, Hist&, Hist&, Hist&, Hist2D&, Hist&, Hist&);
            using FillMethodMap = std::map<DataTypePair, FillMethodPtr>;

            const auto createHist = [&](const std::string& name) -> HistPtr {
                return HistPtr(new Hist(name.c_str(), "", nbins, xmin, xmax));
            };
            const auto createMineHist = [&](const std::string& suffix) -> HistPtr {
                return createHist("Hmine" + mine_var + suffix);
            };
            const auto createOtherHist = [&](const std::string& suffix) -> HistPtr {
                return createHist("Hother" + mine_var + suffix);
            };
            const auto create2DHist = [&](const std::string& name) -> Hist2DPtr {
                return Hist2DPtr(new Hist2D(name.c_str(), "", nbins, xmin, xmax, 61, -1.525, 1.525));
            };

            auto Hmine_all = createMineHist("all");
            auto Hother_all = createOtherHist("all");
            auto Hmine_common = createMineHist("common");
            auto Hother_common = createOtherHist("common");
            auto Hmine_diff = createMineHist("diff");
            auto Hother_diff = createOtherHist("diff");
            auto Hmine_vs_other = create2DHist("Hmine_vs_other" + mine_var);

            TBranch* myBranch = Tmine->GetBranch(mine_var.c_str());
            if (!myBranch)
                throw exception("My Branch '%1%' is not found.") % mine_var;
            TBranch* otherBranch = Tother->GetBranch(other_var.c_str());
            if (!otherBranch)
                throw exception("Other Branch '%1%' is not found.") % other_var;
            TClass *myClass, *otherClass;
            EDataType myType, otherType;
            myBranch->GetExpectedType(myClass, myType);
            otherBranch->GetExpectedType(otherClass, otherType);
            if(myClass || otherClass)
                throw exception("Branches with complex objects are not supported for branch '%1%' (%2%).")
                    % mine_var % other_var;

            static const FillMethodMap fillMethods = {
                { { kFloat_t, kFloat_t },
                  &Print_SyncPlots::FillAllHistograms<Float_t, Float_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kDouble_t, kDouble_t },
                  &Print_SyncPlots::FillAllHistograms<Double_t, Double_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kDouble_t, kFloat_t },
                  &Print_SyncPlots::FillAllHistograms<Double_t, Float_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kFloat_t, kDouble_t },
                  &Print_SyncPlots::FillAllHistograms<Float_t, Double_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kFloat_t, kUInt_t },
                  &Print_SyncPlots::FillAllHistograms<Float_t, UInt_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kInt_t, kInt_t },
                  &Print_SyncPlots::FillAllHistograms<Int_t, Int_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kInt_t, kDouble_t },
                  &Print_SyncPlots::FillAllHistograms<Int_t, Double_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kDouble_t, kInt_t },
                  &Print_SyncPlots::FillAllHistograms<Double_t, Int_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kULong64_t, kULong64_t },
                  &Print_SyncPlots::FillAllHistograms<ULong64_t, ULong64_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kBool_t, kBool_t },
                  &Print_SyncPlots::FillAllHistograms<Bool_t, Bool_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kBool_t, kChar_t },
                  &Print_SyncPlots::FillAllHistograms<Bool_t, Char_t, MySelector, OtherSelector, Hist, Hist2D> },
                { { kBool_t, kInt_t },
                  &Print_SyncPlots::FillAllHistograms<Bool_t, Int_t, MySelector, OtherSelector, Hist, Hist2D> },
            };

            const auto typePair = DataTypePair(myType, otherType);
            if(!fillMethods.count(typePair))
                throw exception("Unknown branch type combination (%1%, %2%) for branch ('%3%', '%4%').")
                    % myType % otherType % mine_var % other_var;
            FillMethodPtr fillMethod = fillMethods.at(typePair);
            (this->*fillMethod)(mine_var, other_var, my_selector, other_selector, *Hmine_all, *Hother_all,
                       *Hmine_common, *Hother_common, *Hmine_vs_other, *Hmine_diff, *Hother_diff);

            DrawSuperimposedHistograms(Hmine_all, Hother_all, selection_label, mine_var, other_var, "all");
            DrawSuperimposedHistograms(Hmine_common, Hother_common, selection_label, mine_var, other_var, "common");
            DrawSuperimposedHistograms(Hmine_diff, Hother_diff, selection_label, mine_var, other_var, "different");
            Draw2DHistogram(Hmine_vs_other, selection_label, mine_var, other_var);

        } catch(std::runtime_error& e){
            std::cerr << "WARNING: " << e.what() << std::endl;
        }
    }

    static std::string MakeTitle(const std::string& mine_var, const std::string& other_var,
                                 const std::string& event_subset, const std::string& selection_label)
    {
        std::ostringstream ss_title;
        ss_title << mine_var;
        if(other_var != mine_var)
            ss_title << " (" << other_var << ")";
        ss_title << ", " << event_subset << " events";
        if(selection_label.size())
            ss_title << ", pre-selection: " << selection_label;
        return ss_title.str();
    }

    void DrawSuperimposedHistograms(std::shared_ptr<TH1F> Hmine, std::shared_ptr<TH1F> Hother,
                                    const std::string& selection_label, const std::string& mine_var,
                                    const std::string& other_var, const std::string& event_subset)
    {
        const std::string title = MakeTitle(mine_var, other_var, event_subset, selection_label);
        Hmine->SetTitle(title.c_str());
        Hmine->GetYaxis()->SetTitle("Events");
        Hmine->GetXaxis()->SetTitle(mine_var.c_str());
        Hmine->SetLineColor(1);
        Hmine->SetMarkerColor(1);
        Hmine->SetStats(0);

//        Hother->GetYaxis()->SetTitle(selection_label.c_str());
//        Hother->GetXaxis()->SetTitle(var.c_str());
        Hother->SetLineColor(2);
        Hother->SetMarkerColor(2);
        Hother->SetStats(0);

        TPad pad1("pad1","",0,0.2,1,1);
        TPad pad2("pad2","",0,0,1,0.2);

        pad1.cd();

        // Draw one histogram on top of the other
        if(Hmine->GetMaximum()>Hother->GetMaximum())
            Hmine->GetYaxis()->SetRangeUser(0,Hmine->GetMaximum()*1.1);
        else
            Hmine->GetYaxis()->SetRangeUser(0,Hother->GetMaximum()*1.1);
        Hmine->Draw("hist");
        Hother->Draw("histsame");
        DrawTextLabels(Hmine->Integral(0,Hmine->GetNbinsX()+1), Hother->Integral(0,Hother->GetNbinsX()+1));

        pad2.cd();

        // Draw the ratio of the historgrams
        std::unique_ptr<TH1F> HDiff((TH1F*)Hother->Clone("HDiff"));
        HDiff->Divide(Hmine.get());
        ///HDiff->GetYaxis()->SetRangeUser(0.9,1.1);
        HDiff->GetYaxis()->SetRangeUser(0.9,1.1);
        //HDiff->GetYaxis()->SetRangeUser(0.98,1.02);
        //HDiff->GetYaxis()->SetRangeUser(0.,2.0);
        HDiff->GetYaxis()->SetNdivisions(3);
        HDiff->GetYaxis()->SetLabelSize(0.1);
        HDiff->GetYaxis()->SetTitleSize(0.1);
        HDiff->GetYaxis()->SetTitleOffset(0.5);
        //HDiff->GetYaxis()->SetTitle(myGroup + " / " + group);
        HDiff->GetYaxis()->SetTitle("Ratio");
        HDiff->GetXaxis()->SetNdivisions(-1);
        HDiff->GetXaxis()->SetTitle("");
        HDiff->GetXaxis()->SetLabelSize(0.0001);
        HDiff->SetMarkerStyle(7);
        HDiff->SetMarkerColor(2);
        HDiff->Draw("histp");
        TLine line;
        line.DrawLine(HDiff->GetXaxis()->GetXmin(),1,HDiff->GetXaxis()->GetXmax(),1);

        canvas.Clear();
        pad1.Draw();
        pad2.Draw();

        PrintCanvas(title);
    }

    void Draw2DHistogram(std::shared_ptr<TH2F> Hmine_vs_other, const std::string& selection_label,
                         const std::string& mine_var, const std::string& other_var)
    {
        const std::string title = MakeTitle(mine_var, other_var, "common", selection_label);
        Hmine_vs_other->SetTitle(title.c_str());
        const std::string my_name = mine_var + "_mine";
        const std::string other_name = other_var + "_other";
        std::ostringstream y_name;
        y_name << "(" << other_name << " - " << my_name << ")/" << other_name;
        Hmine_vs_other->GetXaxis()->SetTitle(other_name.c_str());
        Hmine_vs_other->GetYaxis()->SetTitle(y_name.str().c_str());
        Hmine_vs_other->SetStats(0);

        TPad pad1("pad1","", 0, 0, 1, 1);
        pad1.cd();
        Hmine_vs_other->Draw("colz");
        const size_t n_events = Hmine_vs_other->Integral(0, Hmine_vs_other->GetNbinsX() + 1,
                                                         0, Hmine_vs_other->GetNbinsY() + 1);
        DrawTextLabels(n_events, n_events);
        canvas.Clear();
        pad1.Draw();
        PrintCanvas(title + " 2D", isLastDraw);
    }

    void DrawTextLabels(size_t n_events_mine, size_t n_events_other)
    {
        TText TXmine;
        TXmine.SetTextColor(1);
        TXmine.SetTextSize(.04);
        TText TXother;
        TXother.SetTextColor(2);
        TXother.SetTextSize(.04);

        TXmine.DrawTextNDC(.23,.84,myGroup+" : " + n_events_mine);
        TXother.DrawTextNDC(.53,.84,group+": " + n_events_other);
    }

    template<typename MyVarType, typename OtherVarType, typename MySelector, typename OtherSelector,
             typename Histogram, typename Histogram2D>
    void FillAllHistograms(const std::string& mine_var, const std::string& other_var, const MySelector& my_selector,
                           const OtherSelector& other_selector, Histogram& Hmine_all, Histogram& Hother_all,
                           Histogram& Hmine_common, Histogram& Hother_common, Histogram2D& Hmine_vs_other,
                           Histogram& Hmine_diff, Histogram& Hother_diff)
    {
        const std::vector<MyVarType> my_values = CollectValues<MyVarType>(*Tmine, mine_var);
        const std::vector<OtherVarType> other_values = CollectValues<OtherVarType>(*Tother, other_var);
        FillCommonHistograms(mine_var, my_values, other_values, my_selector, other_selector,
                             Hmine_common, Hother_common, Hmine_vs_other);
        FillInclusiveHistogram(my_values, my_selector, Hmine_all);
        FillInclusiveHistogram(other_values, other_selector, Hother_all);

        FillExclusiveHistogram(my_values, other_values, my_selector, other_selector, Hmine_diff, Hother_diff);
    }

    template<typename MyVarType, typename OtherVarType, typename MySelector, typename OtherSelector,
             typename Histogram, typename Histogram2D>
    void FillCommonHistograms(const std::string& var, const std::vector<MyVarType>& my_values,
                              const std::vector<OtherVarType>& other_values,
                              const MySelector& my_selector, const OtherSelector& other_selector,
                              Histogram& my_histogram, Histogram& other_histogram, Histogram2D& histogram2D)
    {
        std::cout << var << " bad events:\n";
        for(const auto& event_entry_pair : common_event_to_entry_pair_map) {
            const size_t my_entry = event_entry_pair.second.first;
            const size_t other_entry = event_entry_pair.second.second;
            if(!my_selector(my_entry) || !other_selector(other_entry))
                continue;
            const MyVarType& my_value = my_values.at(my_entry);
            const OtherVarType& other_value = other_values.at(other_entry);
            my_histogram.Fill(my_value);
            other_histogram.Fill(other_value);
            if(other_value) {
                const double y_value = double(other_value - my_value)/other_value;
                const auto diff = other_value - my_value;
                if (std::abs(y_value) >= 0.1 )
                    std::cout << "run:lumi:evt = " << event_entry_pair.first << ", other = " << other_value
                              << ", my = " << my_value << ", other - my = " << diff << std::endl;
                histogram2D.Fill(other_value, y_value);
            }
        }
    }

    template<typename VarType, typename Histogram, typename Selector>
    void FillInclusiveHistogram(const std::vector<VarType>& values, const Selector& selector, Histogram& histogram)
    {
        for(size_t n = 0; n < values.size(); ++n) {
            if(selector(n))
                histogram.Fill(values.at(n));
        }
    }

    template<typename MyVarType, typename OtherVarType, typename Histogram, typename MySelector, typename OtherSelector>
    void FillExclusiveHistogram(const std::vector<MyVarType>& my_values,
                                const std::vector<OtherVarType>& other_values,
                                const MySelector& my_selector, const OtherSelector& other_selector,
                                Histogram& my_histogram, Histogram& other_histogram)
    {
        for (const auto& event_entry_pair : common_event_to_entry_pair_map){
            const size_t my_entry = event_entry_pair.second.first;
            const size_t other_entry = event_entry_pair.second.second;
            if(my_selector(my_entry) && !other_selector(other_entry)){
                const MyVarType& my_value = my_values.at(my_entry);
                my_histogram.Fill(my_value);
            }
            if(!my_selector(my_entry) && other_selector(other_entry)){
                const OtherVarType& other_value = other_values.at(other_entry);
                other_histogram.Fill(other_value);
            }
        }
        for(const auto& event_entry : my_events_only_map) {
            if(my_selector(event_entry.second)) {
                const MyVarType& value = my_values.at(event_entry.second);
                my_histogram.Fill(value);
            }
        }
        for(const auto& event_entry : other_events_only_map) {
            if(other_selector(event_entry.second)) {
                const OtherVarType& value = other_values.at(event_entry.second);
                other_histogram.Fill(value);
            }
        }
    }

    void CollectEvents(TTree& my_tree, TTree& other_tree)
    {
        my_events = CollectEventIds(my_tree, config.GetMyIdBranches());
        const EventSet my_events_set(my_events.begin(), my_events.end());
        other_events = CollectEventIds(other_tree, config.GetOtherIdBranches());
        const EventSet other_events_set(other_events.begin(), other_events.end());

        EventSet intersection, my_events_only, other_events_only;
        EventSetIntersection(my_events_set, other_events_set, intersection);
        EventSetDifference(my_events_set, other_events_set, my_events_only);
        EventSetDifference(other_events_set, my_events_set, other_events_only);

        EventToEntryMap my_event_to_entry_map, other_event_to_entry_map;
        FillEventToEntryMap(my_events, my_event_to_entry_map);
        FillEventToEntryMap(other_events, other_event_to_entry_map);

        std::cout << "# my events = " << my_events.size() << ", " << "# my unique events = " << my_events_set.size()
                  << "\n# other events = " << other_events.size()
                  << ", # other unique events = " << other_events_set.size()
                  << "\n# common events = " << intersection.size() << std::endl;

        std::cout << "Mine events" << std::endl;
        for(const auto& event_entry : my_event_to_entry_map) {
            if(intersection.count(event_entry.first)) {
                common_event_to_entry_pair_map[event_entry.first] =
                        std::pair<size_t, size_t>(event_entry.second, other_event_to_entry_map.at(event_entry.first));
            }
            if(my_events_only.count(event_entry.first)) {
                my_events_only_map[event_entry.first] = event_entry.second;
                std::cout << "run:lumi:evt = " << event_entry.first << std::endl;
            }
        }

        std::cout << "Other's events" << std::endl;
        for(const auto& event_entry : other_event_to_entry_map) {
            if(other_events_only.count(event_entry.first)){
                other_events_only_map[event_entry.first] = event_entry.second;
                std::cout << "run:lumi:evt = " << event_entry.first << std::endl;
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
        intersection_vector.resize(iter - intersection_vector.begin());
        intersection_set.clear();
        intersection_set.insert(intersection_vector.begin(), intersection_vector.end());
    }

    static void EventSetDifference(const EventSet& first_set, const EventSet& second_set, EventSet& diff_set)
    {
        EventVector diff_vector(first_set.size());
        const auto iter = std::set_difference(first_set.begin(), first_set.end(),
                                              second_set.begin(), second_set.end(),
                                              diff_vector.begin());
        diff_vector.resize(iter - diff_vector.begin());
        diff_set.clear();
        diff_set.insert(diff_vector.begin(), diff_vector.end());
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

        using CollectMethodPtr = void (Print_SyncPlots::*)(TTree&, const std::string&, std::vector<VarType>&);
        using CollectMethodMap = std::map<EDataType, CollectMethodPtr>;

        static const CollectMethodMap collectMethods = {
            { kInt_t, &Print_SyncPlots::CollectAndConvertValue<Int_t, VarType> },
            { kUInt_t, &Print_SyncPlots::CollectAndConvertValue<UInt_t, VarType> },
            { kULong64_t, &Print_SyncPlots::CollectAndConvertValue<ULong64_t, VarType> },
            { kFloat_t, &Print_SyncPlots::CollectAndConvertValue<Float_t, VarType> },
            { kDouble_t, &Print_SyncPlots::CollectAndConvertValue<Double_t, VarType> },
            { kChar_t, &Print_SyncPlots::CollectAndConvertValue<Char_t, VarType> },
            { kBool_t, &Print_SyncPlots::CollectAndConvertValue<Bool_t, VarType> }
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

        EventVector events;
        for(size_t n = 0; n < evt.size(); ++n)
            events.push_back(EventIdentifier(run.at(n), lumi.at(n), evt.at(n)));
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
    SyncPlotConfig config;
    std::string channel, sample, myGroup, myRootFile, myTree, group, groupRootFile, groupTree;
    std::shared_ptr<TFile> Fmine, Fother, Ftmp;
    std::shared_ptr<TTree> Tmine, Tother;
    std::string tmpName;

    EventVector my_events, other_events;
    EventToEntryMap my_events_only_map, other_events_only_map;
    EventToEntryPairMap common_event_to_entry_pair_map;

    TCanvas canvas;
    std::string file_name;
    bool isFirstPage, isLastDraw;
};

} // namespace analysis

PROGRAM_MAIN(analysis::Print_SyncPlots, Arguments)

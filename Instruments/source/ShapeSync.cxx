/*! Produce sync plots between two groups for the selected distributions.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TKey.h>
#include <TCanvas.h>
#include <TRatioPlot.h>
#include <TText.h>

#include "AnalysisTools/Run/include/program_main.h"
#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/TextIO.h"
#include "AnalysisTools/Core/include/Tools.h"
#include "AnalysisTools/Core/include/AnalysisMath.h"
#include "AnalysisTools/Core/include/PropertyConfigReader.h"
#include "AnalysisTools/Print/include/PlotPrimitives.h"
#include "AnalysisTools/Print/include/DrawOptions.h"
#include "AnalysisTools/Print/include/RootPrintTools.h"

namespace analysis {
struct Arguments {
    REQ_ARG(std::string, cfg);
    REQ_ARG(std::string, output);
    REQ_ARG(std::vector<std::string>, input);
};

struct InputPattern {
    using Item = PropertyConfigReader::Item;
    using ItemCollection = PropertyConfigReader::ItemCollection;
    using RegexVector = std::vector<boost::regex>;

    InputPattern(const ItemCollection& config_items)
    {
        static const std::string targets_item_name = "targets";
        if(!config_items.count(targets_item_name))
            throw exception("Description of input patterns not found.");
        const auto& targets_item = config_items.at(targets_item_name);
        LoadPatterns(targets_item, "dir_names", dir_patterns);
        LoadPatterns(targets_item, "hist_names", hist_patterns);
    }

    bool DirMatch(const std::string& dir_name) const { return HasMatch(dir_name, dir_patterns); }
    bool HistMatch(const std::string& hist_name) const { return HasMatch(hist_name, hist_patterns); }

private:
    static void LoadPatterns(const Item& config_item, const std::string& p_name, RegexVector& patterns)
    {
        const std::string& p_value = config_item.Get<>(p_name);
        const auto& pattern_str_list = SplitValueList(p_value, false, " \t", true);
        for(const auto& pattern_str : pattern_str_list) {
            const std::string full_pattern = "^" + pattern_str + "$";
            patterns.emplace_back(full_pattern);
        }
    }

    static bool HasMatch(const std::string& name, const RegexVector& patterns)
    {
        for(const auto& pattern : patterns) {
            if(boost::regex_match(name, pattern))
                return true;
        }
        return false;
    }

private:
    RegexVector dir_patterns, hist_patterns;
};

struct Source {
    using ItemCollection = PropertyConfigReader::ItemCollection;
    using Hist = TH1;
    using HistPtr = std::shared_ptr<Hist>;
    using HistMap = std::map<std::string, HistPtr>;
    using DirHistMap = std::map<std::string, HistMap>;
    using ClassInheritance = root_ext::ClassInheritance;
    using Position = root_ext::Point<double, 2, false>;

    std::shared_ptr<TFile> file;
    std::string name;
    root_ext::Color color{kBlack};
    Position label_pos{.5, .5};
    float label_size{.02f};
    DirHistMap histograms;

    Source(size_t n, const std::vector<std::string>& inputs, const ItemCollection& config_items,
           const InputPattern& pattern) :
        file(root_ext::OpenRootFile(inputs.at(n)))
    {
        const std::string item_name = boost::str(boost::format("input%1%") % n);
        if(!config_items.count(item_name))
            throw exception("Descriptor for input #%1% not found.") % n;
        const auto& desc = config_items.at(item_name);
        name = desc.Get<>("name");
        if(!desc.Read("name", name))
            name = boost::str(boost::format("input %1%") % n);
        desc.Read("color", color);
        desc.Read("label_pos", label_pos);
        desc.Read("label_size", label_size);
        LoadHistograms(pattern);
    }

private:
    void LoadHistograms(const InputPattern& pattern)
    {
        TIter nextkey(file->GetListOfKeys());
        for(TKey* t_key; (t_key = dynamic_cast<TKey*>(nextkey()));) {
            const std::string dir_name = t_key->GetName();
            const auto inheritance = root_ext::FindClassInheritance(t_key->GetClassName());
            if(inheritance != ClassInheritance::TDirectory || !pattern.DirMatch(dir_name)) continue;
            if(histograms.count(dir_name))
                throw exception("Directory '%1%' has been already processed.") % dir_name;
            auto dir = root_ext::ReadObject<TDirectory>(*file, dir_name);
            LoadHistograms(pattern, dir, histograms[dir_name]);
        }
    }

    static void LoadHistograms(const InputPattern& pattern, TDirectory *dir, HistMap& hists)
    {
        TIter nextkey(dir->GetListOfKeys());
        for(TKey* t_key; (t_key = dynamic_cast<TKey*>(nextkey()));) {
            const std::string hist_name = t_key->GetName();
            const auto inheritance = root_ext::FindClassInheritance(t_key->GetClassName());
            if(inheritance != ClassInheritance::TH1 || !pattern.HistMatch(hist_name)) continue;
            if(hists.count(hist_name))
                throw exception("Histogram '%1%' in directory '%2%' has been already processed.") % hist_name
                    % dir->GetName();
            hists[hist_name] = HistPtr(root_ext::ReadObject<TH1>(*dir, hist_name));
        }
    }
};

class ShapeSync {
public:
    using InputDesc = PropertyConfigReader::Item;
    using NameSet = std::set<std::string>;
    using SampleItemNamesMap = std::map<std::string, NameSet>;
    using HistPtr = Source::HistPtr;
    using DrawOptions = ::root_ext::draw_options::Page;

    ShapeSync(const Arguments& _args) :
        args(_args)
    {
        static const std::string draw_opt_item_name = "draw_opt";

        gROOT->SetBatch(true);
        gROOT->SetMustClean(true);

        PropertyConfigReader config;
        config.Parse(args.cfg());
        if(args.input().size() < 2)
            throw exception("At least 2 inputs should be provided.");
        patterns = std::make_shared<InputPattern>(config.GetItems());

        if(!config.GetItems().count(draw_opt_item_name))
            throw analysis::exception("Draw options not found.");
        draw_options = std::make_shared<DrawOptions>(config.GetItems().at(draw_opt_item_name));

        for(size_t n = 0; n < args.input().size(); ++n)
            inputs.emplace_back(n, args.input(), config.GetItems(), *patterns);
        canvas = std::make_shared<TCanvas>("canvas", "", draw_options->canvas_size.x(),
                                           draw_options->canvas_size.y());
        canvas->cd();
    }

    void Run()
    {
        const auto common_dirs = GetCommonDirs();

        for(auto dir_iter = common_dirs.begin(); dir_iter != common_dirs.end(); ++dir_iter) {
            std::cout << "Processing directory " << *dir_iter << "..." << std::endl;
            const auto common_hists = GetCommonHists(*dir_iter);
            for(auto hist_iter = common_hists.begin(); hist_iter != common_hists.end(); ++hist_iter) {
                const bool is_last = std::next(dir_iter) == common_dirs.end() &&
                                     std::next(hist_iter) == common_hists.end();
                DrawHistogram(*dir_iter, *hist_iter, is_last);
            }
        }
    }

private:
    NameSet GetCommonDirs() const
    {
        SampleItemNamesMap dir_names;
        for(const auto& input : inputs)
            dir_names[input.name] = tools::collect_map_keys(input.histograms);
        const NameSet common_dirs = CollectCommonItems(dir_names);
        ReportNotCommonItems(std::cout, dir_names, common_dirs, "directories");
        return common_dirs;
    }

    NameSet GetCommonHists(const std::string& dir_name) const
    {
        SampleItemNamesMap hist_names;
        for(const auto& input : inputs)
            hist_names[input.name] = tools::collect_map_keys(input.histograms.at(dir_name));
        const NameSet common_hists = CollectCommonItems(hist_names);
        ReportNotCommonItems(std::cout, hist_names, common_hists, "histograms");
        return common_hists;
    }

    static NameSet CollectCommonItems(const SampleItemNamesMap& items)
    {
        NameSet common_items;
        const auto first_input = items.begin();
        for(const auto& item : first_input->second) {
            bool is_common = true;
            for(auto input_iter = std::next(first_input); input_iter != items.end(); ++input_iter) {
                if(!input_iter->second.count(item)) {
                    is_common = false;
                    break;
                }
            }
            if(is_common)
                common_items.insert(item);
        }
        return common_items;
    }

    static void ReportNotCommonItems(std::ostream& os, const SampleItemNamesMap& items, const NameSet& common_items,
                                     const std::string& items_type_name)
    {
        bool has_not_common = false;
        for(auto input_iter = items.begin(); input_iter != items.end(); ++input_iter) {
            bool has_input_not_common = false;
            for(const auto& item_name : input_iter->second) {
                if(common_items.count(item_name)) continue;
                if(!has_not_common)
                    os << "Not common " << items_type_name << ":\n";
                has_not_common = true;
                if(!has_input_not_common)
                    os << input_iter->first << ": ";
                else
                    os << ", ";
                has_input_not_common = true;
                os << item_name;
            }
            if(has_input_not_common)
                os << "\n";
        }
        if(has_not_common)
            os << std::endl;
    }

    void PrintCanvas(const std::string& page_name, bool is_last_page)
    {
        std::ostringstream print_options, output_name;
        print_options << "Title:" << page_name;
        output_name << args.output();
        if(is_first_page && !is_last_page)
            output_name << "(";
        else if(is_last_page && !is_first_page)
            output_name << ")";
        is_first_page = false;

        root_ext::WarningSuppressor ws(kWarning);
        canvas->Print(output_name.str().c_str(), print_options.str().c_str());
    }

    void DrawHistogram(const std::string& dir_name, const std::string& hist_name, bool is_last)
    {
        const std::string title = dir_name + ": " + hist_name;

        auto input = inputs.begin();
        auto hist = input->histograms.at(dir_name).at(hist_name);
        std::map<std::string, PhysicalValue> integrals;
        root_ext::PlotRangeTuner rangeTuner;

        hist->SetTitle(title.c_str());
        hist->GetXaxis()->SetTitle(draw_options->x_title.c_str());
        hist->GetYaxis()->SetTitle(draw_options->y_title.c_str());
        hist->GetXaxis()->SetTitleOffset(draw_options->axis_title_offsets.x());
        hist->GetYaxis()->SetTitleOffset(draw_options->axis_title_offsets.y());
        hist->SetStats(0);

        for(; input != inputs.end(); ++input) {
            hist = input->histograms.at(dir_name).at(hist_name);
            hist->SetLineColor(input->color.GetColor_t());
            hist->SetMarkerColor(input->color.GetColor_t());
            integrals[input->name] = Integral(*input->histograms.at(dir_name).at(hist_name), false);
            if(draw_options->divide_by_bin_width)
                root_ext::plotting::DivideByBinWidth(*hist);
            hist->SetMarkerStyle(kDot);
            rangeTuner.Add(*hist, false, true);
        }

        input = inputs.begin();
        auto h1 = (input++)->histograms.at(dir_name).at(hist_name);
        auto h2 = (input++)->histograms.at(dir_name).at(hist_name);


        TRatioPlot* ratio_plot;
        {
            root_ext::WarningSuppressor ws(kError);
            ratio_plot = new TRatioPlot(h1.get(), h2.get());
        }
        ratio_plot->SetH1DrawOpt("");
        ratio_plot->SetH2DrawOpt("");
        root_ext::plotting::SetMargins(*ratio_plot, draw_options->margins);
        ratio_plot->Draw();
        ratio_plot->GetLowerRefYaxis()->SetLabelSize(draw_options->ratio_y_label_size);
        rangeTuner.SetRangeY(*ratio_plot->GetUpperRefYaxis(), draw_options->log_y, draw_options->y_max_sf,
                             draw_options->y_min_sf, draw_options->y_min_log);
        ratio_plot->GetUpperPad()->SetLogy(draw_options->log_y);
        if(draw_options->max_ratio > 0)
            ratio_plot->GetLowerRefYaxis()->SetRangeUser(std::max(0., 2 - draw_options->max_ratio),
                                                         draw_options->max_ratio);
        std::vector<double> gridlines(2);
        gridlines[0] = std::max(0., 1 - draw_options->allowed_ratio_margin);
        gridlines[1] = 1 + draw_options->allowed_ratio_margin;
        ratio_plot->SetGridlines(gridlines);

        for(; input != inputs.end(); ++input)
            input->histograms.at(dir_name).at(hist_name)->Draw("same");

        for(input = inputs.begin(); input != inputs.end(); ++input) {
            auto integral = integrals.at(input->name);
            std::wostringstream ss;
            ss << std::wstring(input->name.begin(), input->name.end()) << L": ";
            if(integral.GetValue() > draw_options->zero_threshold)
                ss << integral;
            else
                ss << L"0";
            TText label;
            label.SetTextColor(input->color.GetColor_t());
            label.SetTextSize(input->label_size);
            label.DrawTextNDC(input->label_pos.x(), input->label_pos.y(), ss.str().c_str());
        }

        canvas->Update();

        PrintCanvas(title, is_last);
    }

private:
    Arguments args;
    std::vector<Source> inputs;
    std::shared_ptr<InputPattern> patterns;
    std::shared_ptr<DrawOptions> draw_options;
    std::shared_ptr<TCanvas> canvas;
    bool is_first_page{true};
};

} // namespace analysis

PROGRAM_MAIN(analysis::ShapeSync, analysis::Arguments)

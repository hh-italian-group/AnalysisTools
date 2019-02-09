/*! Code to produce stacked plots.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <THStack.h>
#include "AnalysisTools/Core/include/SmartHistogram.h"
#include "DrawOptions.h"
#include "PlotDescriptor.h"
#include "RootPrintTools.h"

namespace root_ext {

class StackedPlotDescriptor : public PlotDescriptor {
public:
    using exception = ::analysis::exception;
    using Hist = SmartHistogram<TH1D>;
    using HistPtr = std::shared_ptr<Hist>;
    using HistPtrVec = std::vector<HistPtr>;
    using Graph = TGraphAsymmErrors;
    using GraphPtr = std::shared_ptr<Graph>;
    using PageOptions = draw_options::Page;
    using HistOptions = draw_options::Histogram;
    using MultiRange = Hist::MultiRange;

    StackedPlotDescriptor(const PageOptions& _page_opt, const draw_options::ItemCollection& opt_items);

    void AddSignalHistogram(const Hist& original_hist, const std::string& legend_title, const Color& color,
                            double scale_factor);

    void AddBackgroundHistogram(const Hist& original_hist, const std::string& legend_title, const Color& color);

    void SetTotalBkg(const Hist& hist);
    void AddDataHistogram(const Hist& original_hist, const std::string& legend_title);
    virtual bool HasPrintableContent() const override;

    virtual void Draw(std::shared_ptr<TPad> main_pad, std::shared_ptr<TPad> ratio_pad, std::shared_ptr<TLegend> legend,
              std::vector<std::shared_ptr<TObject>>& plot_items) override;

private:
    void UpdatePageOptions(const Hist& hist);

    template<typename Item>
    static void ApplyHistOptions(Item& item, const HistOptions& opt)
    {
        item.SetFillStyle(opt.fill_style);
        item.SetFillColor(opt.fill_color.GetColor_t());
        item.SetLineStyle(opt.line_style);
        item.SetLineWidth(opt.line_width);
        item.SetLineColor(opt.line_color.GetColor_t());
        item.SetMarkerStyle(opt.marker_style);
        item.SetMarkerSize(opt.marker_size);
        item.SetMarkerColor(opt.marker_color.GetColor_t());
    }

    static void ApplyHistOptionsEx(Hist& hist, const HistOptions& opt);

    HistPtr PrepareHistogram(const Hist& original_histogram, const HistOptions& opt, const std::string& legend_title,
                             const Color& line_color, const Color& fill_color, bool is_data, double scale_factor = 1.);

    static HistPtr CreateSumHistogram(const HistPtrVec& hists);

    void ApplyAxisSetup(TH1& frame_hist, bool has_ratio) const;
    void ApplyRatioAxisSetup(TH1& frame_hist) const;

    template<typename Item>
    void DrawItem(Item& item, const std::string& draw_opt) const
    {
        const auto opt = "SAME" + draw_opt;
        item.Draw(opt.c_str());
    }

private:
    HistPtrVec signals, backgrounds;
    HistPtr data, bkg_sum;
    GraphPtr data_graph;

    PageOptions page_opt;
    HistOptions signal_opt, bkg_opt, data_opt;
    boost::optional<HistOptions> bkg_unc_opt;

    PlotRangeTuner range_tuner;
};

} // namespace analysis

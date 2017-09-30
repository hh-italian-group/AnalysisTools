/*! Contains useful functions to print histograms with ROOT.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <limits>
#include <vector>
#include <map>

#include <Rtypes.h>
#include <TPaveStats.h>
#include <TPaveLabel.h>
#include <TPad.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TColor.h>

#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/NumericPrimitives.h"
#include "PlotPrimitives.h"

namespace root_ext {

int CreateTransparentColor(int color, float alpha)
{
  TColor* adapt = gROOT->GetColor(color);
  int new_idx = gROOT->GetListOfColors()->GetSize() + 1;
  TColor* trans = new TColor(new_idx, adapt->GetRed(), adapt->GetGreen(),
                             adapt->GetBlue(), "", alpha);
  trans->SetName(Form("userColor%i", new_idx));
  return new_idx;
}

template<typename Histogram, typename ValueType=Double_t>
class HistogramFitter {
public:
    using Range = ::analysis::Range<ValueType>;

    static ValueType FindMinLimitX(const Histogram& h)
    {
        for(Int_t i = 0; i < h.GetNbinsX(); ++i) {
            if(h.GetBinContent(i) != ValueType(0))
                return h.GetBinLowEdge(i);
        }
        return std::numeric_limits<ValueType>::max();
    }

    static ValueType FindMaxLimitX(const Histogram& h)
    {
        for(Int_t i = h.GetNbinsX() - 1; i > 0; --i) {
            if(h.GetBinContent(i) != ValueType(0))
                return h.GetBinLowEdge(i) + h.GetBinWidth(i);
        }
        return std::numeric_limits<ValueType>::lowest();
    }

    static ValueType FindMinLimitY(const Histogram& h)
    {
        ValueType min = std::numeric_limits<ValueType>::max();
        for(Int_t i = 0; i <= h.GetNbinsX() + 1; ++i) {
            if(h.GetBinContent(i) != ValueType(0))
                min = std::min(min, h.GetBinContent(i));
        }
        return min;
    }

    static ValueType FindMaxLimitY(const Histogram& h)
    {
        ValueType max = std::numeric_limits<ValueType>::lowest();
        for(Int_t i = 0; i <= h.GetNbinsX() + 1; ++i) {
            if(h.GetBinContent(i) != ValueType(0))
                max = std::max(max, h.GetBinContent(i));
        }
        return max;
    }


    template<typename Container>
    static void SetRanges(const Container& hists, bool fitX, bool fitY, Range xRange, Range yRange, bool isLogY)
    {
        if(!hists.size())
            return;

        if(fitX) {
            ValueType min = std::numeric_limits<ValueType>::max();
            ValueType max = std::numeric_limits<ValueType>::lowest();
            for(auto h : hists) {
                if(!h) continue;
                min = std::min(min, FindMinLimitX(*h));
                max = std::max(max, FindMaxLimitX(*h));
            }
            xRange = Range(min, max);
        }

        if(fitY) {
            ValueType min = std::numeric_limits<ValueType>::max();
            ValueType max = std::numeric_limits<ValueType>::lowest();
            for(auto h : hists) {
                if(!h) continue;
                min = std::min(min, FindMinLimitY(*h));
                max = std::max(max, FindMaxLimitY(*h));
            }
            const double factor = isLogY ? 2.0 : 1.1;
            min /= factor;
            max *= factor;
            if(isLogY) {
                min = std::max(min, std::numeric_limits<ValueType>::min());
                max = std::max(max, std::numeric_limits<ValueType>::min());
            }
            yRange = Range(min, max);
        }

        for(auto h : hists) {
            if(!h) continue;
            h->SetAxisRange(xRange.min(), xRange.max(), "X");
            h->SetAxisRange(yRange.min(), yRange.max(), "Y");
        }
    }

private:
    HistogramFitter() {}
};

class Adapter {
public:
    static TPaveLabel* NewPaveLabel(const Box<double>& box, const std::string& text)
    {
        return new TPaveLabel(box.left_bottom().x(), box.left_bottom().y(), box.right_top().x(), box.right_top().y(),
                              text.c_str());
    }

    static TPad* NewPad(const Box<double>& box)
    {
        static const char* pad_name = "pad";
        return new TPad(pad_name, pad_name, box.left_bottom().x(), box.left_bottom().y(), box.right_top().x(),
                        box.right_top().y());
    }

private:
    Adapter() {}
};

template<typename Histogram, typename ValueType=Double_t>
class HistogramPlotter {
public:
    struct Options {
        Color_t color;
        Width_t line_width;
        Box<double> pave_stats_box;
        Double_t pave_stats_text_size;
        Color_t fit_color;
        Width_t fit_line_width;
        Options() : color(kBlack), line_width(1), pave_stats_text_size(0), fit_color(kBlack), fit_line_width(1) {}
        Options(Color_t _color, Width_t _line_width, const Box<double>& _pave_stats_box, Double_t _pave_stats_text_size,
            Color_t _fit_color, Width_t _fit_line_width)
            : color(_color), line_width(_line_width), pave_stats_box(_pave_stats_box),
              pave_stats_text_size(_pave_stats_text_size), fit_color(_fit_color), fit_line_width(_fit_line_width) {}
    };

    struct Entry {
        Histogram* histogram;
        Options plot_options;
        Entry(Histogram* _histogram, const Options& _plot_options)
            : histogram(_histogram), plot_options(_plot_options) {}
    };

    using HistogramContainer = std::vector<Histogram*>;

public:
    HistogramPlotter(const std::string& _title, const std::string& _axis_titleX, const std::string& _axis_titleY)
        : title(_title), axis_titleX(_axis_titleX),axis_titleY(_axis_titleY) {}

    void Add(Histogram* histogram, const Options& plot_options)
    {
        histograms.push_back(histogram);
        options.push_back(plot_options);
    }

    void Add(const Entry& entry)
    {
        histograms.push_back(entry.histogram);
        options.push_back(entry.plot_options);
    }

    void Superpose(TPad* main_pad, TPad* stat_pad, bool draw_legend, const Box<double>& legend_box,
                   const std::string& draw_options)
    {
        if(!histograms.size() || !main_pad)
            return;

        histograms[0]->SetTitle(title.c_str());
        histograms[0]->GetXaxis()->SetTitle(axis_titleX.c_str());
        histograms[0]->GetYaxis()->SetTitle(axis_titleY.c_str());

        TLegend* legend = 0;
        if(draw_legend) {
            legend = new TLegend(legend_box.left_bottom().x(), legend_box.left_bottom().y(),
                                 legend_box.right_top().x(), legend_box.right_top().y());
        }

        for(unsigned n = 0; n < histograms.size(); ++n) {
            main_pad->cd();
            const Options& o = options[n];
            Histogram* h = histograms[n];
            if(!h)
                continue;

            const char* opt = n ? "sames" : draw_options.c_str();
            h->Draw(opt);
            if(legend) {
                legend->AddEntry(h, h->GetName());
                legend->Draw();
            }

            main_pad->Update();
            if(!stat_pad)
                continue;
            stat_pad->cd();
            TPaveStats *pave_stats = dynamic_cast<TPaveStats*>(h->GetListOfFunctions()->FindObject("stats"));

            TPaveStats *pave_stats_copy = root_ext::CloneObject(*pave_stats);
            h->SetStats(0);

            pave_stats_copy->SetX1NDC(o.pave_stats_box.left_bottom().x());
            pave_stats_copy->SetX2NDC(o.pave_stats_box.right_top().x());
            pave_stats_copy->SetY1NDC(o.pave_stats_box.left_bottom().y());
            pave_stats_copy->SetY2NDC(o.pave_stats_box.right_top().y());
            pave_stats_copy->ResetAttText();
            pave_stats_copy->SetTextColor(o.color);
            pave_stats_copy->SetTextSize(static_cast<float>(o.pave_stats_text_size));
            pave_stats_copy->Draw();
            stat_pad->Update();
        }
    }

    const HistogramContainer& Histograms() const { return histograms; }

private:
    HistogramContainer histograms;
    std::vector<Options> options;
    std::string title;
    std::string axis_titleX, axis_titleY;
};

} // root_ext

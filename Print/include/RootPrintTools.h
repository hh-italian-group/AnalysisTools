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

namespace root_ext {

static const std::map<std::string, EColor> colorMapName = {{"white",kWhite}, {"black",kBlack}, {"gray",kGray},
                                                           {"red",kRed}, {"green",kGreen}, {"blue",kBlue},
                                                           {"yellow",kYellow}, {"magenta",kMagenta}, {"cyan",kCyan},
                                                           {"orange",kOrange}, {"spring",kSpring}, {"teal",kTeal},
                                                           {"azure",kAzure},
                                                           {"azure_custom", static_cast<EColor>(TColor::GetColor(100,182,232))},
                                                           {"violet",kViolet},{"pink",kPink},
                                                           {"pink_custom", static_cast<EColor>(TColor::GetColor(250,202,255))},
                                                           {"red_custom", static_cast<EColor>(TColor::GetColor(222,90,106))},
                                                           {"violet_custom", static_cast<EColor>(TColor::GetColor(155,152,204))},
                                                           {"yellow_custom", static_cast<EColor>(TColor::GetColor(248,206,104))}};

int CreateTransparentColor(int color, float alpha)
{
  TColor* adapt = gROOT->GetColor(color);
  int new_idx = gROOT->GetListOfColors()->GetSize() + 1;
  TColor* trans = new TColor(new_idx, adapt->GetRed(), adapt->GetGreen(),
                             adapt->GetBlue(), "", alpha);
  trans->SetName(Form("userColor%i", new_idx));
  return new_idx;
}

struct Range {
    Double_t min, max;
    Range() : min(0), max(0) {}
    Range(Double_t _min, Double_t _max) : min(_min), max(_max) {}
};

template<typename Histogram, typename ValueType=Double_t>
class HistogramFitter {
public:
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
            xRange.min = std::numeric_limits<ValueType>::max();
            xRange.max = std::numeric_limits<ValueType>::lowest();
            for(auto h : hists) {
                if(!h) continue;
                xRange.min = std::min(xRange.min, FindMinLimitX(*h));
                xRange.max = std::max(xRange.max, FindMaxLimitX(*h));
            }
        }

        if(fitY) {
            yRange.min = std::numeric_limits<ValueType>::max();
            yRange.max = std::numeric_limits<ValueType>::lowest();
            for(auto h : hists) {
                if(!h) continue;
                yRange.min = std::min(yRange.min, FindMinLimitY(*h));
                yRange.max = std::max(yRange.max, FindMaxLimitY(*h));
            }
            const double factor = isLogY ? 2.0 : 1.1;
            yRange.min /= factor;
            yRange.max *= factor;
            if(isLogY) {
                yRange.min = std::max(yRange.min, std::numeric_limits<ValueType>::min());
                yRange.max = std::max(yRange.max, std::numeric_limits<ValueType>::min());
            }
        }

        for(auto h : hists) {
            if(!h) continue;
            h->SetAxisRange(xRange.min, xRange.max, "X");
            h->SetAxisRange(yRange.min, yRange.max, "Y");
        }
    }

private:
    HistogramFitter() {}
};

struct Point {
    Double_t x, y;
    Point() : x(0), y(0) {}
    Point(Double_t _x, Double_t _y) : x(_x), y(_y) {}
};

struct Box {
    Point left_bottom, right_top;
    Box() {}
    Box(const Point& _left_bottom, const Point& _right_top) : left_bottom(_left_bottom), right_top(_right_top) {}
    Box(Double_t left_bottom_x, Double_t left_bottom_y, Double_t right_top_x, Double_t right_top_y)
        : left_bottom(left_bottom_x, left_bottom_y), right_top(right_top_x, right_top_y) {}
};

class Adapter {
public:
    static TPaveLabel* NewPaveLabel(const Box& box, const std::string& text)
    {
        return new TPaveLabel(box.left_bottom.x, box.left_bottom.y, box.right_top.x, box.right_top.y, text.c_str());
    }

    static TPad* NewPad(const Box& box)
    {
        static const char* pad_name = "pad";
        return new TPad(pad_name, pad_name, box.left_bottom.x, box.left_bottom.y, box.right_top.x, box.right_top.y);
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
        Box pave_stats_box;
        Double_t pave_stats_text_size;
        Color_t fit_color;
        Width_t fit_line_width;
        Options() : color(kBlack), line_width(1), pave_stats_text_size(0), fit_color(kBlack), fit_line_width(1) {}
        Options(Color_t _color, Width_t _line_width, const Box& _pave_stats_box, Double_t _pave_stats_text_size,
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

    void Superpose(TPad* main_pad, TPad* stat_pad, bool draw_legend, const Box& legend_box,
                   const std::string& draw_options)
    {
        if(!histograms.size() || !main_pad)
            return;

        histograms[0]->SetTitle(title.c_str());
        histograms[0]->GetXaxis()->SetTitle(axis_titleX.c_str());
        histograms[0]->GetYaxis()->SetTitle(axis_titleY.c_str());

        TLegend* legend = 0;
        if(draw_legend) {
            legend = new TLegend(legend_box.left_bottom.x, legend_box.left_bottom.y,
                                 legend_box.right_top.x, legend_box.right_top.y);
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

            pave_stats_copy->SetX1NDC(o.pave_stats_box.left_bottom.x);
            pave_stats_copy->SetX2NDC(o.pave_stats_box.right_top.x);
            pave_stats_copy->SetY1NDC(o.pave_stats_box.left_bottom.y);
            pave_stats_copy->SetY2NDC(o.pave_stats_box.right_top.y);
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


std::istream& operator>>(std::istream& s, EColor& color){
    std::string name;
    s >> name;
    if (!root_ext::colorMapName.count(name)){
        std::ostringstream ss;
        ss << "undefined color: '" << name ;
        throw std::runtime_error(ss.str());
    }
    color = root_ext::colorMapName.at(name);
    return s;
}

std::ostream& operator<<(std::ostream& s, const EColor& color){
    for(const auto& entry : root_ext::colorMapName) {
        if(entry.second == color) {
            s << entry.first;
            return s;
        }
    }
    s << "Unknown color " << color;
    return s;
}

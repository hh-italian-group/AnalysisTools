/*! Definition of histogram source classes and page layot options that are used to print ROOT histograms.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <string>

#include "RootPrintTools.h"
#include "AnalysisTools/Core/include/RootExt.h"

namespace root_ext {

struct PageSideLayout {
    Box<double> main_pad;
    bool has_stat_pad;
    Box<double> stat_pad;
    bool has_legend;
    bool has_legend_pad;
    Box<double> legend_pad;
    bool has_ratio_pad;
    Box<double> ratio_pad;
};

struct PageSide {
    std::string histogram_name;
    std::string histogram_title;
    std::string axis_titleX;
    std::string axis_titleY;
    std::string draw_options;
    bool use_log_scaleX;
    bool use_log_scaleY;
    bool fit_range_x;
    bool fit_range_y;
    analysis::Range<double> xRange;
    analysis::Range<double> yRange;
    PageSideLayout layout;
};

struct PageLayout {
    bool has_title;
    Box<double> title_box;
    Font_t title_font;
    std::string global_style;
    Int_t stat_options;
    Int_t fit_options;
};

struct Page {
    using RegionCollection = std::vector<const PageSide*>;
    std::string title;
    PageLayout layout;

    virtual RegionCollection Regions() const = 0;
    virtual ~Page() {}
};

struct SingleSidedPage : public Page {
    PageSide side;

    explicit SingleSidedPage(bool has_title = true, bool has_stat_pad = true, bool has_legend = true)
    {
        layout.has_title = has_title;
        layout.title_box = Box<double>(0.1, 0.94, 0.9, 0.98);
        layout.title_font = 52;
        layout.global_style = "Plain";
        if(has_stat_pad) {
            layout.stat_options = 1111;
            layout.fit_options = 111;
        } else {
            layout.stat_options = 0;
            layout.fit_options = 0;
        }

        side.layout.has_stat_pad = has_stat_pad;
        side.layout.has_legend = has_legend;
        side.use_log_scaleX = false;
        side.use_log_scaleY = false;
        side.fit_range_x = true;
        side.fit_range_y = true;

        side.layout.main_pad = Box<double>(0.01, 0.01, 0.85, 0.91);
        side.layout.stat_pad = Box<double>(0.86, 0.01, 0.99, 0.91);
        side.layout.legend_pad = Box<double>(0.5, 0.67, 0.88, 0.88);
    }

    virtual RegionCollection Regions() const
    {
        RegionCollection regions;
        regions.push_back(&side);
        return regions;
    }
};

struct DoubleSidedPage : public Page {
    PageSide left_side, right_side;

    explicit DoubleSidedPage(bool has_title = true, bool has_stat_pad = true, bool has_legend = true)
    {
        layout.has_title = has_title;
        layout.title_box = Box<double>(0.1, 0.94, 0.9, 0.98);
        layout.title_font = 52;
        layout.global_style = "Plain";
        if(has_stat_pad) {
            layout.stat_options = 1111;
            layout.fit_options = 111;
        } else {
            layout.stat_options = 0;
            layout.fit_options = 0;
        }

        left_side.layout.has_stat_pad = has_stat_pad;
        left_side.layout.has_legend = has_legend;
        left_side.use_log_scaleX = false;
        left_side.use_log_scaleY = false;
        left_side.fit_range_x = true;
        left_side.fit_range_y = true;
        left_side.layout.main_pad = Box<double>(0.01, 0.01, 0.35, 0.91);
        left_side.layout.stat_pad = Box<double>(0.36, 0.01, 0.49, 0.91);
        left_side.layout.legend_pad = Box<double>(0.5, 0.67, 0.88, 0.88);

        right_side.layout.has_stat_pad = has_stat_pad;
        right_side.layout.has_legend = has_legend;
        right_side.use_log_scaleX = false;
        right_side.use_log_scaleY = false;
        right_side.fit_range_x = true;
        right_side.fit_range_y = true;
        right_side.layout.main_pad = Box<double>(0.51, 0.01, 0.85, 0.91);
        right_side.layout.stat_pad = Box<double>(0.86, 0.01, 0.99, 0.91);
        right_side.layout.legend_pad = Box<double>(0.5, 0.67, 0.88, 0.88);
    }

    virtual RegionCollection Regions() const
    {
        RegionCollection regions;
        regions.push_back(&left_side);
        regions.push_back(&right_side);
        return regions;
    }
};

template<typename HistogramType, typename _ValueType=Double_t, typename OriginalHistogramType=HistogramType>
class HistogramSource {
public:
    using Histogram = HistogramType;
    using OriginalHistogram = OriginalHistogramType;
    using ValueType = _ValueType;
    using PlotOptions = typename root_ext::HistogramPlotter<Histogram, ValueType>::Options;
    using Entry = typename root_ext::HistogramPlotter<Histogram, ValueType>::Entry;

    static PlotOptions& GetDefaultPlotOptions(size_t n)
    {
        static std::vector<PlotOptions> options;
        if(!options.size())
        {
            options.push_back( PlotOptions(kGreen, 1, root_ext::Box<double>(0.01, 0.71, 0.99, 0.9), 0.1, kGreen, 2) );
            options.push_back( PlotOptions(kViolet, 1, root_ext::Box<double>(0.01, 0.51, 0.99, 0.7), 0.1, kViolet, 2) );
            options.push_back( PlotOptions(kOrange, 1, root_ext::Box<double>(0.01, 0.31, 0.99, 0.5), 0.1, kOrange, 2) );
            options.push_back( PlotOptions(kRed, 1, root_ext::Box<double>(0.01, 0.11, 0.99, 0.3), 0.1, kRed, 2) );
            options.push_back( PlotOptions(kBlue, 1, root_ext::Box<double>(0.01, 0.11, 0.99, 0.3), 0.1, kBlue, 2) );
            options.push_back( PlotOptions(kBlack, 1, root_ext::Box<double>(0.01, 0.11, 0.99, 0.3), 0.1, kBlack, 2) );
        }
        return n < options.size() ? options[n] : options[options.size() - 1];
    }

public:
    void Add(const std::string& display_name, std::shared_ptr<TFile> source_file, const PlotOptions* plot_options = 0)
    {
        if(!plot_options)
            plot_options = &GetDefaultPlotOptions(display_names.size());

        display_names.push_back(display_name);
        source_files.push_back(source_file);
        plot_options_vector.push_back(*plot_options);
    }

    size_t Size() const { return display_names.size(); }

    Entry Get(unsigned id, const std::string& name) const
    {
        if(!source_files[id])
            return Entry(0, plot_options_vector[id]);
        std::string realName = GenerateName(id, name);
        OriginalHistogram* original_histogram = root_ext::ReadObject<OriginalHistogram>(*source_files[id], realName);
        Histogram* histogram = Convert(original_histogram);
        if(!histogram)
            throw std::runtime_error("source histogram not found.");
        Prepare(histogram, display_names[id], plot_options_vector[id]);
        return Entry(histogram, plot_options_vector[id]);
    }

    virtual ~HistogramSource() {}

protected:
    virtual Histogram* Convert(OriginalHistogram* original_histogram) const = 0;

    virtual std::string GenerateName(unsigned /*id*/, const std::string& name) const
    {
        return name;
    }

    virtual void Prepare(Histogram* histogram, const std::string& display_name,
                         const PlotOptions& plot_options) const
    {
        histogram->SetName(display_name.c_str());
        histogram->SetLineColor(plot_options.color);
        histogram->SetLineWidth(plot_options.line_width);
    }

private:
    std::vector< std::shared_ptr<TFile> > source_files;
    std::vector<std::string> display_names;
    std::vector<PlotOptions> plot_options_vector;
};

template<typename Histogram, typename ValueType=Double_t>
class SimpleHistogramSource : public HistogramSource<Histogram, ValueType, Histogram> {
protected:
    virtual Histogram* Convert(Histogram* original_histogram) const
    {
        return root_ext::CloneObject(*original_histogram);
    }
};

} // namespace root_ext

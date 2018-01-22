/*! Definition of classes that contain draw options.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/PropertyConfigReader.h"
#include "AnalysisTools/Core/include/NumericPrimitives.h"
#include "PlotPrimitives.h"

namespace root_ext {
namespace draw_options {

using Item = ::analysis::PropertyConfigReader::Item;
using ItemCollection = ::analysis::PropertyConfigReader::ItemCollection;
using Size = ::root_ext::Size<float, 2>;
using SizeI = ::root_ext::Size<int, 2>;
using Point = ::root_ext::Point<float, 2, false>;
using PointI = ::root_ext::Point<int, 2, false>;
using MarginBox = ::root_ext::MarginBox<float>;
using Box = ::root_ext::Box<float>;
using Angle = ::analysis::Angle<2>;
using Flag2D = ::root_ext::Point<bool, 2, false>;

#define READ(name) opt.Read(#name, name);
#define READ_VAR(r, opt, name) READ(name)
#define READ_ALL(...) BOOST_PP_SEQ_FOR_EACH(READ_VAR, opt, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

struct Page {
    SizeI canvas_size{600, 600};
    Box main_pad{0, 0, 1, 1};
    MarginBox margins{.1f, .1f, .1f, .1f};
    Size paper_size{20.,20.};
    Color canvas_color{kWhite};
    short canvas_border_size{10};
    short canvas_border_mode{0};
    int palette{1}, opt_stat{0};
    float end_error_size{2};
    Flag2D grid_xy{false, false}, tick_xy{true, true};
    Point tick_length_xy{.03f, .03f};
    PointI n_div_xy{510, 510};
    Point axis_title_sizes{.005f, .005f}, axis_title_offsets{1.f, 1.f},
          axis_label_sizes{.04f, .04f}, axis_label_offsets{.015f, .005f};

    std::string x_title, y_title;
    bool divide_by_bin_width{false};
    bool log_x{false}, log_y{false};
    double y_min_sf{1}, y_max_sf{1.2}, y_min_log{0.001};
    boost::optional<double> y_min;

    bool draw_ratio{true};
    std::string ratio_y_title{"Ratio"};
    float ratio_y_title_size{.005f}, ratio_y_title_offset{1.f}, ratio_y_label_size{.04f}, ratio_y_label_offset{.005f};
    int ratio_n_div_y{510};
    bool ratio_log_y{false};
    double max_ratio{-1}, allowed_ratio_margin{0.2}, ratio_y_min_sf{0.9}, ratio_y_max_sf{1.1}, ratio_y_min_log{0.001};
    float ratio_pad_size{.1f}, ratio_pad_spacing{.01f};

    double zero_threshold{-std::numeric_limits<double>::infinity()};
    bool blind{false};

    std::string legend_opt;
    std::vector<std::string> text_boxes_opt;

    Page() {}

    explicit Page(const Item& opt)
    {
        READ_ALL(canvas_size, main_pad, margins, paper_size, canvas_color, canvas_border_size, canvas_border_mode,
                 palette, opt_stat, end_error_size, grid_xy, tick_xy, tick_length_xy, n_div_xy, axis_title_sizes,
                 axis_title_offsets, axis_label_sizes, axis_label_offsets, x_title, y_title, divide_by_bin_width, log_x,
                 log_y, y_min_sf, y_max_sf, y_min_log, draw_ratio, ratio_y_title, ratio_y_title_size,
                 ratio_y_title_offset, ratio_y_label_size, ratio_y_label_offset, ratio_n_div_y, ratio_log_y, max_ratio,
                 allowed_ratio_margin, ratio_y_min_sf, ratio_y_max_sf, ratio_y_min_log, ratio_pad_size,
                 ratio_pad_spacing, zero_threshold, blind)

        opt.Read("legend", legend_opt);
        std::string text_boxes_str;
        opt.Read("text_boxes", text_boxes_str);
        text_boxes_opt = ::analysis::SplitValueList(text_boxes_str, false, ", \t", true);
        if(opt.Has("y_min"))
            y_min = opt.Get<double>("y_min");
    }

    Box GetRatioPadBox() const
    {
        const float left_bottom_x = main_pad.left_bottom_x();
        const float right_top_x = main_pad.right_top_x();
        const float right_top_y = main_pad.left_bottom_y();
        const float left_bottom_y = right_top_y - ratio_pad_size;
        return Box(left_bottom_x, left_bottom_y, right_top_x, right_top_y);
    }

    float GetRatioPadSizeSF() const
    {
        return (main_pad.right_top_y() - main_pad.left_bottom_y()) / ratio_pad_size;
    }
};

struct PositionedElement {
    Point pos{.5f, .5f};
    std::string pos_ref;

    PositionedElement() {}
    PositionedElement(const PositionedElement&) = default;
    PositionedElement& operator=(const PositionedElement&) = default;
    virtual ~PositionedElement() {}

    PositionedElement(const Item& opt)
    {
        READ_ALL(pos, pos_ref);
    }
};

struct Legend : PositionedElement {
    Size size{.25f, .25f};
    Color fill_color{kWhite};
    short fill_style{0};
    int border_size{0};
    float text_size{.026f};
    Font font{42};

    Legend() {}
    explicit Legend(const Item& opt) : PositionedElement(opt)
    {
        READ_ALL(size, fill_color, fill_style, border_size, text_size, font)
    }
};

struct Text : PositionedElement {
    std::vector<std::string> text;
    float text_size{.2f};
    float line_spacing{0.3f};
    Angle angle{0};
    Font font;
    TextAlign align{TextAlign::LeftTop};
    Color color{kBlack};

    Text() {}
    explicit Text(const Item& opt) : PositionedElement(opt)
    {
        READ_ALL(text_size, line_spacing, angle, font, align, color);
        std::string text_str;
        opt.Read("text", text_str);
        SetText(text_str);
    }

    void SetText(std::string text_str)
    {
        boost::replace_all(text_str, "\\n", "\n");
        text = ::analysis::SplitValueList(text_str, true, "\n", false);
    }
};

struct Histogram {
    short fill_style{0};
    Color fill_color{kWhite};

    short line_style{2}, line_width{2};
    Color line_color{kBlack};

    short marker_style{20};
    float marker_size{.0f};
    Color marker_color{kBlack};

    std::string legend_title, legend_style{"f"};
    std::string draw_opt{"HIST"}, unc_hist;

    bool blind{false};

    Histogram() {}
    explicit Histogram(const Item& opt)
    {
        READ_ALL(fill_style, fill_color, line_style, line_width, line_color, marker_style, marker_size, marker_color,
                 legend_title, legend_style, draw_opt, unc_hist, blind);
    }

    bool DrawUnc() const { return !unc_hist.empty(); }
};


#undef READ_ALL
#undef READ_VAR
#undef READ

} // namespace draw_options
} // namespace root_ext

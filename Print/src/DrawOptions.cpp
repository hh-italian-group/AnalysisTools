/*! Definition of classes that contain draw options.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Print/include/DrawOptions.h"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

namespace root_ext {
namespace draw_options {

#define READ(name) opt.Read(#name, name);
#define READ_VAR(r, opt, name) READ(name)
#define READ_ALL(...) BOOST_PP_SEQ_FOR_EACH(READ_VAR, opt, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

Page::Page(const Item& opt)
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

Box Page::GetRatioPadBox() const
{
    const float left_bottom_x = main_pad.left_bottom_x();
    const float right_top_x = main_pad.right_top_x();
    const float right_top_y = main_pad.left_bottom_y();
    const float left_bottom_y = right_top_y - ratio_pad_size;
    return Box(left_bottom_x, left_bottom_y, right_top_x, right_top_y);
}

float Page::GetRatioPadSizeSF() const
{
    return (main_pad.right_top_y() - main_pad.left_bottom_y()) / ratio_pad_size;
}

PositionedElement::PositionedElement(const Item& opt)
{
    READ_ALL(pos, pos_ref);
}

Legend::Legend(const Item& opt) : PositionedElement(opt)
{
    READ_ALL(size, fill_color, fill_style, border_size, text_size, font)
}

Text::Text(const Item& opt) : PositionedElement(opt)
{
    READ_ALL(text_size, line_spacing, angle, font, align, color);
    std::string text_str;
    opt.Read("text", text_str);
    SetText(text_str);
}

void Text::SetText(std::string text_str)
{
    boost::replace_all(text_str, "\\n", "\n");
    text = ::analysis::SplitValueList(text_str, true, "\n", false);
}

Histogram::Histogram(const Item& opt)
{
    READ_ALL(fill_style, fill_color, line_style, line_width, line_color, marker_style, marker_size, marker_color,
             legend_title, legend_style, draw_opt, unc_hist, blind, apply_syst_unc);
}

bool Histogram::DrawUnc() const { return !unc_hist.empty(); }

#undef READ_ALL
#undef READ_VAR
#undef READ

} // namespace draw_options
} // namespace root_ext

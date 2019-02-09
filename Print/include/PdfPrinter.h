/*! Print ROOT histograms to PDF.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include "DrawOptions.h"
#include "PlotDescriptor.h"

namespace root_ext {
class PdfPrinter {
public:
    using exception = ::analysis::exception;
    using PageOptions = draw_options::Page;
    using LegendOptions = draw_options::Legend;
    using LabelOptions = draw_options::Text;
    using PosElem = draw_options::PositionedElement;
    using PosElemMap = std::map<std::string, const PosElem*>;
    using Point = draw_options::Point;
    using Size = draw_options::Size;

    PdfPrinter(const std::string& _output_file_name, const draw_options::ItemCollection& opt_items,
               PageOptions& _page_opt);

    LabelOptions& GetLabelOptions(const std::string& name);
    void Print(const std::string& title, PlotDescriptor& desc, bool is_last);

private:
    Point GetAbsolutePosition(const std::string& name) const;
    Point GetAbsolutePosition(const std::string& name, std::set<std::string>& previous_names) const;
    void DrawLabel(const std::string& label_name, const LabelOptions& label_opt,
                   std::vector<std::shared_ptr<TObject>>& plot_items) const;

private:
    PageOptions page_opt;
    boost::optional<LegendOptions> legend_opt;
    std::map<std::string, LabelOptions> label_opts;
    std::shared_ptr<TCanvas> canvas;
    std::string output_file_name;
    bool has_first_page{false}, has_last_page{false};
    PosElemMap positioned_elements;
    Point left_top_origin, right_top_origin, left_bottom_origin, right_bottom_origin;
    Size inner_size;
};

} // namespace root_ext

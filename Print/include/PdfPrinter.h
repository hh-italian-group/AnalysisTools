/*! Print ROOT histograms to PDF.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <TROOT.h>
#include <TStyle.h>
#include <Rtypes.h>
#include <TError.h>
#include <TLatex.h>
#include <TFrame.h>

#include "DrawOptions.h"
#include "RootPrintTools.h"

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
               PageOptions& _page_opt) :
        page_opt(_page_opt), output_file_name(_output_file_name)
    {
        canvas = plotting::NewCanvas(page_opt.canvas_size);

        gStyle->SetPaperSize(page_opt.paper_size.x(), page_opt.paper_size.y());
        gStyle->SetPalette(page_opt.palette);
        gStyle->SetEndErrorSize(page_opt.end_error_size);
        gStyle->SetPadGridX(page_opt.grid_xy.x());
        gStyle->SetPadGridY(page_opt.grid_xy.y());
        gStyle->SetPadTickX(page_opt.tick_xy.x());
        gStyle->SetPadTickY(page_opt.tick_xy.y());
        gStyle->SetTickLength(page_opt.tick_length_xy.x(), "X");
        gStyle->SetTickLength(page_opt.tick_length_xy.y(), "Y");
        gStyle->SetNdivisions(page_opt.n_div_xy.x(), "X");
        gStyle->SetNdivisions(page_opt.n_div_xy.y(), "Y");
        gStyle->SetOptStat(page_opt.opt_stat);

        canvas->SetFillColor(page_opt.canvas_color.GetColor_t());
        canvas->SetBorderSize(page_opt.canvas_border_size);
        canvas->SetBorderMode(page_opt.canvas_border_mode);

        left_top_origin = Point(page_opt.main_pad.left_bottom_x() + page_opt.margins.left(),
                                page_opt.main_pad.right_top_y() - page_opt.margins.top());
        right_top_origin = Point(page_opt.main_pad.right_top_x() - page_opt.margins.right(),
                                 page_opt.main_pad.right_top_y() - page_opt.margins.top());
        left_bottom_origin = Point(page_opt.main_pad.left_bottom_x() + page_opt.margins.left(),
                                   page_opt.main_pad.left_bottom_y() + page_opt.margins.bottom());
        right_bottom_origin = Point(page_opt.main_pad.right_top_x() - page_opt.margins.right(),
                                    page_opt.main_pad.left_bottom_y() + page_opt.margins.bottom());
        inner_size = Size(page_opt.main_pad.right_top_x() - page_opt.main_pad.left_bottom_x()
                          - page_opt.margins.left() - page_opt.margins.right(),
                          page_opt.main_pad.right_top_y() - page_opt.main_pad.left_bottom_y()
                          - page_opt.margins.bottom() - page_opt.margins.top());


        if(page_opt.legend_opt.size()) {
            if(!opt_items.count(page_opt.legend_opt))
                throw exception("Legend options '%1%' not found.") % page_opt.legend_opt;
            legend_opt = LegendOptions(opt_items.at(page_opt.legend_opt));
            positioned_elements[page_opt.legend_opt] = &(*legend_opt);
        }

        for(const auto& label_opt_name : page_opt.text_boxes_opt) {
            if(!opt_items.count(label_opt_name))
                throw exception("Text label options '%1%' not found.") % label_opt_name;
            label_opts[label_opt_name] = LabelOptions(opt_items.at(label_opt_name));
            positioned_elements[label_opt_name] = &label_opts.at(label_opt_name);
        }
    }

    LabelOptions& GetLabelOptions(const std::string& name)
    {
        if(!label_opts.count(name))
            throw exception("Text label '%1%' not found.") % name;
        return label_opts.at(name);
    }

    template<typename PlotDescriptor>
    void Print(const std::string& title, PlotDescriptor& desc, bool is_last)
    {
        if(!desc.HasPrintableContent())
            return;
        if(is_last && has_last_page)
            throw exception("Last page has already been printed.");

        canvas->SetTitle(title.c_str());
        canvas->cd();

        {
            auto main_pad = plotting::NewPad(page_opt.main_pad);
            std::shared_ptr<TPad> ratio_pad;
            if(page_opt.draw_ratio) {
                canvas->cd();
                ratio_pad = plotting::NewPad(page_opt.GetRatioPadBox());
            }
            plotting::SetMargins(*main_pad, page_opt.margins, ratio_pad.get(), page_opt.GetRatioPadSizeSF(),
                                 page_opt.ratio_pad_spacing);

            std::vector<std::shared_ptr<TObject>> plot_items;
            std::shared_ptr<TLegend> legend;
            if(legend_opt) {
                canvas->cd();
                const auto legend_pos = GetAbsolutePosition(page_opt.legend_opt);
                legend = std::make_shared<TLegend>(legend_pos.x(), legend_pos.y(),
                                                   legend_pos.x() + legend_opt->size.x() * inner_size.x(),
                                                   legend_pos.y() + legend_opt->size.y() * inner_size.y());
                legend->SetFillColor(legend_opt->fill_color.GetColor_t());
                legend->SetFillStyle(legend_opt->fill_style);
                legend->SetBorderSize(legend_opt->border_size);
                legend->SetTextSize(legend_opt->text_size);
                legend->SetTextFont(legend_opt->font.code());
            }

            if(ratio_pad) {
                ratio_pad->Draw();
                ratio_pad->SetTitle("");
            }

            main_pad->Draw();
            main_pad->cd();
            main_pad->SetTitle("");

            desc.Draw(main_pad, ratio_pad, legend, plot_items);
            canvas->cd();
            if(legend)
                legend->Draw();
            for(const auto& label_opt_entry : label_opts)
                DrawLabel(label_opt_entry.first, label_opt_entry.second, plot_items);

            canvas->Draw();

            std::ostringstream print_options, output_name;
            print_options << "Title:" << title;
            output_name << output_file_name;
            if(!has_first_page && !is_last)
                output_name << "(";
            else if(has_first_page && is_last)
                output_name << ")";

            WarningSuppressor ws(kWarning);
            canvas->Print(output_name.str().c_str(), print_options.str().c_str());
            has_first_page = true;
            has_last_page = is_last;
        }

        canvas->Clear();
    }

private:
    Point GetAbsolutePosition(const std::string& name) const
    {
        std::set<std::string> previous_names;
        return GetAbsolutePosition(name, previous_names);
    }

    Point GetAbsolutePosition(const std::string& name, std::set<std::string>& previous_names) const
    {
        if(!positioned_elements.count(name))
            throw exception("Element with name '%1%' not found.") % name;
        if(previous_names.count(name))
            throw exception("A loop is found in the elemnt position definition.");
        const auto& elem = *positioned_elements.at(name);
        if(!elem.pos_ref.size())
            return elem.pos;
        if(elem.pos_ref == "inner_left_top")
            return left_top_origin + elem.pos * inner_size.flip_y();
        if(elem.pos_ref == "inner_right_top")
            return right_top_origin - elem.pos * inner_size;
        if(elem.pos_ref == "inner_left_bottom")
            return left_bottom_origin + elem.pos * inner_size;
        if(elem.pos_ref == "inner_right_bottom")
            return right_bottom_origin + elem.pos * inner_size.flip_x();
        if(positioned_elements.count(elem.pos_ref)) {
            previous_names.insert(name);
            const auto& ref_position = GetAbsolutePosition(elem.pos_ref, previous_names);
            return ref_position + elem.pos * inner_size;
        }
        throw analysis::exception("Unknown position reference '%1%'.") % elem.pos_ref;
    }

    void DrawLabel(const std::string& label_name, const LabelOptions& label_opt,
                   std::vector<std::shared_ptr<TObject>>& plot_items) const
    {
        auto pos = GetAbsolutePosition(label_name);
        for(const auto& line : label_opt.text) {
            std::shared_ptr<TLatex> latex(new TLatex(pos.x(), pos.y(), line.c_str()));
            latex->SetNDC();
            latex->SetTextSize(label_opt.text_size);
            latex->SetTextFont(label_opt.font.code());
            latex->SetTextAlign(static_cast<Short_t>(label_opt.align));
            latex->SetTextAngle(static_cast<float>(label_opt.angle.value_degrees()));
            latex->SetTextColor(label_opt.color.GetColor_t());
            latex->Draw("same");
            plot_items.push_back(latex);
            const float alpha = static_cast<float>(label_opt.angle.value());
            const float cos = std::cos(alpha), sin = std::sin(alpha);
            const Point shift(0, -(1 + label_opt.line_spacing) * static_cast<float>(latex->GetYsize()));
            pos = pos + Point(shift.x() * cos + shift.y() * sin, - shift.x() * sin + shift.y() * cos);
        }
    }

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

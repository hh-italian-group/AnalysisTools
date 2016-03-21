/*! Print ROOT histograms to PDF.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <sstream>
#include <stdexcept>
#include <iostream>

#include <TROOT.h>
#include <TStyle.h>
#include <Rtypes.h>
#include <TError.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <THStack.h>

#include "RootPrintSource.h"
#include "TdrStyle.h"
//#include "StackedPlotDescriptor.h"

namespace root_ext {

class PdfPrinter {
public:
    PdfPrinter(const std::string& _output_file_name, bool _verbose = false)
        : canvas(new TCanvas("","",50, 50, 700, 700)), output_file_name(_output_file_name), n_pages(0),
          verbose(_verbose)
    {
//        int W = 700;
//        int H = 700;

//        int H_ref = 700;
//        int W_ref = 700;

//        float T = 0.08*H_ref;
//        float B = 0.12*H_ref;
//        float L = 0.12*W_ref;
//        float R = 0.04*W_ref;

        canvas->SetFillColor(0);
        canvas->SetBorderMode(0);
        canvas->SetFrameFillStyle(0);
        canvas->SetFrameLineColor(kWhite);
        canvas->SetFrameBorderMode(0);
//        canvas->SetLeftMargin( L/W );
//        canvas->SetRightMargin( R/W );
//        canvas->SetTopMargin( T/H );
//        canvas->SetBottomMargin( B/H );
//        canvas->SetTickx(0);
//        canvas->SetTicky(0);

        const Int_t old_gErrorIgnoreLevel = gErrorIgnoreLevel;
        if(!verbose)
            gErrorIgnoreLevel = kWarning;
        canvas->Print((output_file_name + "[").c_str());
        gErrorIgnoreLevel = old_gErrorIgnoreLevel;
    }

    template<typename Source>
    void Print(const Page& page, const Source& source)
    {
        gROOT->SetStyle(page.layout.global_style.c_str());
        gStyle->SetOptStat(page.layout.stat_options);
        gStyle->SetOptFit(page.layout.fit_options);
        canvas->cd();

        canvas->SetTitle(page.title.c_str());
        if(page.layout.has_title) {
            TPaveLabel *title = Adapter::NewPaveLabel(page.layout.title_box, page.title);
            title->SetTextFont(page.layout.title_font);
            title->Draw();
        }

        Page::RegionCollection page_regions = page.Regions();
        for(Page::RegionCollection::const_iterator iter = page_regions.begin(); iter != page_regions.end(); ++iter)
        {
            canvas->cd();
            DrawHistograms(*(*iter), source);
        }

        canvas->Draw();
        std::ostringstream print_options;
        print_options << "Title:" << page.title;
        canvas->Print(output_file_name.c_str(), print_options.str().c_str());
        ++n_pages;
    }

//    void PrintStack(analysis::StackedPlotDescriptor& stackDescriptor)
//    {
//        if(!stackDescriptor.NeedDraw())
//            return;
//        canvas->cd();
//        canvas->SetTitle(stackDescriptor.GetTitle().c_str());
//        canvas->Clear();
//        stackDescriptor.Draw(*canvas);
//        canvas->Draw();
//        std::ostringstream print_options;
//        print_options << "Title:" << stackDescriptor.GetTitle();
//        const Int_t old_gErrorIgnoreLevel = gErrorIgnoreLevel;
//        gErrorIgnoreLevel = kWarning;
//        canvas->Print(output_file_name.c_str(), print_options.str().c_str());
//        gErrorIgnoreLevel = old_gErrorIgnoreLevel;
//        ++n_pages;
//    }

    ~PdfPrinter()
    {
        const Int_t old_gErrorIgnoreLevel = gErrorIgnoreLevel;
        gErrorIgnoreLevel = kWarning;
        if (n_pages > 1){
            canvas->Clear();
            canvas->Print(output_file_name.c_str());
        }
        canvas->Print((output_file_name+"]").c_str());
        gErrorIgnoreLevel = old_gErrorIgnoreLevel;
        if(verbose)
            std::cout << "Info in <TCanvas::Print>: pdf file " << output_file_name << " has been closed" << std::endl;
    }

private:
    template<typename Source>
    void DrawHistograms(const PageSide& page_side, const Source& source)
    {
        using Plotter = root_ext::HistogramPlotter<typename Source::Histogram, typename Source::ValueType>;
        using Fitter = root_ext::HistogramFitter<typename Source::Histogram, typename Source::ValueType>;

        TPad* stat_pad = 0;
        if(page_side.layout.has_stat_pad) {
            stat_pad = Adapter::NewPad(page_side.layout.stat_pad);
            stat_pad->Draw();
        }

        TPad *pad = Adapter::NewPad(page_side.layout.main_pad);
        if(page_side.use_log_scaleX)
            pad->SetLogx();
        if(page_side.use_log_scaleY)
            pad->SetLogy();
        pad->Draw();
        pad->cd();

        Plotter plotter(page_side.histogram_title, page_side.axis_titleX, page_side.axis_titleY);
        for(unsigned n = 0; n < source.Size(); ++n)
        {
            const typename Plotter::Entry entry = source.Get(n, page_side.histogram_name);
            plotter.Add(entry);
        }

        Fitter::SetRanges(plotter.Histograms(), page_side.fit_range_x, page_side.fit_range_y, page_side.xRange,
                          page_side.yRange, page_side.use_log_scaleY);
        plotter.Superpose(pad, stat_pad, page_side.layout.has_legend, page_side.layout.legend_pad,
                          page_side.draw_options);
    }

private:
    std::shared_ptr<TCanvas> canvas;
    std::string output_file_name;
    size_t n_pages;
    bool verbose;
};

} // namespace root_ext

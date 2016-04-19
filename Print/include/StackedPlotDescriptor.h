/*! Code to produce stacked plots using CMS preliminary style.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <iostream>
#include <iomanip>
#include <memory>

#include <TH1.h>
#include <THStack.h>
#include <TLine.h>
#include <TFrame.h>
#include <TStyle.h>
#include <TROOT.h>

#include "RootPrintSource.h"
#include "TdrStyle.h"
#include "CMS_lumi.h"

#include "SmartHistogram.h"
#include "HttStyles.h"

namespace analysis {

class StackedPlotDescriptor {
public:
    typedef root_ext::SmartHistogram<TH1D> Histogram;
    typedef std::shared_ptr<Histogram> hist_ptr;
    typedef std::vector<hist_ptr> hist_ptr_vector;

    StackedPlotDescriptor(const std::string& page_title, bool draw_title, const std::string& channelNameLatex,
                          const std::string& _categoryName, bool _draw_ratio, bool _drawBKGerrors)
        : text(new TPaveText(0.18, 0.95, 0.65, 0.99, "NDC")), categoryText(new TPaveText(0.21, 0.71, 0.35, 0.75, "NDC")),
          channelText(new TPaveText(0.2, 0.85, 0.25, 0.89, "NDC")), channelName(channelNameLatex),
          categoryName(_categoryName), draw_ratio(_draw_ratio), drawBKGerrors(_drawBKGerrors)
    {


        page.side.fit_range_x = false;
        page.title = page_title;
        page.layout.has_title = draw_title;
        if (draw_ratio){
            if (page.layout.has_title) {
                page.side.layout.main_pad = root_ext::Box(0.02,0.19,0.95,0.94);
                page.side.layout.ratio_pad = root_ext::Box(0.02,0.02,1,0.28);
            } else {
                page.side.layout.main_pad = root_ext::Box(0.02,0.21,1,1);
                page.side.layout.ratio_pad = root_ext::Box(0.02,0.02,1,0.30);
            }
        }
        else {
            if (page.layout.has_title) {
                page.side.layout.main_pad = root_ext::Box(0.02,0.02,0.95,0.94);
            } else {
                page.side.layout.main_pad = root_ext::Box(0.,0., 1, 1);
            }
        }

        if (draw_ratio){
            legend = std::shared_ptr<TLegend>(new TLegend (0.6, 0.7, 0.85, 0.90)); // 0.6, 0.55, 0.85, 0.90
        }
        else {
            legend = std::shared_ptr<TLegend>(new TLegend (0.52, 0.60, 0.89, 0.90));
        }

        legend->SetFillColor(0);
        legend->SetTextSize(0.026);
        legend->SetTextFont(42);
        legend->SetFillStyle (0);
        legend->SetFillColor (0);
        legend->SetBorderSize(0);

        text->SetTextSize(0.035);
        text->SetTextFont(62);
        text->SetFillColor(0);
        text->SetBorderSize(0);
        text->SetMargin(0.01);
        text->SetTextAlign(12); // align left
        std::ostringstream ss_text;
        ss_text << "H#rightarrowhh#rightarrowbb#tau#tau " ;
        text->AddText(0.01,0.05, ss_text.str().c_str());


        //category
        categoryText->SetTextSize(0.035);
        categoryText->SetTextFont(62);
        categoryText->SetFillColor(0);
        categoryText->SetBorderSize(0);
        categoryText->SetMargin(0.01);
        categoryText->SetTextAlign(12); // align left
        std::ostringstream ss_text_2;
        ss_text_2 << channelName << ", " << categoryName;
        categoryText->AddText(ss_text_2.str().c_str());

//        channelText->SetTextSize(0.08); //0.05
//        channelText->SetTextFont(62);
//        channelText->SetFillColor(0);
//        channelText->SetBorderSize(0);
//        channelText->SetMargin(0.01);
//        channelText->SetTextAlign(12); // align left
//        channelText->SetX1(0.21);
//        channelText->SetY1(0.82);
//        channelText->AddText(channelName.c_str());
    }

    const std::string& GetTitle() const { return page.title; }

    void AddBackgroundHistogram(const Histogram& original_histogram, const std::string& legend_title, Color_t color)
    {
        hist_ptr histogram = PrepareHistogram(original_histogram);
        histogram->SetFillColor(color);
        histogram->SetFillStyle(1001);
        histogram->SetLegendTitle(legend_title);
        background_histograms.push_back(histogram);
        //stack->Add(histogram.get());

        if(!sum_backgound_histogram){
            sum_backgound_histogram = hist_ptr(new Histogram(*histogram));
            sum_backgound_histogram->SetLegendTitle("Bkg. uncertainty");
        }
        else
            sum_backgound_histogram->Add(histogram.get());
    }

    void AddSignalHistogram(const Histogram& original_signal, const std::string& legend_title, Color_t color,
                            unsigned scale_factor)
    {
        hist_ptr histogram = PrepareHistogram(original_signal);
        //Reb
        htt_style::SetStyle();
        histogram->SetFillStyle(1001);
        histogram->SetLineStyle(2);
        histogram->SetFillColor(0);
        histogram->SetLineColor(kBlue+3);
        histogram->SetLineWidth(3);
        //ours
        //histogram->SetLineColor(color);
        //histogram->SetLineColor(4);
        //histogram->SetLineStyle(11); //2
        //histogram->SetLineWidth(2); //3
        histogram->Scale(scale_factor);
        signal_histograms.push_back(histogram);
        std::ostringstream ss;
        if(scale_factor != 1)
            ss << scale_factor << "x ";
        ss << legend_title;
        histogram->SetLegendTitle(ss.str());
    }

    void AddDataHistogram(const Histogram& original_data, const std::string& legend_title,
                          bool blind, const std::vector< std::pair<double, double> >& blind_regions)
    {
        if(data_histogram)
            throw std::runtime_error("Only one data histogram per stack is supported.");

        data_histogram = PrepareHistogram(original_data);
        data_histogram->SetLegendTitle(legend_title);

        if(blind)
            BlindHistogram(data_histogram, blind_regions);
    }

    bool NeedDraw() const
    {
        return background_histograms.size() || data_histogram || signal_histograms.size();
    }

    void Draw(TCanvas& canvas)
    {
        cms_tdr::setTDRStyle();
//        htt_style::SetStyle();
//        gStyle->SetLineStyleString(11,"20 10");

        if(page.layout.has_title) {
            TPaveLabel *title = root_ext::Adapter::NewPaveLabel(page.layout.title_box, page.title);
            title->SetTextFont(page.layout.title_font);
            title->Draw();
        }
        main_pad = std::shared_ptr<TPad>(root_ext::Adapter::NewPad(page.side.layout.main_pad));
        if(page.side.use_log_scaleX)
            main_pad->SetLogx();
        if(page.side.use_log_scaleY)
            main_pad->SetLogy();

        int W = 700;
        int H = 700;

        int H_ref = 700;
        int W_ref = 700;

        float T = 0.08*H_ref;
        float B = 0.14*H_ref;
        float L = 0.18*W_ref;
        float R = 0.05*W_ref;

//        main_pad->SetFillColor(0);
//        main_pad->SetBorderMode(0);
//        main_pad->SetFrameFillStyle(0);
//        main_pad->SetFrameBorderMode(0);
        main_pad->SetLeftMargin( L/W );
        main_pad->SetRightMargin( R/W );
        main_pad->SetTopMargin( T/H );
        main_pad->SetBottomMargin( B/H );
        main_pad->SetTickx(1);
        main_pad->SetTicky(1);

        main_pad->Draw();
        main_pad->cd();

        if (data_histogram)
            legend->AddEntry(data_histogram.get(), data_histogram->GetLegendTitle().c_str(), "PLE");
        if (drawBKGerrors && sum_backgound_histogram)
            legend->AddEntry(sum_backgound_histogram.get(),sum_backgound_histogram->GetLegendTitle().c_str(), "f");
        for(const hist_ptr& signal : signal_histograms)
            legend->AddEntry(signal.get(), signal->GetLegendTitle().c_str(), "F");
        for(const hist_ptr& background : background_histograms)
            legend->AddEntry(background.get(), background->GetLegendTitle().c_str(), "F");



        if (background_histograms.size()) {
            const auto& bkg_hist = background_histograms.front();
            stack = std::shared_ptr<THStack>(new THStack(bkg_hist->GetName(), bkg_hist->GetTitle()));

            for (auto iter = background_histograms.rbegin(); iter != background_histograms.rend(); ++iter){
                stack->Add(iter->get());
            }
            stack->Draw("HIST");

            Double_t maxY = stack->GetMaximum();
            if (data_histogram){
                const Int_t maxBin = data_histogram->GetMaximumBin();
                const Double_t maxData = data_histogram->GetBinContent(maxBin) + data_histogram->GetBinError(maxBin);
                maxY = std::max(maxY, maxData);
            }

            for (const hist_ptr& signal : signal_histograms)
                maxY = std::max(maxY,signal->GetMaximum());

            stack->SetMaximum(maxY * bkg_hist->MaxYDrawScaleFactor());

            const Double_t minY = page.side.use_log_scaleY ? 1 : 0;
            stack->SetMinimum(minY);


            if (draw_ratio){
                stack->GetXaxis()->SetTitle("");
                stack->GetXaxis()->SetLabelColor(kWhite);

            }
            else {
                stack->GetXaxis()->SetTitle(page.side.axis_titleX.c_str());
                stack->GetXaxis()->SetTitleOffset(1.00); //1.05
//                stack->GetXaxis()->SetTitleSize(0.055); //0.04
                stack->GetXaxis()->SetLabelSize(0.04);
                stack->GetXaxis()->SetLabelOffset(0.015);
//                stack->GetXaxis()->SetTitleFont(42); //62
            }

//            stack->GetYaxis()->SetTitleSize(0.055); //0.05
            stack->GetYaxis()->SetTitleOffset(1.4); //1.45
            stack->GetYaxis()->SetLabelSize(0.04);
            stack->GetYaxis()->SetTitle(page.side.axis_titleY.c_str());
//            stack->GetYaxis()->SetTitleFont(42); //62

            if (drawBKGerrors){
                sum_backgound_histogram->SetMarkerSize(0);
                //new
                // int new_idx = root_ext::CreateTransparentColor(12,0.5);
                // sum_backgound_histogram->SetFillColor(new_idx);
                // sum_backgound_histogram->SetFillStyle(3001);
                // sum_backgound_histogram->SetLineWidth(0);
                // sum_backgound_histogram->Draw("e2same");
//                end new

                  //old style for bkg uncertainties
               sum_backgound_histogram->SetFillColor(13);
               sum_backgound_histogram->SetFillStyle(3013);
               sum_backgound_histogram->SetLineWidth(0);
               sum_backgound_histogram->Draw("e2same");
            }
        }

        for(const hist_ptr& signal : signal_histograms)
            signal->Draw("SAME HIST");

        if(data_histogram) {
            data_histogram->SetMarkerColor(1);
            data_histogram->SetLineColor(1);
            data_histogram->SetFillColor(1);
            data_histogram->SetFillStyle(0);
            data_histogram->SetLineWidth(2);


            data_histogram->SetMarkerStyle(20);
            data_histogram->SetMarkerSize(1.1);
//            data_histogram->Draw("samepPE0X0");
            data_histogram->Draw("pesame");
        }

        text->Draw("same");
//        channelText->Draw("same");
        categoryText->Draw("same");

        cms_tdr::writeExtraText = true;
        cms_tdr::extraText = TString("Unpublished");
        cms_tdr::CMS_lumi(main_pad.get(), 4, 11); //Labels Definition


        legend->Draw("same");

        const std::string axis_titleX = page.side.axis_titleX;
        if (data_histogram && draw_ratio){
            ratio_pad = std::shared_ptr<TPad>(root_ext::Adapter::NewPad(page.side.layout.ratio_pad));
            ratio_pad->Draw();

            ratio_pad->cd();


            ratio_histogram = hist_ptr(new Histogram(*data_histogram));
            ratio_histogram->Divide(sum_backgound_histogram.get());

            ratio_histogram->GetYaxis()->SetRangeUser(0.5,1.5);
            ratio_histogram->GetYaxis()->SetNdivisions(505);
            ratio_histogram->GetYaxis()->SetLabelSize(0.11);
            ratio_histogram->GetYaxis()->SetTitleSize(0.14);
            ratio_histogram->GetYaxis()->SetTitleOffset(0.55);
            ratio_histogram->GetYaxis()->SetTitle("Obs/Bkg");
            ratio_histogram->GetYaxis()->SetTitleFont(62);
            ratio_histogram->GetXaxis()->SetNdivisions(510);
            ratio_histogram->GetXaxis()->SetTitle(axis_titleX.c_str());
            ratio_histogram->GetXaxis()->SetTitleSize(0.1);
            ratio_histogram->GetXaxis()->SetTitleOffset(0.98);
            ratio_histogram->GetXaxis()->SetTitleFont(62);
            //ratio_histogram->GetXaxis()->SetLabelColor(kBlack);
            ratio_histogram->GetXaxis()->SetLabelSize(0.1);
            ratio_histogram->SetMarkerStyle(20);
            ratio_histogram->SetMarkerColor(1);
            ratio_histogram->SetMarkerSize(1);

            ratio_histogram->Draw("E0P");

            TLine* line = new TLine();
            line->SetLineStyle(3);
            line->DrawLine(ratio_histogram->GetXaxis()->GetXmin(), 1, ratio_histogram->GetXaxis()->GetXmax(), 1);
            TLine* line1 = new TLine();
            line1->SetLineStyle(3);
            line1->DrawLine(ratio_histogram->GetXaxis()->GetXmin(), 1.2, ratio_histogram->GetXaxis()->GetXmax(), 1.2);
            TLine* line2 = new TLine();
            line2->SetLineStyle(3);
            line2->DrawLine(ratio_histogram->GetXaxis()->GetXmin(), 0.8, ratio_histogram->GetXaxis()->GetXmax(), 0.8);
            ratio_pad->SetTopMargin(0.04);
            ratio_pad->SetBottomMargin(0.3);
            ratio_pad->Update();
        }

        canvas.cd();
        main_pad->Draw();
        canvas.RedrawAxis();
        canvas.GetFrame()->Draw();

        canvas.cd();
        if (data_histogram && draw_ratio)
            ratio_pad->Draw();
    }

private:
    hist_ptr PrepareHistogram(const Histogram& original_histogram)
    {
        hist_ptr histogram(new Histogram(original_histogram));
        histogram->SetLineColor(root_ext::colorMapName.at("black"));
        histogram->SetLineWidth(1.);
        if (histogram->NeedToDivideByBinWidth())
            ReweightWithBinWidth(histogram);
        UpdateDrawInfo(histogram);
        return histogram;
    }

    static void ReweightWithBinWidth(hist_ptr histogram)
    {
        for(Int_t n = 1; n <= histogram->GetNbinsX(); ++n) {
            const double new_value = histogram->GetBinContent(n) / histogram->GetBinWidth(n);
            const double new_bin_error = histogram->GetBinError(n) / histogram->GetBinWidth(n);
            histogram->SetBinContent(n, new_value);
            histogram->SetBinError(n, new_bin_error);
        }
    }

    static void BlindHistogram(hist_ptr histogram, const std::vector< std::pair<double, double> >& blind_regions)
    {
        for(Int_t n = 1; n <= histogram->GetNbinsX(); ++n) {
            const double x = histogram->GetBinCenter(n);
            const auto blind_predicate = [&](const std::pair<double, double>& region) -> bool {
                return x > region.first && x < region.second;
            };

            const bool need_blind = std::any_of(blind_regions.begin(), blind_regions.end(), blind_predicate);
            histogram->SetBinContent(n, need_blind ? -1 : histogram->GetBinContent(n));
            histogram->SetBinError(n, need_blind ? -1 : histogram->GetBinError(n));
        }
    }

    void UpdateDrawInfo(hist_ptr hist)
    {
        page.side.use_log_scaleY = hist->UseLogY();
        page.side.axis_titleX = hist->GetXTitle();
        page.side.axis_titleY = hist->GetYTitle();
    }

private:
    root_ext::SingleSidedPage page;
    hist_ptr data_histogram;
    hist_ptr sum_backgound_histogram;
    hist_ptr ratio_histogram;
    hist_ptr_vector background_histograms;
    hist_ptr_vector signal_histograms;
    std::shared_ptr<THStack> stack;
    std::shared_ptr<TLegend> legend;
    std::shared_ptr<TPaveText> text;
    std::shared_ptr<TPaveText> categoryText;
    std::shared_ptr<TPaveText> channelText;
    std::shared_ptr<TPad> main_pad, ratio_pad;
    std::string channelName;
    std::string categoryName;
    bool draw_ratio;
    bool drawBKGerrors;
};

} // namespace analysis

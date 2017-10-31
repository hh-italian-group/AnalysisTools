/*! Test TRatioPlot printing.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TStyle.h>
#include <TCanvas.h>
#include <TRatioPlot.h>
#include <TF1.h>
//#include <TROOT.h>

#include "AnalysisTools/Run/include/program_main.h"

struct Arguments {
    REQ_ARG(std::string, output);
};


class RatioPlot_t {
public:
    RatioPlot_t(const Arguments& _args) : args(_args) {}

    void Run()
    {
        gROOT->SetBatch(true);
        gROOT->SetMustClean(true);
        gStyle->SetOptStat(0);
        auto c1 = new TCanvas("c1", "A ratio example");
        auto h1 = new TH1D("h1", "h1", 50, 0, 10);
        auto h2 = new TH1D("h2", "h2", 50, 0, 10);
        auto f1 = new TF1("f1", "exp(- x/[0] )");
        f1->SetParameter(0, 3);
        h1->FillRandom("f1", 1900);
        h2->FillRandom("f1", 2000);
        h1->Sumw2();
        h2->Scale(1.9 / 2.);
        h1->GetXaxis()->SetTitle("x");
        h1->GetYaxis()->SetTitle("y");
        auto rp = new TRatioPlot(h1, h2);
        c1->SetTicks(0, 1);
        rp->Draw();
        c1->Update();
        c1->Print(args.output().c_str());
    }

private:
    Arguments args;
};

PROGRAM_MAIN(RatioPlot_t, Arguments)

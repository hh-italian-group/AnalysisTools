/*! Print smart histograms with specified name superimposing several files.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TTree.h>

#include "AnalysisTools/Run/include/program_main.h"
#include "AnalysisTools/Core/include/RootExt.h"
#include "../include/RootPrintToPdf.h"

class MyHistogramSource : public root_ext::HistogramSource<TH1D, Double_t, TTree> {
public:
    MyHistogramSource(const root_ext::Range& _xRange, unsigned _nBins)
        : xRange(_xRange), nBins(_nBins) {}
protected:
    virtual TH1D* Convert(TTree* tree) const
    {
        std::ostringstream s_name;
        s_name << tree->GetName() << "_hist";
        const std::string name = s_name.str();
        const std::string command = "values>>+" + name;

        TH1D* histogram = new TH1D(name.c_str(), name.c_str(), static_cast<int>(nBins), xRange.min, xRange.max);
        tree->Draw(command.c_str(), "");
        return histogram;
    }

private:
    root_ext::Range xRange;
    unsigned nBins;
};

struct Arguments {
    REQ_ARG(std::string, outputFileName);
    REQ_ARG(std::string, histogramName);
    REQ_ARG(std::string, title);
    REQ_ARG(double, xMin);
    REQ_ARG(double, xMax);
    REQ_ARG(unsigned, nBins);
    REQ_ARG(bool, useLogX);
    REQ_ARG(bool, useLogY);
    REQ_ARG(std::vector<std::string>, inputs);
};

class Print_SmartHistogram {
public:
    using FileTagPair = std::pair<std::string, std::string>;
    using Printer = root_ext::PdfPrinter;

    Print_SmartHistogram(const Arguments& _args)
       : args(_args), printer(args.outputFileName()), xRange(args.xMin(), args.xMax()), source(xRange, args.nBins())
    {
        for(const std::string& inputName : args.inputs()) {
            const size_t split_index = inputName.find_first_of(':');
            const std::string fileName = inputName.substr(0, split_index);
            const std::string tagName = inputName.substr(split_index + 1);
            inputs.push_back(FileTagPair(fileName, tagName));
        }
        for(const FileTagPair& fileTag : inputs) {
            auto file = root_ext::OpenRootFile(fileTag.first);
            source.Add(fileTag.second, file);
        }
    }

    void Run()
    {
        page.side.use_log_scaleX = args.useLogX();
        page.side.use_log_scaleY = args.useLogY();
        page.side.xRange = xRange;
        page.side.fit_range_x = false;
        page.side.layout.has_legend = false;

        Print(args.histogramName(), args.title());
    }

private:
    void Print(const std::string& name, const std::string& title,
               std::string name_suffix = "", std::string title_suffix = "")
    {
        page.side.histogram_name = AddSuffix(name, name_suffix);
        page.title = page.side.histogram_title = AddSuffix(title, title_suffix, ". ");
        printer.Print(page, source);
    }

    static std::string AddSuffix(const std::string& name, const std::string& suffix, std::string separator = "_")
    {
        std::ostringstream full_name;
        full_name << name;
        if(suffix.size())
            full_name << separator << suffix;
        return full_name.str();
    }

private:
    Arguments args;
    std::vector<FileTagPair> inputs;
    root_ext::SingleSidedPage page;
    Printer printer;
    root_ext::Range xRange;
    MyHistogramSource source;
};

PROGRAM_MAIN(Print_SmartHistogram, Arguments)

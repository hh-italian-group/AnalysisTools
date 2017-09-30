/*! Print histogram for a tree branch with a specified name superimposing several files.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TTree.h>

#include "AnalysisTools/Run/include/program_main.h"
#include "AnalysisTools/Core/include/RootExt.h"
#include "../include/RootPrintToPdf.h"

class MyHistogramSource : public root_ext::HistogramSource<TH1D, Double_t, TTree> {
public:
    MyHistogramSource(const std::string& _branchName, const analysis::Range<double>& _xRange, unsigned _nBins)
        : branchName(_branchName), xRange(_xRange), nBins(_nBins) {}
protected:
    virtual TH1D* Convert(TTree* tree) const
    {
        const std::string name = branchName + "_hist";
        const std::string command = branchName + ">>+" + name;

        TH1D* histogram = new TH1D(name.c_str(), name.c_str(), static_cast<int>(nBins), xRange.min(), xRange.max());
        tree->Draw(command.c_str(), "");
        return histogram;
    }

private:
    std::string branchName;
    analysis::Range<double> xRange;
    unsigned nBins;
};

struct Arguments {
    REQ_ARG(std::string, outputFileName);
    REQ_ARG(std::string, treeName);
    REQ_ARG(std::string, branchName);
    REQ_ARG(std::string, title);
    REQ_ARG(double, xMin);
    REQ_ARG(double, xMax);
    REQ_ARG(unsigned, nBins);
    REQ_ARG(bool, useLogX);
    REQ_ARG(bool, useLogY);
    REQ_ARG(std::vector<std::string>, inputs);
};

class Print_TreeBranch {
public:
    using FileTagPair = std::pair<std::string, std::string>;
    using Printer = root_ext::PdfPrinter;

    Print_TreeBranch(const Arguments& _args)
       : args(_args), printer(args.outputFileName()), xRange(args.xMin(), args.xMax()),
         source(args.branchName(), xRange, args.nBins())
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
        page.side.axis_titleX = "P_{T}(#tau) [GeV]";
        page.side.axis_titleY = "N entries";

        Print(args.treeName(), args.title());
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
    analysis::Range<double> xRange;
    MyHistogramSource source;
};

PROGRAM_MAIN(Print_TreeBranch, Arguments)

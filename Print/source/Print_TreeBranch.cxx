/*! Print histogram for a tree branch with a specified name superimposing several files.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TTree.h>

#include "AnalysisTools/Core/include/RootExt.h"
#include "../include/RootPrintToPdf.h"

class MyHistogramSource : public root_ext::HistogramSource<TH1D, Double_t, TTree> {
public:
    MyHistogramSource(const std::string& _branchName, const root_ext::Range& _xRange, unsigned _nBins)
        : branchName(_branchName), xRange(_xRange), nBins(_nBins) {}
protected:
    virtual TH1D* Convert(TTree* tree) const
    {
        const std::string name = branchName + "_hist";
        const std::string command = branchName + ">>+" + name;

        TH1D* histogram = new TH1D(name.c_str(), name.c_str(), nBins, xRange.min, xRange.max);
        tree->Draw(command.c_str(), "");
        return histogram;
    }

private:
    std::string branchName;
    root_ext::Range xRange;
    unsigned nBins;
};

class Print_TreeBranch {
public:
    typedef std::pair< std::string, std::string > FileTagPair;
    typedef root_ext::PdfPrinter Printer;
//    typedef root_ext::SimpleHistogramSource<TH1D, Double_t> MyHistogramSource;

    template<typename ...Args>
    Print_TreeBranch(const std::string& outputFileName, const std::string& _treeName,
                         const std::string& _branchName, const std::string& _title, double _xMin, double _xMax,
                         unsigned _nBins,  bool _useLogX, bool _useLogY, const Args& ...args)
       : printer(outputFileName), treeName(_treeName), branchName(_branchName), title(_title),
         xRange(_xMin, _xMax), nBins(_nBins), useLogX(_useLogX), useLogY(_useLogY), source(branchName, xRange, nBins)
    {
        Initialize(args...);
        for(const FileTagPair& fileTag : inputs) {
            auto file = root_ext::OpenRootFile(fileTag.first);
            source.Add(fileTag.second, file);
        }
    }

    void Run()
    {
        page.side.use_log_scaleX = useLogX;
        page.side.use_log_scaleY = useLogY;
        page.side.xRange = xRange;
        page.side.fit_range_x = false;
        page.side.layout.has_legend = false;
        page.side.axis_titleX = "P_{T}(#tau) [GeV]";
        page.side.axis_titleY = "N entries";

        Print(treeName, title);
    }

private:
    template<typename ...Args>
    void Initialize(const std::string& inputName, const Args& ...args)
    {
        const size_t split_index = inputName.find_first_of(':');
        const std::string fileName = inputName.substr(0, split_index);
        const std::string tagName = inputName.substr(split_index + 1);
        inputs.push_back(FileTagPair(fileName, tagName));
        Initialize(args...);
    }

    void Initialize() {}

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
    std::vector<FileTagPair> inputs;
    root_ext::SingleSidedPage page;
    Printer printer;
    std::string treeName, branchName, title;
    root_ext::Range xRange;
    unsigned nBins;
    bool useLogX, useLogY;
    MyHistogramSource source;
};

/*! Print smart histograms with specified name superimposing several files.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TTree.h>

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

        TH1D* histogram = new TH1D(name.c_str(), name.c_str(), nBins, xRange.min, xRange.max);
        tree->Draw(command.c_str(), "");
        return histogram;
    }

private:
    root_ext::Range xRange;
    unsigned nBins;
};

class Print_SmartHistogram {
public:
    using FileTagPair = std::pair<std::string, std::string>;
    using Printer = root_ext::PdfPrinter;

    Print_SmartHistogram(const std::string& outputFileName, const std::string& _histogramName,
                         const std::string& _title, double _xMin, double _xMax, unsigned _nBins,  bool _useLogX,
                         bool _useLogY, const std::vector<std::string>& args)
       : printer(outputFileName), histogramName(_histogramName), title(_title), xRange(_xMin, _xMax), nBins(_nBins),
         useLogX(_useLogX), useLogY(_useLogY), source(xRange, nBins)
    {
        for(const std::string& inputName : args) {
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
        page.side.use_log_scaleX = useLogX;
        page.side.use_log_scaleY = useLogY;
        page.side.xRange = xRange;
        page.side.fit_range_x = false;
        page.side.layout.has_legend = false;

        Print(histogramName, title);
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
    std::vector<FileTagPair> inputs;
    root_ext::SingleSidedPage page;
    Printer printer;
    std::string histogramName, title;
    root_ext::Range xRange;
    unsigned nBins;
    bool useLogX, useLogY;
    MyHistogramSource source;
};

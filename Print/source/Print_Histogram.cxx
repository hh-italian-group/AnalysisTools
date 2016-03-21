/*! Print histograms with specified name superimposing several files.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Core/include/RootExt.h"
#include "../include/RootPrintToPdf.h"

class MyHistogramSource : public root_ext::SimpleHistogramSource<TH1D, Double_t> {
public:
    MyHistogramSource(unsigned _rebin)
        : rebinFactor(_rebin) {}
protected:
    virtual void Prepare(TH1D* histogram, const std::string& display_name,
                         const PlotOptions& plot_options) const
    {
        SimpleHistogramSource::Prepare(histogram,display_name,plot_options);
        histogram->Rebin(rebinFactor);
    }

private:
    unsigned rebinFactor;
};

class Print_Histogram {
public:
    using FileTagPair = std::pair<std::string, std::string>;
    using Printer = root_ext::PdfPrinter;

    Print_Histogram(const std::string& outputFileName, const std::string& _histogramName, const std::string& _title,
                    double _xMin, double _xMax, unsigned _rebin, bool _useLogX, bool _useLogY,
                    const std::vector<std::string>& args)
       : printer(outputFileName), histogramName(_histogramName), title(_title), xRange(_xMin, _xMax),
         useLogX(_useLogX), useLogY(_useLogY), source(_rebin)
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
        page.side.axis_titleX = "P_{T} (#mu) [GeV]";
        page.side.axis_titleY = "N entries";
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
    bool useLogX, useLogY;
    MyHistogramSource source;
};


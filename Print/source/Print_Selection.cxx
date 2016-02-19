/*! Print event selection histograms.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <iostream>

#include "AnalysisTools/Core/include/RootExt.h"
#include "../include/RootPrintToPdf.h"

class Print_Selection {
public:
    typedef std::pair< std::string, std::string > FileTagPair;
    typedef root_ext::PdfPrinter Printer;
    typedef root_ext::SimpleHistogramSource<TH1D, Double_t> MyHistogramSource;

    template<typename ...Args>
    Print_Selection(const std::string& outputFileName, const Args& ...args):
       printer(outputFileName)
    {
        Initialize(args...);
        for(const FileTagPair& fileTag : inputs) {
            auto file = root_ext::OpenRootFile(fileTag.first);
            source.Add(fileTag.second, file);
        }
    }

    void Run()
    {
        page.side.use_log_scaleY = true;
        page.side.layout.has_legend = false;

        PrintAll("Selection_event", "Event selection");
        PrintAll("Selection_muons", "Muon selection");
        PrintAll("Selection_taus", "Tau selection");
        PrintAll("Selection_electrons_bkg", "Bkg Electron selection");
        PrintAll("Selection_bjets", "b-jet selection");


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

    void PrintAll(const std::string& name, const std::string& title, std::string second_suffix = "")
    {
        try {
            Print(name, title, AddSuffix("", second_suffix), "Number of entries");
            Print(name, title, AddSuffix("effRel", second_suffix), "Relative efficiency");
            Print(name, title, AddSuffix("effAbs", second_suffix), "Absolute efficiency");
        } catch(std::runtime_error& e) {
            std::cerr << "ERROR: " << e.what() << std::endl;
        }
    }

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
    MyHistogramSource source;
    root_ext::SingleSidedPage page;
    Printer printer;
};

/*! Common CERN ROOT extensions.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include "AnalysisTools/Core/include/RootExt.h"

#include <memory>
#include <map>

#include <TROOT.h>
#include <TClass.h>
#include <TLorentzVector.h>
#include <TMatrixD.h>
#include <TFile.h>
#include <Compression.h>

#include "AnalysisTools/Core/include/exception.h"

namespace root_ext {

std::shared_ptr<TFile> CreateRootFile(const std::string& file_name, ROOT::ECompressionAlgorithm compression,
                                      int compression_level)
{
    std::shared_ptr<TFile> file(TFile::Open(file_name.c_str(), "RECREATE", "", compression * 100 + compression_level));
    if(!file || file->IsZombie())
        throw analysis::exception("File '%1%' not created.") % file_name;
    return file;
}

std::shared_ptr<TFile> OpenRootFile(const std::string& file_name)
{
    std::shared_ptr<TFile> file(TFile::Open(file_name.c_str(), "READ"));
    if(!file || file->IsZombie())
        throw analysis::exception("File '%1%' not opened.") % file_name;
    return file;
}

void WriteObject(const TObject& object, TDirectory* dir, const std::string& name)
{
    if(!dir)
        throw analysis::exception("Can't write object to nullptr.");
    const std::string name_to_write = name.size() ? name : object.GetName();
    dir->WriteTObject(&object, name_to_write.c_str(), "Overwrite");
}


TDirectory* GetDirectory(TDirectory& root_dir, const std::string& name, bool create_if_needed)
{
    if(!name.size() || (name.size() == 1 && name.at(0) == '/'))
        return &root_dir;
    TDirectory* dir = root_dir.GetDirectory(name.c_str());
    if(!dir && create_if_needed) {
        const size_t pos = name.find("/");
        if(pos == std::string::npos || pos == name.size() - 1) {
            root_dir.mkdir(name.c_str());
            dir = root_dir.GetDirectory(name.c_str());
        } else {
            const std::string first_dir_name = name.substr(0, pos), sub_dirs_path = name.substr(pos + 1);
            TDirectory* first_dir = GetDirectory(root_dir, first_dir_name, true);
            dir = GetDirectory(*first_dir, sub_dirs_path, true);
        }
    }

    if(!dir)
        throw analysis::exception("Unable to get directory '%1%' from the root directory '%2%'.")
            % name % root_dir.GetName();
    return dir;
}

ClassInheritance FindClassInheritance(const std::string& class_name)
{
    TClass *cl = gROOT->GetClass(class_name.c_str());
    if(!cl)
        throw analysis::exception("Unable to get TClass for class named '%1%'.") % class_name;

    ClassInheritance inheritance;
    if(cl->InheritsFrom("TH1"))
        inheritance = ClassInheritance::TH1;
    else if(cl->InheritsFrom("TTree"))
        inheritance = ClassInheritance::TTree;
    else if(cl->InheritsFrom("TDirectory"))
        inheritance = ClassInheritance::TDirectory;
    else
        throw analysis::exception("Unknown class inheritance for class named '%1%'.") % class_name;

    return inheritance;
}
} // namespace root_ext


std::ostream& operator<<(std::ostream& s, const TVector3& v)
{
    s << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
    return s;
}

std::ostream& operator<<(std::ostream& s, const TLorentzVector& v)
{
    s << "(pt=" << v.Pt() << ", eta=" << v.Eta() << ", phi=" << v.Phi() << ", E=" << v.E() << ", m=" << v.M() << ")";
    return s;
}

// Based on TMatrixD::Print code.
std::ostream& operator<<(std::ostream& s, const TMatrixD& matrix)
{
    if (!matrix.IsValid()) {
        s << "Matrix is invalid";
        return s;
    }

    //build format
    static const char *format = "%11.4g ";
    char topbar[100];
    snprintf(topbar,100, format, 123.456789);
    size_t nch = strlen(topbar) + 1;
    if (nch > 18) nch = 18;
    char ftopbar[20];
    for(size_t i = 0; i < nch; i++) ftopbar[i] = ' ';
    size_t nk = 1 + size_t(std::log10(matrix.GetNcols()));
    snprintf(ftopbar+nch/2,20-nch/2,"%s%zud","%",nk);
    size_t nch2 = strlen(ftopbar);
    for (size_t i = nch2; i < nch; i++) ftopbar[i] = ' ';
    ftopbar[nch] = '|';
    ftopbar[nch+1] = 0;

    s << matrix.GetNrows() << "x" << matrix.GetNcols() << " matrix";

    size_t cols_per_sheet = 5;
    if (nch <= 8) cols_per_sheet =10;
    const size_t ncols  = static_cast<size_t>(matrix.GetNcols());
    const size_t nrows  = static_cast<size_t>(matrix.GetNrows());
    const size_t collwb = static_cast<size_t>(matrix.GetColLwb());
    const size_t rowlwb = static_cast<size_t>(matrix.GetRowLwb());
    nk = 5+nch*std::min<size_t>(cols_per_sheet, static_cast<size_t>(matrix.GetNcols()));
    for (size_t i = 0; i < nk; i++)
        topbar[i] = '-';
    topbar[nk] = 0;
    for (size_t sheet_counter = 1; sheet_counter <= ncols; sheet_counter += cols_per_sheet) {
        s << "\n     |";
        for (size_t j = sheet_counter; j < sheet_counter+cols_per_sheet && j <= ncols; j++) {
            char ftopbar_out[100];
            snprintf(ftopbar_out, 100, ftopbar, j+collwb-1);
            s << ftopbar_out;
        }
        s << "\n" << topbar << "\n";
        if (matrix.GetNoElements() <= 0) continue;
        for (size_t i = 1; i <= nrows; i++) {
            char row_out[100];
            snprintf(row_out, 100, "%4zu |",i+rowlwb-1);
            s << row_out;
            for (size_t j = sheet_counter; j < sheet_counter+cols_per_sheet && j <= ncols; j++) {
                snprintf(row_out, 100, format, matrix(static_cast<Int_t>(i+rowlwb-1),
                                                      static_cast<Int_t>(j+collwb-1)));
                s << row_out;
            }
            s << "\n";
        }
    }
    return s;
}

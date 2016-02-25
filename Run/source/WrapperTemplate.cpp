/*! Template file to create wrapper for cxx executable script.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <cstddef>
#include "SOURCE"
#include <TROOT.h>
#include <iostream>
#include <memory>
#include <vector>

namespace make_tools {

template<typename Value>
struct CmdLineArgument {
    static bool TryParse(const std::string& str, Value& value)
    {
        std::istringstream s(str);
        s >> value;
        return !s.fail();
    }
};

template<>
struct CmdLineArgument<bool> {
    static bool TryParse(const std::string& str, bool& value)
    {
        std::istringstream s(str);
        s >> std::boolalpha >> value;
        return !s.fail();
    }
};

template<typename T>
struct Factory {
    static T* Make(const std::vector<std::string>& argv)
    {
        ARG_PARSE_CODE
    }
};
} // namespace make_tools

int main(int argc, char *argv[])
{
        int result = 0;
        try {
                gROOT->ProcessLine("#include <vector>");

                std::vector<std::string> args;
                for(int n = 0; n < argc; ++n)
                    args.push_back(argv[n]);
                std::unique_ptr<NAME> a( make_tools::Factory<NAME>::Make(args) );
                a->Run();
        }
        catch(std::exception& e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
                result = 1;
        }
        return result;
}

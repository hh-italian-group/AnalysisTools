/*! Definition of the wrapper for the main entry of a program.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <exception>
#include <iostream>
#include <boost/program_options.hpp>
#include <TROOT.h>

namespace run {

using options_description = boost::program_options::options_description;

template <typename T>
auto value(T* v) -> decltype(boost::program_options::value(v)) { return boost::program_options::value(v); }

bool ParseProgramArguments(int argc, char* argv[], options_description& options_desc)
{
    namespace po = boost::program_options;

    options_description desc("Available command line arguments");
    desc.add_options()("help", "print help message");
    desc.add(options_desc);

    po::variables_map variables;

    try {
        po::store(po::parse_command_line(argc, argv, desc), variables);
        if(variables.count("help")) {
            std::cout << desc << std::endl;
            return false;
        }
        notify(variables);
    }
    catch(po::error& e) {
        std::cerr << "ERROR: " << e.what() << ".\n\n" << desc << std::endl;
        return false;
    }
    return true;
}

template<typename Program, typename Options>
int Main(int argc, char* argv[], const Options& options, options_description& options_desc)
{
    static constexpr int NORMAL_EXIT_CODE = 0;
    static constexpr int ERROR_EXIT_CODE = 1;
    static constexpr int PRINT_ARGS_EXIT_CODE = 2;

    try {
        gROOT->ProcessLine("#include <vector>");
        if(!ParseProgramArguments(argc, argv, options_desc))
            return PRINT_ARGS_EXIT_CODE;
        Program program(options);
        program.Run();
    } catch(std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return ERROR_EXIT_CODE;
    }

    return NORMAL_EXIT_CODE;
}

} // namespace run

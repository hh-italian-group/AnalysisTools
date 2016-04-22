/*! Definition of the wrapper for the main entry of a program.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <exception>
#include <iostream>
#include <list>
#include <boost/program_options.hpp>
#include <TROOT.h>
#include <TH1.h>

#define REQ_ARG(type, name) run::Argument<type> name{#name, ""}
#define OPT_ARG(type, name, default_value) run::Argument<type> name{#name, "", default_value}

#define PROGRAM_MAIN(program_class, args_class) \
    int main(int argc, char* argv[]) { \
        using namespace run; \
        args_class args; \
        options_description desc; \
        positional_options_description pos_desc; \
        ArgumentBase::ApplyAll(desc, pos_desc); \
        return Main<program_class>(argc, argv, args, desc, pos_desc);\
    } \
    /**/

namespace run {

using options_description = boost::program_options::options_description;
using positional_options_description = boost::program_options::positional_options_description;

template <typename T>
auto value(T* v) -> decltype(boost::program_options::value(v)) { return boost::program_options::value(v); }

bool ParseProgramArguments(int argc, char* argv[], const options_description& options_desc,
                           const positional_options_description& pos_desc)
{
    namespace po = boost::program_options;

    options_description desc("Available command line arguments");
    desc.add_options()("help", "print help message");
    desc.add(options_desc);

    po::variables_map variables;

    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), variables);
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
int Main(int argc, char* argv[], const Options& options, const options_description& options_desc,
         const positional_options_description& pos_desc)
{
    static constexpr int NORMAL_EXIT_CODE = 0;
    static constexpr int ERROR_EXIT_CODE = 1;
    static constexpr int PRINT_ARGS_EXIT_CODE = 2;

    try {
        TH1::SetDefaultSumw2();
        gROOT->ProcessLine("#include <vector>");
        gROOT->SetMustClean(kFALSE);
        if(!ParseProgramArguments(argc, argv, options_desc, pos_desc))
            return PRINT_ARGS_EXIT_CODE;
        Program program(options);
        program.Run();
    } catch(std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return ERROR_EXIT_CODE;
    }

    return NORMAL_EXIT_CODE;
}

struct ArgumentBase {
    using ArgList = std::list<ArgumentBase*>;
    std::string name, description;
    bool required;

    ArgumentBase(const std::string& _name, const std::string& _description, bool _required)
        : name(_name), description(_description), required(_required)
    {
        Arguments().push_back(this);
    }

    virtual ~ArgumentBase()
    {
        auto iter = std::find(Arguments().begin(), Arguments().end(), this);
        if(iter != Arguments().end())
            Arguments().erase(iter);
    }

    virtual void Apply(options_description& options_desc, positional_options_description& pos_desc) = 0;

    static void ApplyAll(options_description& options_desc, positional_options_description& pos_desc)
    {
        for(ArgumentBase* arg : Arguments())
            arg->Apply(options_desc, pos_desc);
    }

    static ArgList& Arguments() { static ArgList args; return args; }
};


namespace detail {
template<typename T>
struct ArgumentMultiplicity { static int Get() { return 1; } };

template<typename T>
struct ArgumentMultiplicity<std::vector<T>> { static int Get() { return -1; } };

} // namespace detail

template<typename T>
struct Argument : public ArgumentBase {
    using Value = T;

    Value val;

    Argument(const std::string& _name, const std::string& _description)
        : ArgumentBase(_name, _description, true) {}

    Argument(const std::string& _name, const std::string& _description, const Value& default_value)
        : ArgumentBase(_name, _description, false), val(default_value) {}

    const Value& operator()() const { return val; }

    virtual void Apply(options_description& options_desc, positional_options_description& pos_desc) override
    {
        auto opt_value = value<Value>(&val);
        if(required)
            opt_value->required();
        else
            opt_value->default_value(val);
        options_desc.add_options()(name.c_str(), opt_value, description.c_str());
        pos_desc.add(name.c_str(), detail::ArgumentMultiplicity<Value>::Get());
    }
};

} // namespace run

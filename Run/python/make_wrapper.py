#!/usr/bin/env python
# Creates wrapper for cxx executable script.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

import sys
import os.path
import string
import ROOT

def main(args):
    if len(args) != 3:
        print "Usage: make_wrapper.py template_file input_file output_file"
        sys.exit()

    template_file_name = args[0]
    input_file_name = args[1]
    output_file_name = args[2]

    class_name, file_ext = os.path.splitext(os.path.basename(input_file_name))

    input = open(input_file_name, 'r').read()

    class_def_pos = input.find(class_name)
    if class_def_pos == -1:
        print "ERROR: class definition not found."
        sys.exit()

    constructor_def_pos = input.find(class_name, class_def_pos + 1)
    if constructor_def_pos == -1:
        print "ERROR: constructor definition not found."
        sys.exit()

    constructor_start_pos = input.find("(", constructor_def_pos)
    constructor_end_pos = input.find(")", constructor_def_pos)

    if constructor_start_pos == -1 or constructor_end_pos == -1 or constructor_end_pos <= constructor_start_pos:
        print "ERROR: bad constructor definition."
        sys.exit()

    constructor = input[constructor_start_pos+1:constructor_end_pos]

    args = constructor.replace("const", "").replace("&", "").split(",")

    arg_types = []
    for arg in args:
        type_name, value_name = arg.split()
        arg_types.append(type_name)

    n_max = len(arg_types)
    indent = "        "
    variable_number_of_args = False
    min_n_args = n_max
    arg_parse_code = ""

    for n in range(0, n_max):
        type_name = arg_types[n]
        if type_name == "std::vector<std::string>" and n == n_max - 1:
            arg_parse_code += "{0}std::vector<std::string> arg_{2}(argv.begin() + {2}, argv.end());\n".format(
                              indent, type_name, n+1)
            variable_number_of_args = True
        elif type_name == "std::string":
            arg_parse_code += "{0}std::string arg_{1} = argv.at({1});\n".format(indent, n+1)
        else:
            arg_parse_code += "{0}{1} arg_{2};\n{0}if(!CmdLineArgument<{1}>::TryParse(argv.at({2}), arg_{2}))\n" \
                              "{0}    throw std::runtime_error(\"Invalid command line argument #{2}\");\n".format(
                              indent, type_name, n+1)

    arg_parse_code += "\n{0}return new T(".format(indent)
    next_split = ""
    for n in range(0, n_max):
        arg_parse_code += "{0}arg_{1}".format(next_split, n+1)
        next_split = ", "
    arg_parse_code += ");"

    n_args_condition = ("argv.size() < {0}" if variable_number_of_args else "argv.size() != {0} + 1").format(n_max)

    arg_parse_code = "if({1})\n{0}    throw std::runtime_error(\"Invalid number of arguments\");\n\n"\
                     .format(indent, n_args_condition) + arg_parse_code


    template = open(template_file_name, 'r').read()

    output = template.replace("SOURCE", input_file_name).replace("ARG_PARSE_CODE", arg_parse_code) \
             .replace("NAME", class_name)

    output_file = open(output_file_name, 'w')
    output_file.write(output)
    output_file.close()


if __name__ == '__main__':
    main(sys.argv[1:])

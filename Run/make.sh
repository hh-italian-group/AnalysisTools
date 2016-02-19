#!/bin/bash
# Compile program executable for a given C++ file, passing command line arguments into the main object constructor.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

function get_arg_value {
        if [ "${ARGS[$1]:0:1}" == "@" ] ; then
                arg_value="${ARGS[$1]:1}"
        else
                arg_value="\"${ARGS[$1]}\""
        fi

}


SCRIPT_RUN_PATH=$1
JOB_NAME=$2
COMPILE_FLAGS=$3
NAME=$4
ARGS=("$@")

if [ "$CMSSW_BASE/" = "/" ] ; then
    SCRIPT_PATH="."
else
    SCRIPT_PATH="$CMSSW_BASE/src/HHbbTauTau"
fi

if [ ! -d "$SCRIPT_RUN_PATH" ] ; then
    echo "ERROR: script run path $SCRIPT_RUN_PATH doesn't exist"
    exit 1
fi

SOURCE=$( find $SCRIPT_PATH -name "${NAME}.C" )
n=${#ARGS[@]}
arg_list=""
if (( n > 4 )) ; then
        get_arg_value 4
        arg_list="$arg_value"
        for (( i = 5; i < n; i++ )) ; do
                get_arg_value i
                arg_list="$arg_list, $arg_value"
        done
fi

CODE_OUT="$SCRIPT_RUN_PATH/${JOB_NAME}.cpp"
EXE_NAME="$SCRIPT_RUN_PATH/${JOB_NAME}"
rm -f "$EXE_NAME"

printf \
"#include \"${SOURCE}\"
#include <TROOT.h>
#include <iostream>
int main()
{
        int result = 0;
        try {
                gROOT->ProcessLine(\"#include <vector>\");
                $NAME a( $arg_list );
                a.Run();
        }
        catch(std::exception& e) {
                std::cerr << \"ERROR: \" << e.what() << std::endl;
                result = 1;
        }
        return result;
}
" > $CODE_OUT

g++ -std=c++0x -Wall $COMPILE_FLAGS \
        -I. -I$CMSSW_BASE/src -I$CMSSW_RELEASE_BASE/src -I$ROOT_INCLUDE_PATH -I$BOOST_INCLUDE_PATH \
        $( root-config --libs ) -lMathMore -lGenVector -lTMVA -lASImage -L$BOOST_BASE/lib  \
        -o $EXE_NAME $CODE_OUT

RESULT=$?
rm -f "$CODE_OUT"
exit $RESULT

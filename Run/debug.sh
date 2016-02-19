#!/bin/bash
# Compile and run C++ file with a specified name, passing to it command line arguments in debug mode.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

NAME=$1

if [ "$CMSSW_BASE/" = "/" ] ; then
    SCRIPT_PATH="."
else
    SCRIPT_PATH="$CMSSW_BASE/src/HHbbTauTau"
fi

SCRIPT_RUN_PATH="$SCRIPT_PATH/run"
mkdir -p $SCRIPT_RUN_PATH


LC_CTYPE=C SUFFIX=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 8 | head -n 1)
JOB_NAME=${NAME}_${SUFFIX}
EXE_NAME="$SCRIPT_RUN_PATH/$JOB_NAME"

$SCRIPT_PATH/RunTools/make.sh $SCRIPT_RUN_PATH $JOB_NAME -g $*

RESULT=$?
if [[ $RESULT -eq 0 ]] ; then
        valgrind -v --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=debug.log \
                 --suppressions=RunTools/config/known_memory_leaks.supp \
                 --suppressions=RunTools/config/known_issues.supp --gen-suppressions=all $EXE_NAME
        rm -f "$EXE_NAME"
fi


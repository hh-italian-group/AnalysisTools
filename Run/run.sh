#!/bin/bash
# Compile and execute cxx target.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -lt 1 ] ; then
    printf "run.sh: compile and execute cxx target.\n"
    printf "Usage:\n"
    printf "./run.sh target_name [args]\tcompile and execute the target by its name;\n"
    printf "./run.sh --list\t\t\tlist all available targets.\n"
    exit
fi

function make_target_list {
    local prj="$1"
    TARGET_LIST=$( cd "$prj" && make help \
    | grep -v -e "\(^[^\.]\|^\.\.\. \(${prj}\|all\|clean\|depend\|edit_cache\|rebuild_cache\|others\|configs\|scripts\|sources\|headers\|wrappers\)\)" \
    | sed 's/^\.\.\. //')
}

NAME=$1

if [ "$CMSSW_BASE/" = "/" ] ; then
    BUILD_PATH="build"
else
    BUILD_PATH="$CMSSW_BASE/build"
fi

WORKING_PATH=$(pwd)
cd "$BUILD_PATH"
PROJECTS=$(ls)
TARGET_FOUND=0
EXE_NAME=""

for PROJECT in $PROJECTS ; do
    make_target_list "$PROJECT"
    if [ "$NAME" = "--list" ] ; then
        if [ "$TARGET_LIST" != "" ] ; then
            echo "$TARGET_LIST"
        fi
    else
        TARGET=$(echo "$TARGET_LIST" | grep "$NAME")
        if [ "$NAME" = "$TARGET" ] ; then
            TARGET_FOUND=1
            cd "$PROJECT"
            make "$NAME"
            RESULT=$?
            if [ $RESULT -ne 0 ] ; then
                echo "ERROR: failed to compile $NAME."
                exit
            fi
            EXE_NAME="./$BUILD_PATH/$PROJECT/$NAME"
            break
        fi
    fi
done

if [ "$NAME" = "--list" ] ; then
    exit
fi

cd "$WORKING_PATH"
if ! [ -f "$EXE_NAME" ] ; then
    echo "ERROR: target $NAME not found."
    exit
fi

$EXE_NAME "${@:2}"

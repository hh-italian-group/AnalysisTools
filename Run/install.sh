#!/bin/bash
# Prepare analysis workspace.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -lt 2 ] ; then
    echo "Usage: build_root project_name_1 [project_name_2] ..."
    exit
fi

if [ ! -f "AnalysisTools/Run/run.sh" ] ; then
    echo "Install script should be run in the source root directory."
    exit
fi

if [ ! -f "run.sh" ] ; then
    ln -s "AnalysisTools/Run/run.sh" "run.sh"
fi

BUILD_ROOT="$1"
SOURCE_ROOT="$(pwd)"
if [[ "$OSTYPE" == "darwin"* ]]; then
    CC_COMPILER=/usr/bin/clang
    CXX_COMPILER=/usr/bin/clang++
else
    CC_COMPILER=clang
    CXX_COMPILER=clang++
fi

command -v cmake3 >/dev/null 2>&1
RESULT=$?
if [ $RESULT -eq 0 ] ; then
    CMAKE=cmake3
else
    CMAKE=cmake
fi

for PROJECT_NAME in "${@:2}" ; do
    cd "$SOURCE_ROOT"
    BUILD_DIR="$BUILD_ROOT/$PROJECT_NAME"
    SOURCE_DIR="$SOURCE_ROOT/$PROJECT_NAME"
    if [ ! -d "$SOURCE_DIR" ] ; then
        echo "Project '$PROJECT_NAME' not found."
        exit
    fi
    if [ -d "$BUILD_DIR" ] ; then
        rm -r "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    CC=$CC_COMPILER CXX=$CXX_COMPILER $CMAKE "$SOURCE_DIR"
    RESULT=$?
    if [ $RESULT -ne 0 ] ; then
        echo "ERROR: unable to cmake project '$PROJECT_NAME'."
        exit
    fi
done

echo "Projects { ${@:2} } are installed."

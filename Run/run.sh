#!/bin/bash
# Compile and execute cxx target.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -lt 1 ] ; then
    printf "run.sh: compile and execute cxx target.\n"
    printf "Usage:\n"
    printf "./run.sh target_name [args]     compile and execute the target by its name.\n"
    printf "./run.sh --list                 list all available targets.\n"
    printf "./run.sh --update               update build areas.\n"
    printf "./run.sh --make target_name     compile target without executing it.\n"
    printf "./run.sh --make-all             compile all targets.\n"
    exit 1
fi

function make_target_list {
    local prj="$1"
    local root_targets=$( cd "$prj" && make help | grep -v -e "\(^[^\.]\|/\)" | sed 's/^\.\.\. //' )
    TARGET_LIST=$( echo "$root_targets" \
        | grep -v -e "\(${prj}\|all\|clean\|depend\|edit_cache\|rebuild_cache\|others\|configs\|scripts\|sources\|headers\|wrappers\|RootDictionaries.*\)")

}

NAME=$1
MAKE_ONLY=0

if [ "$NAME" = "--make" ] ; then
    MAKE_ONLY=1
    NAME=$2
fi

if [ "$CMSSW_BASE/" = "/" ] ; then
    BUILD_PATH="./build"
else
    BUILD_PATH="$CMSSW_BASE/build"
fi

if [ ! -f .rootrc ] ; then
    echo "RooFit.Banner: no" > .rootrc
fi

command -v cmake3 >/dev/null 2>&1
RESULT=$?
if [ $RESULT -eq 0 ] ; then
    CMAKE=cmake3
else
    CMAKE=cmake
fi

WORKING_PATH=$(pwd)
cd "$BUILD_PATH"
PROJECTS=$(ls)
TARGET_FOUND=0
EXE_NAME=""

for PROJECT in $PROJECTS ; do
    if [ "$NAME" = "--update" ] ; then
        cd $PROJECT
        $CMAKE .
        RESULT=$?
        if [ $RESULT -ne 0 ] ; then
            exit $RESULT
        fi
        cd ..
        continue
    elif [ "$NAME" = "--make-all" ] ; then
        cd $PROJECT
        $CMAKE --build . -- -j4
        RESULT=$?
        if [ $RESULT -ne 0 ] ; then
            exit $RESULT
        fi
        cd ..
        continue
    fi
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
            $CMAKE --build . --target "$NAME" -- -j4
            RESULT=$?
            if [ $RESULT -ne 0 ] ; then
                echo "ERROR: failed to compile $NAME."
                exit 1
            fi
            EXE_NAME="$BUILD_PATH/$PROJECT/$NAME"
            break
        fi
    fi
done

if [ "$NAME" = "--list" -o "$NAME" = "--update"  -o "$NAME" = "--make-all" ] ; then exit 0 ; fi

cd "$WORKING_PATH"
if ! [ -f "$EXE_NAME" ] ; then
    echo "ERROR: target $NAME not found."
    exit 1
fi

if [ $MAKE_ONLY -eq 1 ] ; then
    echo "Executible '$EXE_NAME' is successfully created."
    exit 0
fi

$EXE_NAME "${@:2}"
RESULT=$?
exit $RESULT

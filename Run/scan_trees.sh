#!/bin/bash
# Scan root files in the given directory for the specified event.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -ne 2 ] ; then
    echo "Usage: trees_path even_id"
    exit
fi

TREES_PATH=$1
EVENT_ID=$2

SCAN_FILE="RunTools/source/Scan.C"

find $TREES_PATH -type f -exec root -b -l -q $SCAN_FILE\($EVENT_ID,\"\{\}\"\) \; > /dev/null

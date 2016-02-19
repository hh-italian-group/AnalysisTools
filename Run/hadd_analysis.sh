#!/bin/bash
# Apply hadd command to all analysis output ROOT files and place the results into the output directory.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -lt 2 -o $# -gt 3 ] ; then
    echo "Usage: analysis_path output_path [create_archive]"
    exit
fi

ANALYSIS_PATH=$1

OUTPUT_PATH=$2
if [ -d "$OUTPUT_PATH" ] ; then
    echo "ERROR: output path '$OUTPUT_PATH' already exists."
    exit
fi
mkdir -p $OUTPUT_PATH
OUTPUT_PATH=$( cd "$OUTPUT_PATH" ; pwd )

CREATE_ARCHIVE=$3

cd $ANALYSIS_PATH

ANA_FOLDERS=$( find . -maxdepth 1 ! -path . -type d | awk -F/ '{ print $NF }'  )

for FOLDER in $ANA_FOLDERS ; do
    SUB_FOLDERS=$( find $FOLDER -maxdepth 1 -type d | awk -F/ '{ print $NF }' )
    for SUB_FOLDER in $SUB_FOLDERS ; do
                if [ $SUB_FOLDER = $FOLDER ] ; then
                        continue
                fi
        mkdir -p $OUTPUT_PATH/$FOLDER
        if [ $SUB_FOLDER = "Radion" ] ; then
            cp $FOLDER/$SUB_FOLDER/*.root $OUTPUT_PATH/$FOLDER/
        else
            hadd -f9 $OUTPUT_PATH/$FOLDER/${SUB_FOLDER}.root $FOLDER/$SUB_FOLDER/*.root
        fi
        done
done

if [ "$CREATE_ARCHIVE" = "archive" -o "$CREATE_ARCHIVE" = "create" -o "$CREATE_ARCHIVE" = "yes" ] ; then
    cd $OUTPUT_PATH
    tar cjvf ${OUTPUT_PATH}.tar.bz2 .
fi

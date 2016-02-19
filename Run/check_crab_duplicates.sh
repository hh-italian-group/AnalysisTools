#!/bin/bash
# Check if there are duplicated files in CRAB output and, if needed, remove older duplicates.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -ne 2 ] ; then
    echo "Usage: dir_name file_name_prefix"
    echo "Output: paths of files that has the same job id."
    exit
fi

DIR_NAME=$1
PREFIX=$2

TOTAL_NUMBER_OF_FILES=$( ls $DIR_NAME | wc -l )

echo "Total number of files in the directory: $TOTAL_NUMBER_OF_FILES"

JOB_LIST=$( ls $DIR_NAME | sed "s/\(${PREFIX}_\)\([0-9]*\)\(.*\)/\2/" | sort | uniq -d )

if [ "x$JOB_LIST" = "x" ] ; then
        echo "There are no jobs that have more than one output file."
        exit
fi

echo "IDs of the jobs that have more than one output file:" $JOB_LIST

read -p "List all duplicate files (yes/no)? " -r REPLAY
if [ "$REPLAY" != "y" -a "$REPLAY" != "yes" -a "$REPLAY" != "Y" ] ; then exit ; fi

FILE_NAME_PATTERNS=$( echo "$JOB_LIST" | sed "s/\(.*\)/${PREFIX}_\1_\*/" )

for PATTERN in $FILE_NAME_PATTERNS ; do
        FILE_LIST=$( find $DIR_NAME -name "$PATTERN" )
        ls -al $FILE_LIST
done

read -p "Delete all duplicates that have older modification date (yes/no)? " -r REPLAY
if [ "$REPLAY" != "y" -a "$REPLAY" != "yes" -a "$REPLAY" != "Y" ] ; then exit ; fi


read -p "Please, input name of the tool that will be used to remove files: " -r RM_TOOL
if [ "x$RM_TOOL" = "x" ] ; then exit ; fi

for PATTERN in $FILE_NAME_PATTERNS ; do
        FILE_LIST=$( find $DIR_NAME -name "$PATTERN" )
        FILE_ARRAY=( $FILE_LIST )
        NEWEST_FILE=${FILE_ARRAY[0]}
        for FILE in $FILE_LIST ; do
                if [ "$FILE" -nt "$NEWEST_FILE" ] ; then
                        NEWEST_FILE=$FILE
                fi
        done
        for FILE in $FILE_LIST ; do
                if [ "$FILE" != "$NEWEST_FILE" ] ; then
                        printf "Removing file '$FILE'... "
                        $RM_TOOL $FILE
                        printf "done.\n"
                fi
        done
done



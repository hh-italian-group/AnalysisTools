#!/bin/bash
# Resubmit failed jobs on batch.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -ne 6 ] ; then
    echo "Usage: dataset_name output_path global_tag is_mc include_sim prefix"
    exit
fi

DATASET=$1
OUTPUT_PATH=$2
GLOBAL_TAG=$3
IS_MC=$4
INCLUDE_SIM=$5
PREFIX=$6

if [ ! -d "$OUTPUT_PATH" ] ; then
    echo "ERROR: output path '$OUTPUT_PATH' does not exist."
    exit
fi
OUTPUT_PATH=$( cd "$OUTPUT_PATH" ; pwd )


#FILE_LIST_PATH="TreeProduction/dataset/${DATASET}_Pat"
FILE_LIST_PATH="PatProduction/dataset/${DATASET}_AOD"
if [ ! -d $FILE_LIST_PATH ] ; then
    echo "ERROR: file list path '$FILE_LIST_PATH' does not exists."
    exit
fi
JOBS=$( find $FILE_LIST_PATH -maxdepth 1 -name "*.txt" -printf "%f\n" | sed "s/\.txt//" | sort )
if [ "x$JOBS" = "x" ] ; then
        echo "ERROR: directory '$FILE_LIST_PATH' does not contains any job description."
        exit
fi
N_JOBS=$( echo "$JOBS" | wc -l )
echo "Total number of jobs: $N_JOBS"

NEW_FILE_LIST_PATH="PatProduction/dataset/retry/${DATASET}"
n=1
while [ -d $NEW_FILE_LIST_PATH ] ; do
    n=$(($n + 1))
    NEW_FILE_LIST_PATH="PatProduction/dataset/retry/${DATASET}_${n}"
done

FILE_JOB_RESULT="$OUTPUT_PATH/job_result.log"
SUCCESSFULL_JOBS=""
N_SUCCESSFULL_JOBS=0
if [ -f $FILE_JOB_RESULT ] ; then
    SUCCESSFULL_JOBS=$( cat $FILE_JOB_RESULT | sed -n "s/\(^0 \)\([^ ]*\)\(.*\)/\2/p" | sort )
    N_SUCCESSFULL_JOBS=$( echo "$SUCCESSFULL_JOBS" | sed '/^\s*$/d' | wc -l )
    echo "Number of successfull jobs: $N_SUCCESSFULL_JOBS"
else
    echo "job_results.log not found. Considering that there are no finished jobs yet."
fi

JOB_STAT_RESULT=$( qstat -u $(whoami) | grep $DATASET )
JOBS_IN_QUEUE=$( echo "$JOB_STAT_RESULT" | awk '{print $4}' )
N_RUNNING_JOBS=$( echo "$JOB_STAT_RESULT" | grep " R " | wc -l )
N_PENDING_JOBS=$( echo "$JOB_STAT_RESULT" | grep " Q " | wc -l )

echo "Number of running jobs: $N_RUNNING_JOBS"
echo "Number of pending jobs: $N_PENDING_JOBS"

JOBS_TO_RESUBMIT=$( echo -e "$JOBS"\\n"$SUCCESSFULL_JOBS"\\n"$JOBS_IN_QUEUE" | sort | sed '/^\s*$/d' | uniq -u )
N_JOBS_TO_RESUBMIT=$( echo "$JOBS_TO_RESUBMIT" | sed '/^\s*$/d' | wc -l )
if [ $N_JOBS_TO_RESUBMIT -eq 0 ] ; then
        echo "There are no jobs to resubmit."
        exit
fi
echo "Jobs to resubmit: "$JOBS_TO_RESUBMIT
read -p "Number of jobs to resubmit: $N_JOBS_TO_RESUBMIT. Continue? (yes/no) " -r REPLAY
if [ "$REPLAY" != "y" -a "$REPLAY" != "yes" -a "$REPLAY" != "Y" ] ; then
        echo "No jobs have been resubmited."
        exit
fi

echo "Resubmit number $n"
mkdir -p $NEW_FILE_LIST_PATH
echo "$JOBS_TO_RESUBMIT" | xargs -n 1 printf "$FILE_LIST_PATH/%b.txt $NEW_FILE_LIST_PATH/\n" | xargs -n 2 cp

#RunTools/submitTreeProducer_Batch.sh local Bari 0 $NEW_FILE_LIST_PATH $OUTPUT_PATH $GLOBAL_TAG $INCLUDE_SIM $PREFIX
RunTools/submitPatProducer_Batch.sh local Bari 0 $NEW_FILE_LIST_PATH $OUTPUT_PATH $GLOBAL_TAG $IS_MC $INCLUDE_SIM $PREFIX

#!/bin/bash
# Definition of a function that submit jobs on batch.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

function submit_batch {

    if [ $# -lt 5 ] ; then
        echo "Usage: queue storage job_name output_path script [script_args]"
        exit 1
    fi

    QUEUE=$1
    STORAGE=$2
    JOB_NAME=$3
    OUTPUT_PATH=$4
    RUN_SCRIPT_PATH=$5

    if [ ! -f "$RUN_SCRIPT_PATH" ] ; then
        echo "ERROR: script '$RUN_SCRIPT_PATH' does not exist."
        exit 1
    fi

    if [ "$QUEUE" = "cms" -a "$STORAGE" = "Pisa" ] ; then
        bsub -q $QUEUE -m austroh -E /usr/local/lsf/work/infn-pisa/scripts/testq-preexec-cms.bash \
             -J $JOB_NAME $RUN_SCRIPT_PATH ${@:6}
    elif [ "$QUEUE" = "local" -a "$STORAGE" = "Pisa" ] ; then
        bsub -q $QUEUE -m austroh -E /usr/local/lsf/work/infn-pisa/scripts/testq-preexec-cms.bash \
             -J $JOB_NAME $RUN_SCRIPT_PATH ${@:6}
    elif [ "$QUEUE" = "local" -a "$STORAGE" = "Bari" ] ; then
        echo "$RUN_SCRIPT_PATH ${@:6}" | qsub -q $QUEUE -N $JOB_NAME -o $OUTPUT_PATH -e $OUTPUT_PATH -
    elif [ "$QUEUE" = "fai5" -a "$STORAGE" = "Pisa" ] ; then
        bsub -Is -q $QUEUE -J $JOB_NAME $RUN_SCRIPT_PATH ${@:6} &
    elif [ "$QUEUE" = "fai" ] ; then
        bsub -Is -q $QUEUE -R "select[defined(fai)]" -J $JOB_NAME $RUN_SCRIPT_PATH ${@:6} &
    elif [ $STORAGE = "Local" ] ; then
        $RUN_SCRIPT_PATH ${@:6} &
    else
        echo "unknow queue"
        exit 1
    fi
}


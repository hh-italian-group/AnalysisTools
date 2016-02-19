#!/bin/bash
# Run data replica between sites using lcg-cp.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -ne 3 ] ; then
    echo "Usage: source destination directory"
    exit
fi

SOURCE_NAME=$1
DESTINATION_NAME=$2
DIRECTORY_NAME=$3

BARI_LOCAL_PATH=/lustre
MIN_SPEED=5000000 # B/s
NUMBER_OF_STREAMS=4

if [ "$SOURCE_NAME" = "T2_IT_Pisa" ] ; then
    SOURCE_PREFIX="srm://stormfe1.pi.infn.it:8444/srm/managerv2?SFN="
elif [ "$SOURCE_NAME" = "T2_IT_Bari" ] ; then
    SOURCE_PREFIX="srm://storm-se-01.ba.infn.it:8444/srm/managerv2?SFN="
else
    echo "ERROR: unknown source name '$SOURCE_NAME'."
    exit
fi

if [ "$DESTINATION_NAME" = "T2_IT_Pisa" ] ; then
    DESTINATION_PREFIX="srm://stormfe1.pi.infn.it:8444/srm/managerv2?SFN="
elif [ "$DESTINATION_NAME" = "T2_IT_Bari" ] ; then
    DESTINATION_PREFIX="srm://storm-se-01.ba.infn.it:8444/srm/managerv2?SFN="
else
    echo "ERROR: unknown destination name '$DESTINATION_NAME'."
    exit
fi

FULL_SOURCE_PATH=""
FULL_DESTINATION_PATH=""

LS_RESULT=$( lcg-ls -l $SOURCE_PREFIX/$DIRECTORY_NAME )
RESULT=$?
if [ $RESULT -ne 0 ] ; then
    echo "ERROR: directory '$DIRECTORY_NAME' not found in '$SOURCE_NAME'."
fi

while read FILE_DESC ; do
        if [ "$FILE_DESC" = "" ] ; then continue ; fi
        echo $FILE_DESC
    FILE_NAME=$( echo $FILE_DESC | awk '{print $7}' )
#    if [ "$SOURCE_NAME" = "T2_IT_Bari" ] ; then
#        FILE_SIZE=$( stat -c%s "$BARI_LOCAL_PATH/$FILE_NAME" )
#    else
        FILE_SIZE=$( echo $FILE_DESC | awk '{print $5}' )
#    fi
    FILE_NAME=$( echo $FILE_DESC | awk '{print $7}' )
    TIMEOUT=$(( $FILE_SIZE / $MIN_SPEED ))
    NEED_COPY=1
    while [ $NEED_COPY -ne 0 ] ; do
        LS_RESULT_DST=$( lcg-ls --srm-timeout=$TIMEOUT --connect-timeout=$TIMEOUT -l $DESTINATION_PREFIX/$DIRECTORY_NAME | grep $FILE_NAME )
        ITERATION=0
        while read FILE_DESC_DST ; do
            if [ $ITERATION -ne 0 ] ; then
                echo "ERROR: more than one file in destination for input 'FILE_NAME'"
                exit
            fi
                        if [ ! "$FILE_DESC_DST" = "" ] ; then
                                echo $FILE_DESC_DST
                                ITERATION=$(($ITERATION + 1))
                #if [ "$DESTINATION_NAME" = "T2_IT_Bari" ] ; then
                #    FILE_SIZE_DST=$( stat -c%s "$BARI_LOCAL_PATH/$FILE_NAME" )
                #else
                    FILE_SIZE_DST=$( echo $FILE_DESC_DST | awk '{print $5}' )
                #fi
                                echo "$FILE_SIZE $FILE_SIZE_DST"
                                if [ $FILE_SIZE -eq $FILE_SIZE_DST ] ; then
                                        echo "File $FILE_NAME is already transfered."
                                        NEED_COPY=0
                                else
                                        echo "Removing partially transfered file $FILE_NAME..."
                                        srmrm -2 $DESTINATION_PREFIX/$FILE_NAME
                                        if [ $? -ne 0 ] ; then
                                                echo "ERROR: can't remove file."
                                                exit
                                        fi
                                fi
                        fi
        done <<EOT
$LS_RESULT_DST
EOT
        if [ $NEED_COPY -ne 0 ] ; then
                        echo "Timeout: $TIMEOUT"
                        ( lcg-cp -b -D srmv2 -v --srm-timeout=$TIMEOUT --connect-timeout=$TIMEOUT -n $NUMBER_OF_STREAMS \
                                $SOURCE_PREFIX/$FILE_NAME $DESTINATION_PREFIX/$FILE_NAME ) & LCG_PID=$!
                ( trap 'kill $(jobs -p)' EXIT && sleep $TIMEOUT && kill -9 $LCG_PID ) & WATCHDOG_PID=$!
                    wait $LCG_PID
            LCG_RESULT=$?
                        #echo $( jobs -p )
                        kill $WATCHDOG_PID &> /dev/null
                WATCHDOG_KILLED=$?
            if [ $WATCHDOG_KILLED -eq 0 -a $LCG_RESULT -eq 0 ] ; then NEED_COPY=0 ; fi
                fi
#		exit
    done
#	exit
done <<EOT
$LS_RESULT
EOT

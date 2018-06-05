#!/bin/bash
# Submit job in a local queue.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

if [ $# -lt 4 ] ; then
    echo "Usage: queue job_name output_path exe [exe_args]"
    exit 1
fi

QUEUE="$1"
JOB_NAME="$2"
OUTPUT_PATH="$3"
EXE_NAME="$4"

if [[ $EXE_NAME =~ .*/.* ]] && [ -d "$(dirname "$EXE_NAME")" ] ; then
    EXE_NAME="$(cd "$(dirname "$EXE_NAME")" && pwd)/$(basename "$EXE_NAME")"
fi

command -v "$EXE_NAME" &> /dev/null
RESULT=$?
if [ $RESULT -ne 0 ] ; then
    echo "ERROR: can't find executable '$EXE_NAME'."
    exit 1
fi

mkdir -p "$OUTPUT_PATH"
if [ ! -d "$OUTPUT_PATH" ] ; then
    echo "ERROR: can't create output path '$OUTPUT_PATH'."
    exit 1
fi
cd "$OUTPUT_PATH"
OUTPUT_PATH="$(pwd)"

if [ -d "/gpfs/ddn" ] ; then SITE="Pisa"
elif [ -d "/lustre" ] ;  then SITE="Bari"
elif [ -d "/gwpool" ] ;  then SITE="Milan"
else SITE="CERN" ; fi

if [ $SITE = "Pisa" -o $SITE = "Bari" -o $SITE = "Milan" ] ; then
    SOURCE_LINE="source /cvmfs/cms.cern.ch/cmsset_default.sh"
else
    SOURCE_LINE=
fi

SCRIPT_NAME="run_job.sh"
SCRIPT_PATH="$PWD/$SCRIPT_NAME"
LOG_NAME="run_job.log"


function arg_to_str {
    local whitespace="[[:space:]]"
    ARG_STR=()
    for arg in "$@" ; do
        if [[ $arg =~ $whitespace ]] ; then
            arg=\"$arg\"
        fi
        ARG_STR+=("$arg")
    done
}

function determine_n_slots {
    local pattern='^[-]+n[_-](threads|processes|parallel)$'
    local next=0
    N_SLOTS=1
    for arg in "$@" ; do
        if (( $next == 1 )) ; then
            N_SLOTS=$arg
            break
        fi
        if [[ $arg =~ $pattern ]] ; then
            next=1
        fi
    done
}

arg_to_str "${@:5}"
determine_n_slots "${@:5}"

read -d '' SCRIPT << EOF
#!/bin/sh
cd "$OUTPUT_PATH"
echo "Job started at \$(date)." >> $LOG_NAME
$SOURCE_LINE
eval \$( scramv1 runtime -sh )
$EXE_NAME ${ARG_STR[@]} &>> $LOG_NAME
RESULT=\$\?
if [ \$RESULT -eq 0 ] ; then
    MSG="successfully ended"
else
    MSG="failed"
fi
echo "Job \$MSG at \$(date)." >> $LOG_NAME
EOF

echo "$SCRIPT" > "$SCRIPT_NAME"
chmod u+x "$SCRIPT_NAME"

echo "Job submission at $(date)." > $LOG_NAME

if [ $QUEUE = "interactive" ] ; then
    "$SCRIPT_PATH"
elif [ $SITE = "Pisa" ] ; then
    bsub -q "$QUEUE" -n $N_SLOTS -E "/usr/local/lsf/work/infn-pisa/scripts/testq-preexec-cms.bash" -J "$JOB_NAME" \
         "$SCRIPT_PATH" 2>&1 | tee -a $LOG_NAME
elif [ $SITE = "Bari" -o $SITE = "Milan" ] ; then
    qsub -q "$QUEUE" -N "$JOB_NAME" "$SCRIPT_PATH" 2>&1 | tee -a $LOG_NAME
else
    bsub -q "$QUEUE" -n $N_SLOTS -J "$JOB_NAME" "$SCRIPT_PATH" 2>&1 | tee -a $LOG_NAME
fi

#!/bin/bash
#source /afs/pi.infn.it/grid_exp_sw/cms/scripts/setcms.sh

export SCRAM_ARCH=slc6_amd64_gcc462
export VO_CMS_SW_DIR=/cvmfs/cms.cern.ch/
source $VO_CMS_SW_DIR/cmsset_default.sh
source $VO_CMS_SW_DIR/crab/CRAB_2_9_2/crab.sh

export CMSSW_GIT_REFERENCE=/gpfs/ddn/cms/user/androsov/.cmsgit-cache

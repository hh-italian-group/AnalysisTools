#!/bin/bash
# Setup analysis environment.
# This file is part of https://github.com/hh-italian-group/AnalysisTools.

#cmsenv
eval `scramv1 runtime -sh`
export CMSSW_ROOT_PATH=$( cd "$CMSSW_DATA_PATH/.." ; pwd )
export CMSSW_RELEASE_BASE_SRC="$CMSSW_RELEASE_BASE/src"
export CMSSW_BASE_SRC="$CMSSW_BASE/src"

export GCC_PATH=$( scram tool info gcc-ccompiler | grep GCC_CCOMPILER_BASE | sed s/GCC_CCOMPILER_BASE=// )
export GCC_INCLUDE_PATH=$( find "$GCC_PATH/include/c++/" -maxdepth 1 -type d | tail -n 1 )

export BOOST_BASE=$( scram tool info boost | grep BOOST_BASE | sed s/BOOST_BASE=// )
export BOOST_INCLUDE_PATH="$BOOST_BASE/include"
export ROOT_INCLUDE_PATH="$ROOTSYS/include"
export HEPMC_INCLUDE_PATH=$( scram tool info hepmc | grep HEPMC_BASE | sed s/HEPMC_BASE=// )/include

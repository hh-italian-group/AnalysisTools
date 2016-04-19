project(AnalysisTools)
cmake_minimum_required(VERSION 2.8)

set(AnalysisTools_DIR "${PROJECT_SOURCE_DIR}")
include("${AnalysisTools_DIR}/cmake/include/common.cmake")

file(GLOB OTHER_LIST "Run/srm_tools/*")
add_custom_target(others SOURCES ${OTHER_LIST})

add_library(AnalysisTools OBJECT ${HEADER_LIST})

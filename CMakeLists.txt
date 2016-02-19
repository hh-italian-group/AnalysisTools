project(AnalysisTools)
cmake_minimum_required(VERSION 2.8)

include("common.cmake")

file(GLOB OTHER_LIST "Run/srm_tools/*")
add_custom_target(others SOURCES ${OTHER_LIST})

add_library(AnalysisTools OBJECT ${HEADER_LIST})



project(AnalysisTools)
cmake_minimum_required(VERSION 3.6)

set(AnalysisTools_DIR "${PROJECT_SOURCE_DIR}")
include("${AnalysisTools_DIR}/cmake/include/common.cmake")

file(GLOB SRM_TOOLS_LIST "Run/srm_tools/*")

add_library(AnalysisTools STATIC ${SOURCE_LIST} "${RootDict}")
add_dependencies(AnalysisTools GenerateRootDict)
foreach(exe_name ${EXE_LIST})
    target_link_libraries("${exe_name}" AnalysisTools)
endforeach()

set_target_properties(Print_Histogram Print_Selection Print_SmartHistogram Print_TreeBranch PROPERTIES EXCLUDE_FROM_ALL 1)

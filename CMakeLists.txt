project(AnalysisTools)
cmake_minimum_required(VERSION 2.8)

include("common.cmake")

file(GLOB OTHER_LIST "Run/srm_tools/*")
add_custom_target(others SOURCES ${OTHER_LIST})

add_library(AnalysisTools OBJECT ${HEADER_LIST})

set(make_wrapper_script "${CMSSW_BASE_SRC}/AnalysisTools/Run/python/make_wrapper.py")
set(wrapper_template "${CMSSW_BASE_SRC}/AnalysisTools/Run/source/WrapperTemplate.cpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/wrappers")

foreach(exe_source ${EXE_SOURCE_LIST})
    execute_process(COMMAND basename ${exe_source} OUTPUT_VARIABLE exe_source_basename OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE ".cxx" "" exe_name "${exe_source_basename}")
    set(src_wrapper "wrappers/${exe_name}.cpp")
    add_custom_command(OUTPUT "${src_wrapper}"
                       COMMAND ${make_wrapper_script} "${wrapper_template}" "${exe_source}" "${src_wrapper}"
                       IMPLICIT_DEPENDS CXX "${exe_source}" "${wrapper_template}")
    message("Adding executable \"${exe_name}\"...")
    add_executable("${exe_name}" "${src_wrapper}")
    target_link_libraries("${exe_name}" "${all_libs}")
endforeach(exe_source ${EXE_SOURCE_LIST})

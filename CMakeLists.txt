project(AnalysisTools)
cmake_minimum_required(VERSION 2.6)

include("common.cmake")

file(GLOB OTHER_LIST "Run/srm_tools/*")
add_custom_target(others SOURCES ${OTHER_LIST})

add_library(AnalysisTools OBJECT ${HEADER_LIST})

set(make_wrapper_dir "${CMSSW_BASE_SRC}/AnalysisTools/Run/python")
set(make_wrapper_script "make_wrapper")
set(wrapper_template "${CMSSW_BASE_SRC}/AnalysisTools/Run/source/WrapperTemplate.cpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/wrappers")

foreach(exe_source ${EXE_SOURCE_LIST})
    execute_process(COMMAND basename ${exe_source} OUTPUT_VARIABLE exe_source_basename OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REPLACE ".cxx" "" exe_name "${exe_source_basename}")
    set(src_wrapper "${CMAKE_BINARY_DIR}/wrappers/${exe_name}.cpp")
    add_custom_command(OUTPUT "${src_wrapper}"
                       COMMAND python -c "import ${make_wrapper_script}; ${make_wrapper_script}.main( [\"${wrapper_template}\", \"${exe_source}\", \"${src_wrapper}\"] )"
                       IMPLICIT_DEPENDS CXX "${exe_source}" "${wrapper_template}"
                       WORKING_DIRECTORY "${make_wrapper_dir}"
                       VERBATIM)
    message("Adding executable \"${exe_name}\"...")
    add_executable("${exe_name}" "${src_wrapper}")
    target_link_libraries("${exe_name}" ${ALL_LIBS})
endforeach(exe_source ${EXE_SOURCE_LIST})

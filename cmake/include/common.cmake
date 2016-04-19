set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${AnalysisTools_DIR}/cmake/modules")

find_package(ROOT REQUIRED COMPONENTS Core Hist RIO Tree Physics Graf Gpad Matrix)
find_package(BoostEx REQUIRED COMPONENTS program_options filesystem regex system)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIR})
set(ALL_LIBS ${Boost_LIBRARIES} ${ROOT_LIBRARIES})

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "${Boost_LIBRARY_DIRS};${ROOT_LIBRARY_DIR}")

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O3")

execute_process(COMMAND root-config --incdir OUTPUT_VARIABLE ROOT_INCLUDE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)

find_file(scram_path "scram")
if(scram_path)
    set(CMSSW_RELEASE_BASE_SRC "$ENV{CMSSW_RELEASE_BASE}/src")
else()
    set(CMSSW_RELEASE_BASE_SRC "$ENV{CMSSW_RELEASE_BASE_SRC}")
endif()

include_directories(SYSTEM "${CMSSW_RELEASE_BASE_SRC}")

get_filename_component(CMSSW_BASE_SRC "${AnalysisTools_DIR}" DIRECTORY)
include_directories("${CMSSW_BASE_SRC}" "${AnalysisTools_DIR}/Core/include" "${PROJECT_SOURCE_DIR}")

file(GLOB_RECURSE HEADER_LIST "*.h" "*.hh")
add_custom_target(headers SOURCES ${HEADER_LIST})

file(GLOB_RECURSE SOURCE_LIST "*.cxx" "*.C" "*.cpp" "*.cc")
add_custom_target(sources SOURCES ${SOURCE_LIST})

file(GLOB_RECURSE EXE_SOURCE_LIST "*.cxx")

file(GLOB_RECURSE SCRIPT_LIST "*.sh" "*.py")
add_custom_target(scripts SOURCES ${SCRIPT_LIST})

file(GLOB_RECURSE CONFIG_LIST "*.cfg" "*.xml")
add_custom_target(configs SOURCES ${CONFIG_LIST})

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O3")

set(make_wrapper_dir "${AnalysisTools_DIR}/Run/python")
set(make_wrapper_script "make_wrapper")
set(wrapper_template "${AnalysisTools_DIR}/Run/source/WrapperTemplate.cpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/wrappers")

set(EXE_LIST)
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
    list(APPEND EXE_LIST "${exe_name}")
endforeach()


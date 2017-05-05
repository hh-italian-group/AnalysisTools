set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${AnalysisTools_DIR}/cmake/modules")

find_package(ROOT REQUIRED COMPONENTS Core Hist RIO Tree Physics Graf Gpad Matrix MathCore GenVector TMVA)
find_package(BoostEx REQUIRED COMPONENTS program_options filesystem regex system thread)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIR})
set(ALL_LIBS ${Boost_LIBRARIES} ${ROOT_LIBRARIES} pthread)

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "${Boost_LIBRARY_DIRS};${ROOT_LIBRARY_DIR}")

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

file(GLOB_RECURSE CONFIG_LIST "*.cfg" "*.xml" "*.txt")
add_custom_target(configs SOURCES ${CONFIG_LIST})

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -O3")

set(LinkDef "${AnalysisTools_DIR}/Core/include/LinkDef.h")
set(RootDict "${CMAKE_BINARY_DIR}/RootDictionaries.cpp")
set(RootDictIncludes "Math/LorentzVector.h" "Math/PtEtaPhiM4D.h" "Math/PtEtaPhiE4D.h" "Math/PxPyPzM4D.h")
add_custom_command(OUTPUT "${RootDict}"
                   COMMAND rootcling -f "${RootDict}" ${RootDictIncludes} "${LinkDef}"
                   IMPLICIT_DEPENDS CXX "${LinkDef}"
                   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                   VERBATIM)
set(EXE_LIST)
foreach(exe_source ${EXE_SOURCE_LIST})
    get_filename_component(exe_name "${exe_source}" NAME_WE)
    message("Adding executable \"${exe_name}\"...")
    add_executable("${exe_name}" "${exe_source}" "${RootDict}")
    target_link_libraries("${exe_name}" ${ALL_LIBS})
    list(APPEND EXE_LIST "${exe_name}")
endforeach()


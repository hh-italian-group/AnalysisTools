set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${AnalysisTools_DIR}/cmake/modules")

find_package(ROOT REQUIRED COMPONENTS Core Hist RIO Tree Physics Graf Gpad Matrix MathCore GenVector TMVA RooFitCore RooFit ROOTDataFrame TreePlayer)
find_package(BoostEx REQUIRED COMPONENTS program_options filesystem regex system thread iostreams unit_test_framework stacktrace_addr2line)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${ROOT_INCLUDE_DIR})
set(ALL_LIBS ${Boost_LIBRARIES} ${ROOT_LIBRARIES} pthread dl)

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
file(GLOB_RECURSE SOURCE_LIST "*.cpp")
file(GLOB_RECURSE EXE_SOURCE_LIST "*.cxx")
file(GLOB_RECURSE SCRIPT_LIST "*.sh" "*.py")
list(FILTER SCRIPT_LIST EXCLUDE REGEX "/__init__\\.py$")
file(GLOB_RECURSE CONFIG_LIST "*.cfg" "*.xml" "*.txt" "*.md")

set(CXX_WARNING_FLAGS "-Weverything -Wno-missing-prototypes -Wno-unused-member-function -Wno-weak-vtables \
                       -Wno-documentation -Wno-gnu-zero-variadic-macro-arguments -Wno-global-constructors \
                       -Wno-exit-time-destructors -Wno-newline-eof -Wno-c++98-compat-pedantic \
                       -Wno-c++98-compat -Wno-disabled-macro-expansion -Wno-system-headers -Wno-unused-macros \
                       -Wno-shadow -Wno-unknown-pragmas -Wno-format-nonliteral -Wno-double-promotion -Wno-float-equal \
                       -Wno-padded -Wno-missing-braces -Wno-used-but-marked-unused")
# -ftime-report
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
	set(CXX_STD_FLAG "-std=c++17")
else()
	set(CXX_STD_FLAG "-std=c++14")
endif()

set(CXX_COMMON_FLAGS "${CXX_STD_FLAG} -pedantic ${CXX_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CXX_COMMON_FLAGS} -O3 -gline-tables-only")

set(LinkDef "${AnalysisTools_DIR}/Core/include/LinkDef.h")
set(RootDict "${CMAKE_BINARY_DIR}/RootDictionaries.cpp")
set(RootDictIncludes "Math/LorentzVector.h" "Math/PtEtaPhiM4D.h" "Math/PtEtaPhiE4D.h" "Math/PxPyPzM4D.h")
add_custom_command(OUTPUT "${RootDict}"
                   COMMAND rootcling -f "${RootDict}" ${RootDictIncludes} "${LinkDef}"
                   IMPLICIT_DEPENDS CXX "${LinkDef}"
                   WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                   VERBATIM)
set_source_files_properties(${RootDict} PROPERTIES COMPILE_FLAGS "-w")
add_custom_target(GenerateRootDict DEPENDS "${RootDict}")
set(EXE_LIST)
set(TEST_LIST)
foreach(exe_source ${EXE_SOURCE_LIST})
    get_filename_component(exe_name "${exe_source}" NAME_WE)
    message("Adding executable \"${exe_name}\"...")
    add_executable("${exe_name}" "${exe_source}" "${RootDict}")
    target_link_libraries("${exe_name}" ${ALL_LIBS})
    if(exe_name MATCHES "_t$")
        list(APPEND TEST_LIST "${exe_name}")
    endif()
    list(APPEND EXE_LIST "${exe_name}")
endforeach()

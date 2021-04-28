project(libllmc)

include(CheckIncludeFileCXX)
include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
    libfrugi
    GIT_REPOSITORY https://github.com/bergfi/libfrugi.git
    GIT_TAG 93deb6ad08818c99d4a2ddf520f12527bb3dd542
)
FetchContent_Declare(
    dtree
    GIT_REPOSITORY https://github.com/bergfi/dtree.git
    GIT_TAG 861f50b642bc4b1cd6af6d60d91be68e2b77b7dc
)
FetchContent_Declare(
    dmc
    GIT_REPOSITORY https://github.com/bergfi/dmc.git
    GIT_TAG ee509890076baea158432907de36a0331ffd2640
)

FetchContent_MakeAvailable(libfrugi dtree dmc)

if(NOT USE_SYSTEM_LLVM)
    set(CMAKE_BUILD_TYPE_OLD ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE INTERNAL "Build type")
    set(CMAKE_CONFIGURATION_TYPES_OLD ${CMAKE_CONFIGURATION_TYPES})
    set(CMAKE_CONFIGURATION_TYPES "" CACHE INTERNAL "Build configurations")
    set(LLVM_BUILD_TOOLS ON CACHE INTERNAL "Build LLVM tools")
    set(LLVM_INCLUDE_TOOLS ON CACHE INTERNAL "Include LLVM tools")
    set(LLVM_INCLUDE_TESTS OFF CACHE INTERNAL "Build LLVM tests")
    set(LLVM_INCLUDE_BENCHMARKS OFF CACHE INTERNAL "Build LLVM benchmarks")
    set(LLVM_INCLUDE_EXAMPLES OFF CACHE INTERNAL "Build LLVM examples")
    set(LLVM_TARGETS_TO_BUILD "X86" CACHE INTERNAL "LLVM Targets")
    message("Downloading LLVM...")
    FetchContent_Declare(
        llvmproject
        GIT_REPOSITORY https://github.com/llvm/llvm-project.git
        GIT_TAG release/11.x
        GIT_PROGRESS TRUE
        SOURCE_SUBDIR llvm
    )
    FetchContent_MakeAvailable(llvmproject)
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_OLD} CACHE STRING "Build type" FORCE)
    set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES_OLD} CACHE STRING "Build configurations" FORCE)
#    set(LLVM_DIR "${FETCHCONTENT_BINARY_DIR_LLVMPROJECT}/llvm/cmake")
#    set(CMAKE_MODULE_PATH "${FETCHCONTENT_SOURCE_DIR_LLVMPROJECT}/llvm/cmake/modules;${CMAKE_MODULE_PATH}")
#    execute_process(
#        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" ${llvmproject_SOURCE_DIR}/llvm
#        WORKING_DIRECTORY ${llvmproject_BINARY_DIR}
#    )
#    set(LLVM_ROOT "${llvmproject_SOURCE_DIR}/llvm/cmake/modules")
    set(LLVM_ROOT "${llvmproject_BINARY_DIR}/lib/cmake/llvm")
#    ExternalProject_Add(llvmproject
#        GIT_REPOSITORY https://github.com/llvm/llvm-project.git
#        GIT_TAG        master
#        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
#    )
    message("ZOMGGGGGGGGGGGG ${LLVM_ROOT}")
endif()

find_package(LLVM REQUIRED CONFIG PATHS ${llvmproject_BINARY_DIR}/lib/cmake/llvm)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_FLAGS} -fno-rtti -fexceptions")
llvm_map_components_to_libnames(LLVM_LIBS core engine interpreter irreader jitlink)
#set (LLVM_LIBS "LLVMCore;LLVMInterpreter;LLVMIRReader;LLVMJITLink;LLVMX86CodeGen;LLVMX86AsmParser;LLVMX86Desc;LLVMX86Disassembler;LLVMX86Info")
#set (LLVM_LIBS "LLVMIRReader LLVMAsmParser LLVMInterpreter LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen LLVMCFGuard LLVMGlobalISel LLVMSelectionDAG LLVMAsmPrinter LLVMDebugInfoDWARF LLVMCodeGen LLVMScalarOpts LLVMInstCombine LLVMAggressiveInstCombine LLVMTransformUtils LLVMBitWriter LLVMX86Desc LLVMMCDisassembler LLVMX86Info LLVMMCJIT LLVMExecutionEngine LLVMTarget LLVMAnalysis LLVMProfileData LLVMRuntimeDyld LLVMObject LLVMTextAPI LLVMMCParser LLVMBitReader LLVMMC LLVMDebugInfoCodeView LLVMDebugInfoMSF LLVMCore LLVMRemarks LLVMBitstreamReader LLVMBinaryFormat LLVMSupport LLVMDemangle")
set(LLVM_LIBS "LLVMLinker;LLVMIRReader;LLVMAsmParser;LLVMInterpreter;LLVMX86Disassembler;LLVMX86AsmParser;LLVMX86CodeGen;LLVMCFGuard;LLVMGlobalISel;LLVMSelectionDAG;LLVMAsmPrinter;LLVMDebugInfoDWARF;LLVMCodeGen;LLVMScalarOpts;LLVMInstCombine;LLVMAggressiveInstCombine;LLVMTransformUtils;LLVMBitWriter;LLVMX86Desc;LLVMMCDisassembler;LLVMX86Info;LLVMMCJIT;LLVMExecutionEngine;LLVMTarget;LLVMAnalysis;LLVMProfileData;LLVMRuntimeDyld;LLVMObject;LLVMTextAPI;LLVMMCParser;LLVMBitReader;LLVMMC;LLVMDebugInfoCodeView;LLVMDebugInfoMSF;LLVMCore;LLVMRemarks;LLVMBitstreamReader;LLVMBinaryFormat;LLVMSupport;LLVMDemangle")
#set(LLVM_LIBS "${LLVM_LIBRARIES}")
#set(LLVM_LIBS "LLVM")
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "  Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "  - LLVM include:     ${LLVM_INCLUDE_DIRS}")
message(STATUS "  - LLVM libs:        ${LLVM_LIBS}")
message(STATUS "  - LLVM Definitions: ${LLVM_DEFINITIONS}")
message(STATUS "  - LLVM Flags:       ${LLVM_FLAGS}")

set(WITH_LTSMIN "" CACHE FILEPATH "Path to LTSmin install directory")

#include_directories(${LLVM_INCLUDE_DIRS})
#add_definitions(${LLVM_DEFINITIONS})

if(NOT CLANGEXEC)
    message(STATUS "Looking for clang")
    find_program(CLANGEXEC clang)
    if(NOT CLANGEXEC)
        message(STATUS "Looking for clang - not found (needed to compile LLMC OS)")
        message(SEND_ERROR "Looking for clang - not found (needed to compile LLMC OS)")
    else()
        message(STATUS "Looking for clang - found (${CLANGEXEC})")
    endif()
endif()

add_library(libllmc
    generation/StateManager.cpp
    generation/GenerationContext.cpp
    generation/LLVMLTS.cpp
    generation/ProcessStack.cpp
    generation/TransitionGroups.cpp
    LLDMCModelGenerator.cpp
    llvmgen.cpp
    )
set_target_properties(libllmc PROPERTIES OUTPUT_NAME "llmc")
set_property(TARGET libllmc PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libllmc PROPERTY CXX_STANDARD 14)
set_property(TARGET libllmc PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET libllmc PROPERTY VERSION ${llmc_VERSION})
set_property(TARGET libllmc PROPERTY SOVERSION ${llmc_VERSION_MAJOR})
set_property(TARGET libllmc PROPERTY DEBUG_POSTFIX d)

# FFI library
if(NOT HAVE_FFI)
    message(STATUS "Looking for ffi")
    FIND_LIBRARY(FFI_LIBRARY ffi)
    if(FFI_LIBRARY)
        message(STATUS "Looking for ffi - found")
        set(HAVE_FFI 1)
    else()
        #message(SEND_ERROR "Looking for ffi - not found")
    endif()
endif()

target_include_directories(libllmc
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
    PUBLIC ${LLVM_INCLUDE_DIRS}
    )

target_compile_definitions(libllmc
    PUBLIC ${LLVM_DEFINITIONS}
    )

target_link_libraries(libllmc
    PUBLIC -Wl,-E -fPIC ${FFI_LIBRARY} ${LLVM_LIBS} tinfo libdmc libfrugi
    -udmc_insert
    -udmc_insertB
    -udmc_transition
    -udmc_delta
    -udmc_deltaB
    -udmc_get
    -udmc_getpart
    -udmc_getpartB
    )

# PINS LLVM header
#add_custom_command(OUTPUT pins.h.ll
#    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll pins.h.ll
#    DEPENDS pins.h.ll.input
#    COMMENT "Copying PINS LLVM Header"
#    )
#add_custom_command(
#    OUTPUT pins.h.ll
#    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll
#)
#add_custom_target(pinsllvmheader ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll COPYONLY)

#add_custom_command(
#        OUTPUT dmcapi.h.ll
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_SOURCE_DIR}/dmcapi.h.ll
#        ${CMAKE_CURRENT_BINARY_DIR}/dmcapi.h.ll
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dmcapi.h.ll
#)
#add_custom_target(dmcapillvmheader ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dmcapi.h.ll)

#add_library(libdmccapi STATIC
#        libdmccapi.cpp
#        )
#set_target_properties(libdmccapi PROPERTIES OUTPUT_NAME "dmccapi")
#set_property(TARGET libdmccapi PROPERTY POSITION_INDEPENDENT_CODE ON)
#set_property(TARGET libdmccapi PROPERTY CXX_STANDARD 11)
#set_property(TARGET libdmccapi PROPERTY CXX_STANDARD_REQUIRED ON)
#
#target_include_directories(libdmccapi
#    PRIVATE include ../llmc/include
#)
#
## LLMC VM library
#add_custom_command(OUTPUT libdmccapi.ll
#        COMMAND ${CLANGEXEC} ${CMAKE_CURRENT_SOURCE_DIR}/libdmccapi.cpp -S -emit-llvm -o libdmccapi.ll -I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_SOURCE_DIR}/../common -I${CMAKE_CURRENT_SOURCE_DIR}/../llmc/include -I${LTSMIN_INCLUDE_DIRS}
#        DEPENDS libdmccapi.cpp
#        COMMENT "Compiling DMC C API to libdmccapi.ll"
#        )
#add_custom_target(libdmccapill ALL SOURCES libdmccapi.ll)

get_target_property(DMCCAPI_INCLUDES libdmc INCLUDE_DIRECTORIES)
set(DMCCAPI_INCLUDES_LINE "")
foreach(subdir ${DMCCAPI_INCLUDES})
    set(DMCCAPI_INCLUDES_LINE ${DMCCAPI_INCLUDES_LINE} -I ${subdir})
endforeach()
add_custom_command(OUTPUT libdmccapi.ll
    COMMAND ${CLANGEXEC} -fPIC ${CMAKE_CURRENT_SOURCE_DIR}/dmcapi.c -S -emit-llvm -femit-all-decls -o libdmccapi.ll -I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_SOURCE_DIR}/../common -I${CMAKE_CURRENT_SOURCE_DIR}/../llmc/include ${DMCCAPI_INCLUDES_LINE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dmcapi.c
    COMMENT "Compiling DMC C API to libdmccapi.ll"
    )
add_custom_target(libdmccapill ALL SOURCES libdmccapi.ll)

#add_library(libdmccapill OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/dmcapi.cpp)
#target_link_libraries(libdmccapill PUBLIC libdmc)
#target_compile_options(libdmccapill PUBLIC "-S -emit-llvm")

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll DESTINATION share/llmc)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libdmccapi.ll DESTINATION share/llmc)

add_dependencies(libllmc libdmccapill libllmcosll libllmcvmll)

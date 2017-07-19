#project (llmc)
cmake_minimum_required(VERSION 3.2)
set (llmc_VERSION_MAJOR 0)
set (llmc_VERSION_MINOR 1)
set (llmc_VERSION_PATCH 0)

include(CheckIncludeFile)

# Set build-specific flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_C_FLAGS_PROFILE "-pg")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PROFILE "-pg")

find_package(LLVM REQUIRED CONFIG)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_FLAGS} -fno-rtti -fexceptions")
llvm_map_components_to_libnames(LLVM_LIBS core interpreter irreader)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "  Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "  - LLVM include:     ${LLVM_INCLUDE_DIRS}")
message(STATUS "  - LLVM libs:        ${LLVM_LIBS}")
message(STATUS "  - LLVM Definitions: ${LLVM_DEFINITIONS}")
message(STATUS "  - LLVM Flags:       ${LLVM_FLAGS}")

set(WITH_LTSMIN "" CACHE FILEPATH "Path to LTSmin install directory")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Find pins.h
if(WITH_LTSMIN)
    if(EXISTS "${WITH_LTSMIN}/include/ltsmin/pins.h")
        set(CMAKE_REQUIRED_INCLUDES "${WITH_LTSMIN}/include")
        set(LTSMIN_INCLUDE_DIRS "${WITH_LTSMIN}/include")
    else()
        message(SEND_ERROR "${WITH_LTSMIN} does not contain ltsmin/pins.h")
    endif()
else()
    find_path(LTSMIN_INCLUDE_DIRS ltsmin/pins.h)
endif()
if(NOT HAVE_PINS_H)
    check_include_file("ltsmin/pins.h" HAVE_PINS_H)
endif()
set(CMAKE_REQUIRED_INCLUDES)
if(NOT HAVE_PINS_H)
    message(SEND_ERROR "Looking for ltsmin/pins.h - not found")
endif()

add_definitions(-Wall)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/common
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp/include
	${CMAKE_CURRENT_SOURCE_DIR}/libfrugi/include
	${LTSMIN_INCLUDE_DIRS}
)

#add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)
add_subdirectory(libfrugi EXCLUDE_FROM_ALL)
#add_subdirectory(bcloader EXCLUDE_FROM_ALL)
add_subdirectory(ll2pins)
add_subdirectory(llmc)
add_subdirectory(libllmc)
add_subdirectory(libllmcos)

message(STATUS "Building configuration: ${CMAKE_BUILD_TYPE}")
message(STATUS "    C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  CXX flags: ${CMAKE_CXX_FLAGS}")

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/common/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/common/config.h"
  )

if(FALSE)
    file(GLOB_RECURSE project_headers FOLLOW_SYMLINKS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
            message("project_headers: ${project_headers}")
    get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    set(CMAKE_REQUIRED_INCLUDES "${dirs}")
    set(CMAKE_REQUIRED_QUIET ON)
    set(CMAKE_REQUIRED_FLAGS "-c")
    foreach(project_header ${project_headers})
        get_filename_component(header_name ${project_header} NAME_WE)
        check_include_file_cxx("${project_header}" ${header_name}_IS_STANDALONE)
        if(NOT ${header_name}_IS_STANDALONE)
            message("${header_name} is not standalone")
        else()
            message("${header_name} is standalone")
        endif()
    endforeach()
    set(CMAKE_REQUIRED_QUIET)
    set(CMAKE_REQUIRED_INCLUDES)
    set(CMAKE_REQUIRED_FLAGS)
endif()

cmake_minimum_required(VERSION 3.19)
project(llmc)
include(CheckIncludeFile)

set (llmc_VERSION_MAJOR 0)
set (llmc_VERSION_MINOR 2)
set (llmc_VERSION_PATCH 0)
set(llmc_VERSION "${llmc_VERSION_MAJOR}.${llmc_VERSION_MINOR}.${llmc_VERSION_PATCH}")

set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

if(WIN32 AND NOT CYGWIN)
    set(DEF_INSTALL_CMAKE_DIR CMake)
else()
    set(DEF_INSTALL_CMAKE_DIR lib/cmake/llmc)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mcx16")

# Set build-specific flags
set(CMAKE_C_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_C_FLAGS_PROFILE "-pg")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -UNDEBUG -DYYDEBUG=1 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_PROFILE "-pg")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g ${CMAKE_CXX_FLAGS_RELEASE}")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO} -fno-omit-frame-pointer -fsanitize=address")

add_definitions(-Wall)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_BINARY_DIR}/common
)

add_subdirectory(ll2dmc)
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

project (libllmcos)

include(CheckIncludeFileCXX)

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

add_library(libllmcos STATIC
    libllmcos.c
)
set_target_properties(libllmcos PROPERTIES OUTPUT_NAME "llmcos")
set_property(TARGET libllmcos PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libllmcos PROPERTY CXX_STANDARD 11)
set_property(TARGET libllmcos PROPERTY CXX_STANDARD_REQUIRED ON)

# LLMC OS library
add_custom_command(OUTPUT libllmcos.ll
    COMMAND ${CLANGEXEC} ${CMAKE_CURRENT_SOURCE_DIR}/libllmcos.c -S -emit-llvm -o libllmcos.ll -I${LTSMIN_INCLUDE_DIRS}
    DEPENDS libllmcos.c
    COMMENT "Compiling LLMC OS to libllmcos.ll"
    )
add_custom_target(libllmcosll ALL SOURCES libllmcos.ll)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libllmcos.a DESTINATION share/llmc)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/libllmcos.ll DESTINATION share/llmc)


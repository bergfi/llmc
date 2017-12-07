project (libllmc)

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

add_library(libllmc
    LLPinsGenerator.cpp
    llvmgen.cpp
)
set_target_properties(libllmc PROPERTIES OUTPUT_NAME "llmc")
set_property(TARGET libllmc PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET libllmc PROPERTY CXX_STANDARD 11)
set_property(TARGET libllmc PROPERTY CXX_STANDARD_REQUIRED ON)

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

target_link_libraries(libllmc -Wl,-E
    ${FFI_LIBRARY}
    ${LLVM_LIBS}
    tinfo
    libfrugi
)

# PINS LLVM header
#add_custom_command(OUTPUT pins.h.ll
#    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll pins.h.ll
#    DEPENDS pins.h.ll.input
#    COMMENT "Copying PINS LLVM Header"
#    )
add_custom_command(
    OUTPUT pins.h.ll
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll
        ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll
    )
add_custom_target(pinsllvmheader ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll)
#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pins.h.ll ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll COPYONLY)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/pins.h.ll DESTINATION share/llmc)


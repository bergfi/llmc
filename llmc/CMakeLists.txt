project (llmc)

include(CheckIncludeFileCXX)

add_executable(llmc
    main.cpp
    ssgen.cpp
    pins/pinsWrapper.cpp
)
set_property(TARGET llmc PROPERTY CXX_STANDARD 17)
set_property(TARGET llmc PROPERTY CXX_STANDARD_REQUIRED ON)

add_dependencies(llmc dtree)
add_dependencies(llmc libtreedbs)

target_link_libraries(llmc -Wl,-E -fPIC
    libllmc
    libfrugi
    dtree
    libtreedbs
)

include_directories(
    include
    ../libllmc/include
)

install (TARGETS llmc DESTINATION bin)

add_subdirectory(tests)
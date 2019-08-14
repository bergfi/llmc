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

target_link_libraries(llmc -Wl,-E -fPIC
    libllmc
    libfrugi
    dtree
)

include_directories(
    include
)

install (TARGETS llmc DESTINATION bin)

add_subdirectory(tests)
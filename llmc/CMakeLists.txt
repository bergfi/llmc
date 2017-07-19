project (llmc)

include(CheckIncludeFileCXX)

add_executable(llmc
    main.cpp
)
set_property(TARGET llmc PROPERTY CXX_STANDARD 11)
set_property(TARGET llmc PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(llmc -Wl,-E
    libllmc
    libfrugi
)

install (TARGETS llmc DESTINATION bin)


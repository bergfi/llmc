include(CheckIncludeFileCXX)

add_executable(llmc
    main.cpp
    )

set_property(TARGET llmc PROPERTY CXX_STANDARD 17)
set_property(TARGET llmc PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET llmc PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(llmc libllmc)

target_link_libraries(llmc
    PUBLIC -Wl,-E libllmc
    )

target_include_directories(llmc
    PUBLIC
    include
    ../libllmc/include
    ../dmc/include
    ${LLVM_INCLUDE_DIRS}
    )

install(TARGETS llmc DESTINATION bin)

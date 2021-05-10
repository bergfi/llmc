project (ll2dmc)

include(CheckIncludeFileCXX)

add_executable(ll2dmc
    main.cpp
)
set_property(TARGET ll2dmc PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET ll2dmc PROPERTY CXX_STANDARD 14)
set_property(TARGET ll2dmc PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(ll2dmc PUBLIC
    libllmc
    libfrugi
)

install (TARGETS ll2dmc DESTINATION bin)


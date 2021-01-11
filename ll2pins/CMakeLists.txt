project (ll2pins)

include(CheckIncludeFileCXX)

add_executable(ll2pins
    main.cpp
)
set_property(TARGET ll2pins PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET ll2pins PROPERTY CXX_STANDARD 14)
set_property(TARGET ll2pins PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(ll2pins -Wl,-E
    libllmc
    libfrugi
)

install (TARGETS ll2pins DESTINATION bin)


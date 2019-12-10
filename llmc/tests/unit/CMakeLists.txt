include(GoogleTest)

add_executable(storage_test
    storage_test.cpp
)

add_definitions(-fPIC)
add_dependencies(llmc libtreedbs)

gtest_discover_tests(storage_test)
target_link_libraries(storage_test -fPIC
        gtest gmock gtest_main
        -lpthread -lm
        libtreedbs
)
add_test(storage_test storage_test)

#target_link_libraries(storage_test ${GTEST_LIBRARY_DEBUG} ${GTEST_MAIN_LIBRARY_DEBUG})
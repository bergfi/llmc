include(GoogleTest)

add_executable(storage_test
    storage_test.cpp
)

gtest_discover_tests(storage_test)
target_link_libraries(storage_test gtest gmock gtest_main)
add_test(storage_test storage_test)

#target_link_libraries(storage_test ${GTEST_LIBRARY_DEBUG} ${GTEST_MAIN_LIBRARY_DEBUG})
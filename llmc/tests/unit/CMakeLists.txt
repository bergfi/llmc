include(GoogleTest)

add_executable(storage_test
    storage_test.cpp
)
set_property(TARGET storage_test PROPERTY CXX_STANDARD 17)
set_property(TARGET storage_test PROPERTY CXX_STANDARD_REQUIRED ON)

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

add_executable(model_test
        model_test.cpp
        )
set_property(TARGET model_test PROPERTY CXX_STANDARD 17)
set_property(TARGET model_test PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
add_dependencies(llmc libtreedbs libfrugi)

gtest_discover_tests(model_test)
target_link_libraries(model_test -fPIC
        gtest gmock gtest_main
        -lpthread -lm libfrugi
        libtreedbs
        )
add_test(model_test model_test)

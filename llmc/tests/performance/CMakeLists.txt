#include(GoogleTest)

add_executable(model_msq
        model_msq.cpp
        )
set_property(TARGET model_msq PROPERTY CXX_STANDARD 17)
set_property(TARGET model_msq PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(model_msq -fPIC
#        gtest gmock gtest_main
        -lpthread -lm libfrugi
        libtreedbs
        -latomic
        )
#add_test(model_msq model_msq)


add_executable(model_philo
        model_philo.cpp
        )
set_property(TARGET model_philo PROPERTY CXX_STANDARD 17)
set_property(TARGET model_philo PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
#add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(model_philo -fPIC
        #        gtest gmock gtest_main
        -lpthread -lm libfrugi
        libtreedbs
        -latomic
        )
add_executable(model_bakery
        model_bakery.cpp
        )

set_property(TARGET model_bakery PROPERTY CXX_STANDARD 17)
set_property(TARGET model_bakery PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
#add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(model_bakery -fPIC
        #        gtest gmock gtest_main
        -lpthread -lm libfrugi
        libtreedbs
        -latomic
        )

add_executable(model_bakery_fixed
        model_bakery_fixed.cpp
        )

set_property(TARGET model_bakery_fixed PROPERTY CXX_STANDARD 17)
set_property(TARGET model_bakery_fixed PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
#add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(model_bakery_fixed -fPIC
        #        gtest gmock gtest_main
        -lpthread -lm libfrugi
        libtreedbs
        -latomic
        )

add_executable(model_promela
        model_promela.cpp
        )

set_property(TARGET model_promela PROPERTY CXX_STANDARD 17)
set_property(TARGET model_promela PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
#add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(model_promela -fPIC
        #        gtest gmock gtest_main
        -lpthread -lm libfrugi
        libtreedbs
        -latomic
        )

add_executable(dmc
        dmc.cpp
        )

set_property(TARGET dmc PROPERTY CXX_STANDARD 17)
set_property(TARGET dmc PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
#add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(dmc -fPIC
        #        gtest gmock gtest_main
        -lpthread -lm libfrugi -ldl
        libtreedbs
        -latomic
        )

add_executable(model_hashmap
        model_hashmap.cpp
        )

set_property(TARGET model_hashmap PROPERTY CXX_STANDARD 17)
set_property(TARGET model_hashmap PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions(-fPIC)
#add_dependencies(llmc libtreedbs libfrugi)

#gtest_discover_tests(model_msq)
target_link_libraries(model_hashmap -fPIC
        #        gtest gmock gtest_main
        -lpthread -lm libfrugi -ldl
        libtreedbs
        -latomic
        )

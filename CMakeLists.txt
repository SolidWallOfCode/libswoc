cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)

project(libswoc)

include_directories(include)

add_library(libswoc++ STATIC
    include/swoc/TextView.h
    src/TextView.cc
    )

add_executable(test_libswoc
    src/unit_tests/unit_test_main.cc
    src/unit_tests/test_TextView.cc
    )

target_link_libraries(test_libswoc libswoc++)

set_target_properties(libswoc++ PROPERTIES PUBLIC_HEADER include/swoc/TextView.h)
install(TARGETS libswoc++
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/swoc
    )

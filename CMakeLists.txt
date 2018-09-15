cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)

project(libswoc)

include_directories(include)

set(public_headers
    include/swoc/Scalar.h
    include/swoc/TextView.h
    include/swoc/MemSpan.h
    include/swoc/MemArena.h
    include/swoc/BufferWriter.h
    include/swoc/bwf_base.h
    include/swoc/bwf_std.h
)

set(lib_src
    src/TextView.cc
    src/BufferWriterFormat.cc
)

add_library(libswoc++ STATIC "${public_headers}" "${lib_src}")

add_executable(test_libswoc
    src/unit_tests/unit_test_main.cc
    src/unit_tests/test_TextView.cc
)

target_link_libraries(test_libswoc libswoc++)

set_target_properties(libswoc++ PROPERTIES
    PUBLIC_HEADER "${public_headers}"
)

install(TARGETS libswoc++
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/swoc
)

add_custom_target(clang-format-src COMMAND tools/clang-format.sh src)
add_custom_target(clang-format-include COMMAND tools/clang-format.sh include/swoc)

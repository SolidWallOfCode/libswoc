cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)

project(libswoc)

include_directories(include)

set(public_headers
    include/swoc/swoc_common.h
    include/swoc/swoc_meta.h
    include/swoc/Scalar.h
    include/swoc/TextView.h
    include/swoc/IntrusiveDList.h
    include/swoc/MemSpan.h
    include/swoc/MemArena.h
    include/swoc/BufferWriter.h
    include/swoc/bwf_base.h
    include/swoc/bwf_std.h
    include/swoc/bwf_ex.h
    include/swoc/bwf_swoc.h
)

set(lib_src
    src/TextView.cc
    src/MemArena.cc
    src/BufferWriterFormat.cc
)

add_library(libswoc++ STATIC "${public_headers}" "${lib_src}")

add_executable(test_libswoc
    src/unit_tests/unit_test_main.cc
    src/unit_tests/test_TextView.cc
    src/unit_tests/test_MemSpan.cc
    src/unit_tests/test_MemArena.cc
    src/unit_tests/test_BufferWriter.cc
    src/unit_tests/test_BufferWriterFormat.cc
    src/unit_tests/test_IntrusiveDList.cc
    src/unit_tests/test_Scalar.cc
)

find_library(POSIX_THREAD_LIB pthread)
target_link_libraries(test_libswoc libswoc++ ${POSIX_THREAD_LIB})

set_target_properties(libswoc++ PROPERTIES
    PUBLIC_HEADER "${public_headers}"
)

install(TARGETS libswoc++
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/swoc
)

add_custom_target(clang-format-src COMMAND ${CMAKE_HOME_DIRECTORY}/tools/clang-format.sh ${CMAKE_HOME_DIRECTORY}/src)
add_custom_target(clang-format-include COMMAND ${CMAKE_HOME_DIRECTORY}/tools/clang-format.sh ${CMAKE_HOME_DIRECTORY}/include/swoc)

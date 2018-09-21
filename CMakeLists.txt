cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)

project(libswoc)

include_directories(include)

set(core_headers
    include/swoc/swoc_common.h
    include/swoc/swoc_meta.h
    include/swoc/Scalar.h
    include/swoc/TextView.h
    include/swoc/IntrusiveDList.h
    include/swoc/IntrusiveHashMap.h
    include/swoc/MemSpan.h
    include/swoc/MemArena.h
    include/swoc/BufferWriter.h
    include/swoc/bwf_base.h
    include/swoc/bwf_std.h
    include/swoc/bwf_ex.h
    include/swoc/bwf_swoc.h
    include/swoc/Errata.h
    include/swoc/swoc_file.h
    include/swoc/Lexicon.h
    include/swoc/IntrusivePtr.h
)

# These are external but required.
set(ext_headers
    include/swoc/ext/HashFNV.h
)

set(lib_src
    src/TextView.cc
    src/MemArena.cc
    src/BufferWriterFormat.cc
    src/Errata.cc
    src/swoc_file.cc
)

add_library(swoc++ STATIC "${core_headers}" "${ext_headers}" "${lib_src}")

add_executable(test_libswoc
    include/swoc/ext/catch.hpp
    src/unit_tests/unit_test_main.cc
    src/unit_tests/test_TextView.cc
    src/unit_tests/test_MemSpan.cc
    src/unit_tests/test_MemArena.cc
    src/unit_tests/test_BufferWriter.cc
    src/unit_tests/test_BufferWriterFormat.cc
    src/unit_tests/test_IntrusiveDList.cc
    src/unit_tests/test_IntrusiveHashMap.cc
    src/unit_tests/test_Scalar.cc
    src/unit_tests/test_swoc_file.cc
    src/unit_tests/test_Errata.cc
    src/unit_tests/test_Lexicon.cc
    )

find_library(POSIX_THREAD_LIB pthread)
target_link_libraries(test_libswoc swoc++ ${POSIX_THREAD_LIB})

install(TARGETS swoc++ ARCHIVE DESTINATION lib)
install(FILES ${core_headers} DESTINATION include/swoc)
install(FILES ${ext_headers} DESTINATION include/swoc/ext)

add_custom_target(clang-format-src COMMAND ${CMAKE_HOME_DIRECTORY}/tools/clang-format.sh ${CMAKE_HOME_DIRECTORY}/src)
add_custom_target(clang-format-include COMMAND ${CMAKE_HOME_DIRECTORY}/tools/clang-format.sh ${CMAKE_HOME_DIRECTORY}/include/swoc)
add_custom_target(clang-format-ext COMMAND ${CMAKE_HOME_DIRECTORY}/tools/clang-format.sh ${CMAKE_HOME_DIRECTORY}/include/swoc/ext)
add_custom_target(clang-format)
add_dependencies(clang-format clang-format-src clang-format-include clang-format-ext)

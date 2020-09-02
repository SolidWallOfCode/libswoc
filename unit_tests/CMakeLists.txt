cmake_minimum_required(VERSION 3.12)
project(test_libswoc CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(test_libswoc
    unit_test_main.cc

    ex_suffix_expander.cc
    )

target_link_libraries(test_libswoc PUBLIC libswoc)
set_target_properties(test_libswoc PROPERTIES CLANG_FORMAT_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(test_libswoc PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-format-truncation -Wno-stringop-overflow -Wno-invalid-offsetof)
endif()
#add_definitions(-DVERBOSE_EXAMPLE_OUTPUT=1)

cmake_minimum_required(VERSION 3.14)

project("libSWOC")
include(CMakeDependentOption)
set(CMAKE_INSTALL_PREFIX ${CMAKE_HOME_DIRECTORY}/image)

cmake_dependent_option(LIBSWOC_TEST
    "Enable generation of libswoc test targets" ON
    "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

# Fortunately this has no external dependencies so the set up can be simple.
add_subdirectory(code)

if (LIBSWOC_TEST)
    add_subdirectory(unit_tests)
    add_subdirectory(example)
endif()

add_subdirectory(doc EXCLUDE_FROM_ALL)


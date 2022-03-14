cmake_minimum_required(VERSION 3.11)

project("Solid Wall Of C++ Library")
include(CMakeDependentOption)
set(INSTALL_DIR ${CMAKE_HOME_DIRECTORY})

# Fortunately this has no external dependencies so the set up can be simple.
add_subdirectory(code)
add_subdirectory(unit_tests)
add_subdirectory(example)
add_subdirectory(doc EXCLUDE_FROM_ALL)


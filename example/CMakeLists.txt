cmake_minimum_required(VERSION 3.12)
project(libswoc_examples CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(ex_netdb ex_netdb.cc)
target_link_libraries(ex_netdb PUBLIC libswoc)
set_target_properties(ex_netdb PROPERTIES CLANG_FORMAT_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(ex_netdb PRIVATE -Wall -Wextra -Werror)

add_executable(ex_netcompact ex_netcompact.cc)
target_link_libraries(ex_netcompact PUBLIC libswoc)
target_compile_options(ex_netcompact PRIVATE -Wall -Wextra -Werror)

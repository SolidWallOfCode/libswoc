cmake_minimum_required(VERSION 3.11)

project(Lib-SWOC CXX)
set(LIBSWOC_VERSION "1.3.6")
set(CMAKE_CXX_STANDARD 17)
include(GNUInstallDirs)

set(HEADER_FILES
    include/swoc/swoc_version.h
    include/swoc/ArenaWriter.h
    include/swoc/BufferWriter.h
    include/swoc/bwf_base.h
    include/swoc/bwf_ex.h
    include/swoc/bwf_ip.h
    include/swoc/bwf_std.h
    include/swoc/DiscreteRange.h
    include/swoc/Errata.h
    include/swoc/IntrusiveDList.h
    include/swoc/IntrusiveHashMap.h
    include/swoc/swoc_ip.h
    include/swoc/Lexicon.h
    include/swoc/MemArena.h
    include/swoc/MemSpan.h
    include/swoc/Scalar.h
    include/swoc/TextView.h
    include/swoc/swoc_file.h
    include/swoc/swoc_meta.h
    include/swoc/string_view.h
    )

# These are external but required.
set(EXTERNAL_HEADER_FILES
    include/swoc/ext/HashFNV.h
)

set(CC_FILES
    src/bw_format.cc
    src/bw_ip_format.cc
    src/ArenaWriter.cc
    src/Errata.cc
    src/swoc_ip.cc
    src/MemArena.cc
    src/RBTree.cc
    src/swoc_file.cc
    src/TextView.cc
    src/string_view_util.cc
    )

add_library(libswoc STATIC ${CC_FILES})
set_target_properties(libswoc PROPERTIES OUTPUT_NAME swoc-static)
if (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(libswoc PRIVATE -fPIC -Wall -Wextra -Werror -Wnon-virtual-dtor -Wpedantic)
endif()

# Not quite sure how this works, but I think it generates one of two paths depending on the context.
# That is, the generator functions return non-empty strings only in the corresponding context.
target_include_directories(libswoc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

# These install target variables are created by GNUInstallDirs.
install(TARGETS libswoc
    EXPORT libswoc-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # seems to have no effect.
    )
install(DIRECTORY include/swoc DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT libswoc-config
    NAMESPACE libswoc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libswoc
    )


set(PKG_CONFIG_FILE ${CMAKE_INSTALL_LIBDIR}/libswoc.pc)
configure_file("libswoc.pc.cmake" ${PKG_CONFIG_FILE} @ONLY)
install(FILES ${PKG_CONFIG_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# Alledgedly this makes the targets "importable from the build directory" but I see no evidence of that.
# AFAICT the file isn't created at all even with this enabled.
#export(TARGETS libswoc FILE libswoc-config.cmake)

set(CLANG_DIRS )

set_target_properties(libswoc PROPERTIES CLANG_FORMAT_DIRS "${PROJECT_SOURCE_DIR}/src;${PROJECT_SOURCE_DIR}/include")

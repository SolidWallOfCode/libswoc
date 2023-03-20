prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@
libdir=${exec_prefix}/@CMAKE_INSTALL_LIBDIR@

Name: LibSWOC++
Description: A collection of solid C++ utilities and classes.
Version: @LIBSWOC_VERSION@
Requires:
Libs: -L${libdir} -lswoc-static-@LIBSWOC_VERSION@
Cflags: -I${includedir}

#!/bin/bash

if [ -z "$3" ] ; then
  echo "Usage: $0 major minor point"
  exit 1
fi

# Header
sed -i swoc++/include/swoc/swoc_version.h --expr "s/\(MAJOR_VERSION *= *\).*\$/\\1$1;/"
sed -i swoc++/include/swoc/swoc_version.h --expr "s/\(MINOR_VERSION *= *\).*\$/\\1$2;/"
sed -i swoc++/include/swoc/swoc_version.h --expr "s/\(POINT_VERSION *= *\).*\$/\\1$3;/"

sed -i doc/conf.py --expr "s/release = .*\$/release = \"$1.$2.$3\"/"
sed -i doc/Doxyfile --expr "s/\(PROJECT_NUMBER *= *\).*\$/\\1\"$1.$2.$3\"/"

sed -i swoc++/CMakeLists.txt --expr "s/\(LIBSWOC_VERSION *\)\"[^\"]*\"/\\1\"$1.$2.$3\"/"
sed -i swoc++/swoc++.part --expr "s/PartVersion(\"[0-9.]*\")/PartVersion(\"$1.$2.$3\")/"

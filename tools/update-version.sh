#!/bin/bash

if [ -z "$3" ] ; then
  echo "Usage: $0 major minor point"
  exit 1
fi

# Header
sed -i -E code/include/swoc/swoc_version.h --expr "s/SWOC_VERSION_NS _[0-9]+_[0-9]+_[0-9]+/SWOC_VERSION_NS _$1_$2_$3/"wqq
sed -i code/include/swoc/swoc_version.h --expr "s/\(MAJOR_VERSION *= *\).*\$/\\1$1;/"
sed -i code/include/swoc/swoc_version.h --expr "s/\(MINOR_VERSION *= *\).*\$/\\1$2;/"
sed -i code/include/swoc/swoc_version.h --expr "s/\(POINT_VERSION *= *\).*\$/\\1$3;/"

sed -i doc/conf.py --expr "s/release = .*\$/release = \"$1.$2.$3\"/"
sed -i doc/Doxyfile --expr "s/\(PROJECT_NUMBER *= *\).*\$/\\1\"$1.$2.$3\"/"
find doc -name "*.en.rst" -exec sed -i {} --expr "s!/libswoc/blob/[0-9.]*/unit_tests/!/libswoc/blob/$1.$2.$3/unit_tests/!" \;

sed -i code/CMakeLists.txt --expr "s/\(LIBSWOC_VERSION *\)\"[^\"]*\"/\\1\"$1.$2.$3\"/"
sed -i code/libswoc.part --expr "s/PartVersion(\"[0-9.]*\")/PartVersion(\"$1.$2.$3\")/"


#!/usr/bin/env bash

ATS=${1:-../ts}

if [ ! -d "${ATS}" ] ; then
  echo "Target ${ATS} is not a directory"
  exit 1
fi

if [ ! -r Sconstruct ] ; then
  echo "This must be run from the top level libswoc directory."
  exit 2
fi

BASE_PATH="lib"
TARGET_BASE_DIR="${ATS}/${BASE_PATH}"

SRC_PATH="${BASE_PATH}/swoc/src"
TARGET_SRC_DIR="${ATS}/${SRC_PATH}"
if [ -d "${TARGET_SRC_DIR}" ] ; then
  rm -f ${TARGET_SRC_DIR}/*.cc
else
  mkdir -p ${TARGET_SRC_DIR}
fi

cp code/src/*.cc ${TARGET_SRC_DIR}
(cd ${ATS}; git add ${SRC_PATH}/*.cc)

INC_PATH="${BASE_PATH}/swoc/include/swoc"
TARGET_INC_DIR="${ATS}/${INC_PATH}"

if [ -d "${TARGET_INC_DIR}" ] ; then
  rm -f ${TARGET_INC_DIR}/*.h
else
  mkdir -p ${TARGET_INC_DIR}
fi

if [ -d "${TARGET_INC_DIR}/ext" ] ; then
  rm -f ${TARGET_INC_DIR}/ext/*.h
else
  mkdir -p ${TARGET_INC_DIR}/ext
fi

cp code/include/swoc/*.h ${TARGET_INC_DIR}
cp code/include/swoc/ext/*.h ${TARGET_INC_DIR}/ext
cp code/include/swoc/ext/HashFNV.h ${TARGET_INC_DIR}
(cd ${ATS}; git add ${INC_PATH}/*.h ; git add ${INC_PATH}/ext/*.h)

# Build the source
cat <<'TEXT' > ${ATS}/${BASE_PATH}/swoc/Makefile.am
# swoc Makefile.am
#
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

lib_LTLIBRARIES = libtsswoc.la

library_includedir=$(includedir)/swoc

AM_CPPFLAGS += @SWOC_INCLUDES@

libtsswoc_la_LDFLAGS = @AM_LDFLAGS@ -no-undefined -release 1.4.2
libtsswoc_la_SOURCES = \
	src/ArenaWriter.cc  src/bw_format.cc  src/bw_ip_format.cc  src/Errata.cc  src/MemArena.cc  src/RBTree.cc  src/swoc_file.cc  src/swoc_ip.cc  src/TextView.cc src/string_view_util.cc

if EXPORT_SWOC_HEADERS
library_include_HEADERS = \
        include/swoc/ArenaWriter.h \
        include/swoc/BufferWriter.h \
        include/swoc/bwf_base.h \
        include/swoc/bwf_ex.h \
        include/swoc/bwf_fwd.h \
        include/swoc/bwf_ip.h \
        include/swoc/bwf_std.h \
        include/swoc/DiscreteRange.h \
        include/swoc/Errata.h \
        include/swoc/IntrusiveDList.h \
        include/swoc/IntrusiveHashMap.h \
        include/swoc/IPAddr.h \
        include/swoc/IPEndpoint.h \
        include/swoc/IPRange.h \
        include/swoc/IPSrv.h \
        include/swoc/Lexicon.h \
        include/swoc/MemArena.h \
        include/swoc/MemSpan.h \
        include/swoc/RBTree.h \
        include/swoc/Scalar.h \
        include/swoc/swoc_file.h \
        include/swoc/swoc_ip.h \
        include/swoc/swoc_meta.h \
        include/swoc/swoc_version.h\
        include/swoc/string_view_util.h \
        include/swoc/TextView.h \
        include/swoc/Vectray.h \
        include/swoc/HashFNV.h
endif

clean-local:

clang-tidy-local: $(DIST_SOURCES)
	$(CXX_Clang_Tidy)
TEXT
(cd ${ATS} ; git add ${BASE_PATH}/swoc/Makefile.am)

if ! grep -q swoc ${ATS}/configure.ac ; then
  sed -i -e 's!lib/yamlcpp/Makefile!lib/swoc/Makefile\n  &!' ${ATS}/configure.ac
  (cd ${ATS} ; git add configure.ac)
fi

if ! grep -q swoc ${ATS}/${BASE_PATH}/Makefile.am ; then
  sed -i -e '/SUBDIRS =/s!$! swoc!' ${ATS}/${BASE_PATH}/Makefile.am
  (cd ${ATS} ; git add ${BASE_PATH}/Makefile.am)
fi

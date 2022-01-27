#!/usr/bin/env bash
# Need to run this in the top level libswoc directory.
ATS=${1:-../ats}

if [ ! -d "${ATS}" ] ; then
  echo "Target ${ATS} is not a directory"
  exit 1
fi

if [ ! -r Sconstruct ] ; then
  echo "This must be run from the top level libswoc directory."
  exit 2
fi

SRC_DIR="${ATS}/src/swoc"
if [ -d "${SRC_DIR}" ] ; then
  rm -f ${SRC_DIR}/*.cc
else
  mkdir ${SRC_DIR}
fi

cp code/src/*.cc ${SRC_DIR}
rm ${SRC_DIR}/string_view_util.cc
(cd ${ATS}; git add ${SRC_DIR}/*.cc)

INC_DIR="${ATS}/include/swoc"

if [ -d "${INC_DIR}" ] ; then
  rm -f ${INC_DIR}/*.h
else
  mkdir ${INC_DIR}
fi

cp code/include/swoc/*.h ${INC_DIR}
rm ${INC_DIR}/string_view_util.h
sed -i -e "s!swoc/ext/HashFNV.h!tscpp/util/HashFNV.h!g" ${INC_DIR}/*.h
sed -i -e "s!swoc/string_view_util.h!tscpp/util/string_view_util.h!g" ${INC_DIR}/*.h
(cd ${ATS}; git add ${INC_DIR}/*.h)
# Need to move HashFNV.h around and cleanup.
if [ -f ${ATS}/include/tscore/HashFNV.h ] ; then
  (cd ${ATS}; git mv include/tscore/HashFNV.h include/tscpp/util)

  for f in plugins/experimental/ssl_session_reuse/src/publisher.h \
          iocore/hostdb/I_HostDBProcessor.h \
          include/tscore/EnumDescriptor.h \
          src/tscore/HashFNV.cc \
          src/traffic_logstats/logstats.cc \
          proxy/hdrs/HdrToken.cc \
          ; do
    sed -i -e "s!tscore/HashFNV.h!tscpp/util/HashFNV.h!g" ${ATS}/${f}
  done
fi

# Arrange to install the libswoc header files
cat <<'TEXT' > ${INC_DIR}/Makefile.am
# include/swoc Makefile.am
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

library_includedir=$(includedir)/swoc

library_include_HEADERS = \
        ArenaWriter.h \
        BufferWriter.h \
        bwf_base.h bwf_ex.h bwf_fwd.h bwf_ip.h bwf_std.h \
        DiscreteRange.h \
        Errata.h \
        IntrusiveDList.h \
        IntrusiveHashMap.h \
        Lexicon.h \
        MemArena.h \
        MemSpan.h \
        RBTree.h \
        Scalar.h \
        swoc_file.h \
        swoc_ip.h \
        swoc_meta.h \
        swoc_version.h\
        TextView.h
TEXT
sed -i -e 's!^\(SUBDIRS.*util\)$!\1 swoc!' ${INC_DIR}/../Makefile.am
(cd ${ATS} ; git add include/swoc/Makefile.am)
sed -i -e 's!include/tscpp/util/Makefile!&\n  include/swoc/Makefile!' ${ATS}/configure.ac

# Build the source
cat <<'TEXT' > ${SRC_DIR}/Makefile.am
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

include $(top_srcdir)/build/tidy.mk

lib_LTLIBRARIES = libtsswoc.la

AM_CPPFLAGS += -I$(abs_top_srcdir)/include

libtsswoc_la_LDFLAGS = @AM_LDFLAGS@ -no-undefined -version-info @TS_LIBTOOL_VERSION@

libtsswoc_la_SOURCES = \
	ArenaWriter.cc  bw_format.cc  bw_ip_format.cc  Errata.cc  MemArena.cc  RBTree.cc  swoc_file.cc  swoc_ip.cc  TextView.cc

clean-local:

clang-tidy-local: $(DIST_SOURCES)
	$(CXX_Clang_Tidy)
TEXT

sed -i -e 's!src/wccp/Makefile!src/swoc/Makefile\n  &!' ${ATS}/configure.ac
sed -i -e 's! src/tscpp/util !&src/swoc !' ${ATS}/Makefile.am
(cd ${ATS} ; git add src/swoc/Makefile.am)

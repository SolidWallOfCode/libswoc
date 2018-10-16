#!/usr/bin/env bash
# Need to run this in the libswoc directory.
ATS=../work

function rewrite {
  sed -i -E --expr 's!swoc/swoc_meta!tscpp/util/ts_meta!g' $1
  sed -i -E --expr 's!swoc/Scalar!tscpp/util/Scalar!g' $1
  sed -i -E --expr 's!swoc/ext/catch.hpp!catch.hpp!g' $1
  sed -i -E --expr 's!swoc::!ts::!g' $1
  sed -i -E --expr 's!swoc/!tscpp/util/!g' $1
  sed -i -E --expr 's!namespace swoc!namespace ts!g' $1
  sed -i -E --expr 's!ts/swoc_meta!tscpp/util/ts_meta!g' $1
  sed -i -E --expr 's!swoc_meta!ts_meta!g' $1
}

# IntrusiveDList
if [ -f ${ATS}/include/tscore/IntrusiveDList.h ] ; then
  (cd ${ATS}; git mv include/tscore/IntrusiveDList.h include/tscpp/util)
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_IntrusiveDList.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_IntrusiveDList.cc src/tscpp/util/unit_tests)
fi

if cp src/unit_tests/test_IntrusiveDList.cc ${ATS}/src/tscpp/util/unit_tests ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_IntrusiveDList.cc
  sed -i -E --expr '/test_IntrusiveDList[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_IntrusiveDList[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_PostScript.cc!i\
\tunit_tests/test_IntrusiveDList.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
fi

if cp include/swoc/IntrusiveDList.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/IntrusiveDList.h
  find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!tscore/IntrusiveDList[.]h!tscpp/util/IntrusiveDList.h!' {} \;
  find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!IntrusiveDList<([^>]*)>!ts::IntrusiveDList<\1>!' {} \;
  find ${ATS} -name '*.rst' -exec sed -i -E -e 's!src/tscore/unit_tests/test_IntrusiveDList.cc!src/tscpp/util/unit_tests/test_IntrusiveDList.cc!' {} \;
else
  echo "Failed to copy IntrusiveDList.h"
  exit 1;
fi

exit 0

# swoc_meta.h
if [ -f ${ATS}/include/tscore/ts_meta.h ] ; then
  (cd ${ATS}; git mv include/tscore/ts_meta.h include/tscpp/util)
fi

if cp include/swoc/swoc_meta.h ${ATS}/include/tscpp/util/ts_meta.h ; then
  rewrite ${ATS}/include/tscpp/util/ts_meta.h
  sed -i -E --expr 's!tscore/ts_meta.h!tscpp/util/ts_meta.h!g' ${ATS}/src/tscpp/util/unit_tests/test_ts_meta.cc
else
  echo "Failed to copy ts_meta.h"
  exit 1;
fi

# Scalar
if [ -f ${ATS}/include/tscore/Scalar.h ] ; then
  (cd ${ATS}; git mv include/tscore/Scalar.h include/tscpp/util)
fi

if cp include/swoc/Scalar.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/Scalar.h
  sed -i -E --expr 's!tscore/Scalar!tscpp/util/Scalar!g' ${ATS}/include/tscore/MemArena.h
  sed -i -E --expr 's!tscore/Scalar!tscpp/util/Scalar!g' ${ATS}/src/traffic_cache_tool/CacheDefs.h
  if ! grep -q ink_assert ${ATS}/include/tscore/MemArena.h ; then
      sed -i -E --expr '\!tscpp/util/Scalar!a\
    #include "tscore/ink_assert.h"' ${ATS}/include/tscore/MemArena.h
  fi
  if ! grep -q cassert ${ATS}/src/traffic_cache_tool/CacheDefs.cc ; then
      sed -i -E --expr '\!"CacheDefs[.]h"!a\
    #include <cassert>' ${ATS}/src/traffic_cache_tool/CacheDefs.cc
  fi
else
  echo "Failed to copy Scalar.h"
  exit 1;
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_Scalar.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_Scalar.cc src/tscpp/util/unit_tests)
fi

if cp src/unit_tests/test_Scalar.cc ${ATS}/src/tscpp/util/unit_tests ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_Scalar.cc
  sed -i -E --expr '/test_Scalar[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_Scalar[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_TextView.cc!i\
\tunit_tests/test_Scalar.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
fi

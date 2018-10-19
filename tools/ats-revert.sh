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

# Scalar
if [ -f ${ATS}/include/tscore/Scalar.h ] ; then
  (cd ${ATS}; git mv include/tscore/Scalar.h include/tscpp/util)
fi

if cp include/swoc/Scalar.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/Scalar.h
  sed -i -E --expr 's!tscore/Scalar!tscpp/util/Scalar!g' ${ATS}/include/tscpp/util/MemArena.h
  sed -i -E --expr 's!tscore/Scalar!tscpp/util/Scalar!g' ${ATS}/src/traffic_cache_tool/CacheDefs.h
  if ! grep -q ink_assert[.]h ${ATS}/include/tscore/MemArena.h ; then
      sed -i -E --expr '\!tscpp/util/Scalar!a\
#include "tscore/ink_assert.h"' ${ATS}/include/tscore/MemArena.h
  fi
  if ! grep -q '[<]cassert[>]' ${ATS}/src/traffic_cache_tool/CacheDefs.cc ; then
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

# BWF
if [ -f ${ATS}/include/tscore/BufferWriter.h ] ; then
  (cd ${ATS}; git mv include/tscore/BufferWriter.h include/tscpp/util)
fi

if [ -f ${ATS}/include/tscore/BufferWriterForward.h ] ; then
  (cd ${ATS}; git rm include/tscore/BufferWriterForward.h)
fi

if [ -f ${ATS}/src/tscore/BufferWriterFormat.cc ] ; then
  (cd ${ATS}; git mv src/tscore/BufferWriterFormat.cc src/tscpp/util/bw_format.cc)
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_BufferWriter.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_BufferWriter.cc src/tscpp/util/unit_tests)
  sed -i -E --expr '/test_BufferWriter[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_bw_format[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_MemSpan.cc!i\
\tunit_tests/test_bw_format.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_BufferWriterFormat.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_BufferWriterFormat.cc src/tscpp/util/unit_tests/bw_format.cc)
  sed -i -E --expr '/test_bwformat[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_bw_format[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_MemSpan.cc!i\
\tunit_tests/test_bw_format.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
fi

if cp src/bw_format.cc ${ATS}/src/tscpp/util ; then
  rewrite ${ATS}/src/tscpp/util/bw_format.cc
  sed -i -E --expr '/BufferWriterFormat[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_bw_format[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_MemSpan.cc!i\
\tunit_tests/test_bw_format.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
fi

if cp include/swoc/bwf_std.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/bwf_std.h
  (cd ${ATS}; git add include/tscpp/util/bwf_std.h)
  if ! grep -q bwf_std[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!include_HEADERS!a\
\tbwf_std.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

if cp include/swoc/bwf_base.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/bwf_base.h
  (cd ${ATS}; git add include/tscpp/util/bwf_base.h)
  if ! grep -q bwf_base[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!include_HEADERS!a\
\tbwf_base.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

if cp include/swoc/bwf_ex.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/bwf_ex.h
  (cd ${ATS}; git add include/tscpp/util/bwf_ex.h)
  if ! grep -q bwf_ex[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!include_HEADERS!a\
\tbwf_ex.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

if cp include/swoc/bwf_printf.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/bwf_printf.h
  (cd ${ATS}; git add include/tscpp/util/bwf_printf.h)
  if ! grep -q bwf_printf[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!include_HEADERS!a\
\tbwf_printf.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

if cp include/swoc/BufferWriter.h ${ATS}/include/tscpp/util ; then
  rewrite ${ATS}/include/tscpp/util/BufferWriter.h
  (cd ${ATS}; git add include/tscpp/util/BufferWriter.h)
  if ! grep -q BufferWriter[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!include_HEADERS!a\
\tBufferWriter.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!tscore/BufferWriterForward[.]h!tscpp/util/BufferWriter.h!' {} \;
find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!tscore/BufferWriter[.]h!tscpp/util/BufferWriter.h!' {} \;
find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!BWFSpec!bwf::Spec!' {} \;

if ! grep -q bwf_base[.]h ${ATS}/src/tscore/CryptoHash.cc ; then
    sed -i -E --expr '\!tscore/SHA256!a\
#include "tscpp/util/bwf_base.h"' ${ATS}/src/tscore/CryptoHash.cc
fi
sed -i -E --expr '/namespace ts/,+11c\
namespace ts {\
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, ats::CryptoHash const &hash);\
} // namespace ts\
' ${ATS}/include/tscore/CryptoHash.h

cat > /tmp/cmd.sh <<'CODE'
$a\
\
namespace ts\
{\
inline BufferWriter &\
bwformat(BufferWriter &w, bwf::Spec const &spec, ats::CryptoHash const &hash)\
{\
  bwf::Spec local_spec{spec};\
  if ('X' != local_spec._type)\
    local_spec._type = 'x';\
  return bwformat(w, local_spec, std::string_view(reinterpret_cast<const char *>(hash.u8), CRYPTO_HASH_SIZE));\
}\
} // namespace ts
CODE

sed -i -E --file /tmp/cmd.sh ${ATS}/src/tscore/CryptoHash.cc

function bwf_upgrade {
  sed -i -E --expr 's!clip[(]!restrict(!' $1
  sed -i -E --expr 's!extend[(]!restore(!' $1
  sed -i -E --expr 's!auxBuffer[(]!aux_data(!' $1
  sed -i -E --expr 's!fill[(]!commit(!' $1
}

bwf_upgrade ${ATS}/src/tscore/Diags.cc
bwf_upgrade ${ATS}/iocore/eventsystem/I_MIOBufferWriter.h

sed -i -E --expr '96s!commit!fill!' ${ATS}/iocore/eventsystem/I_MIOBufferWriter.h
sed -i -E --expr '140d' ${ATS}/iocore/eventsystem/I_MIOBufferWriter.h
sed -i -E --expr '428,449d' ${ATS}/iocore/eventsystem/IOBuffer.cc

sed -i -E --expr '\!tscpp/util/TextView[.]h!a\
#include "tscpp/util/bwf_base.h"' ${ATS}/src/tscore/ink_inet.cc

exit 0

# swoc_meta.h
if [ -f ${ATS}/include/tscore/ts_meta.h ] ; then
  (cd ${ATS}; git mv include/tscore/ts_meta.h include/tscpp/util)
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_ts_meta.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_ts_meta.cc src/tscpp/util/unit_tests)
fi

if cp include/swoc/swoc_meta.h ${ATS}/include/tscpp/util/ts_meta.h ; then
  rewrite ${ATS}/include/tscpp/util/ts_meta.h
  sed -i -E --expr 's!tscore/ts_meta.h!tscpp/util/ts_meta.h!g' ${ATS}/src/tscpp/util/unit_tests/test_ts_meta.cc
else
  echo "Failed to copy ts_meta.h"
  exit 1;
fi

if cp src/unit_tests/test_meta.cc ${ATS}/src/tscpp/util/unit_tests/test_ts_meta.cc ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_ts_meta.cc
#  sed -i -E --expr 's!tscore/ts_meta.h!tscpp/util/ts_meta.h!g' ${ATS}/src/tscpp/util/unit_tests/test_ts_meta.cc
else
  echo "Failed to copy test_meta.cc"
  exit 1;
fi

# MemSpan.h
if [ -f ${ATS}/include/tscore/MemSpan.h ] ; then
  (cd ${ATS}; git mv include/tscore/MemSpan.h include/tscpp/util)
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_MemSpan.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_MemSpan.cc src/tscpp/util/unit_tests)
fi

if cp include/swoc/MemSpan.h ${ATS}/include/tscpp/util/MemSpan.h ; then
  rewrite ${ATS}/include/tscpp/util/MemSpan.h
  sed -i -E --expr '/test_MemSpan[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_MemSpan[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_PostScript.cc!i\
\tunit_tests/test_MemSpan.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
else
  echo "Failed to copy MemSpan.h"
  exit 1;
fi

if cp src/unit_tests/test_MemSpan.cc ${ATS}/src/tscpp/util/unit_tests/test_MemSpan.cc ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_MemSpan.cc
else
  echo "Failed to copy test_MemSpan.cc"
  exit 1;
fi

find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!tscore/MemSpan[.]h!tscpp/util/MemSpan.h!' {} \;

# TextView
if cp include/swoc/TextView.h ${ATS}/include/tscpp/util/TextView.h ; then
  rewrite ${ATS}/include/tscpp/util/TextView.h
else
  echo "Failed to copy TextView.h"
  exit 1;
fi

if cp src/unit_tests/test_TextView.cc ${ATS}/src/tscpp/util/unit_tests/ ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_TextView.cc
else
  echo "Failed to copy test_TextView.cc"
  exit 1;
fi

### IntrusiveDList
if [ -f ${ATS}/include/tscore/IntrusiveDList.h ] ; then
  (cd ${ATS}; git mv include/tscore/IntrusiveDList.h include/tscpp/util)
  sed -i -E --expr '/IntrusiveDList[.h]/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q IntrusiveDList[.]h ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!la_SOURCES!a\
\tIntrusiveDList.h \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
  if ! grep -q IntrusiveDList[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!include_HEADERS!a\
\tIntrusiveDList.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_IntrusiveDList.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_IntrusiveDList.cc src/tscpp/util/unit_tests)
  sed -i -E --expr '/test_IntrusiveDList[.]cc/d' ${ATS}/src/tscore/Makefile.am
#  if ! grep -q test_IntrusiveDList[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
#      sed -i -E --expr '\!unit_test_main!a\
##\tunit_tests/test_IntrusiveDList.cc \\ requires BufferWriter move to libtscpputil' ${ATS}/src/tscpp/util/Makefile.am
#  fi
fi

if cp src/unit_tests/test_IntrusiveDList.cc ${ATS}/src/tscpp/util/unit_tests ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_IntrusiveDList.cc
  sed -i -E --expr 's!tscpp/util/bwf_base.h!tscore/BufferWriter.h!' ${ATS}/src/tscore/Makefile.am
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

### MemSpan.h extra cleanup.
sed -i -E --expr '/MemSpan[.h]/d' ${ATS}/src/tscore/Makefile.am
if ! grep -q MemSpan[.]h ${ATS}/src/tscpp/util/Makefile.am ; then
  sed -i -E --expr '\!IntrusiveDList[.]h!a\
\tMemSpan.h \\' ${ATS}/src/tscpp/util/Makefile.am
fi

if ! grep -q MemSpan[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
  sed -i -E --expr '\!IntrusiveDList[.]h!a\
\tMemSpan.h \\' ${ATS}/include/tscpp/util/Makefile.am
fi

### MemArena.h
if [ -f ${ATS}/include/tscore/MemArena.h ] ; then
  (cd ${ATS}; git mv include/tscore/MemArena.h include/tscpp/util)
  sed -i -E --expr '/MemArena[.h]/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q MemArena[.]h ${ATS}/src/tscpp/util/Makefile.am ; then
    sed -i -E --expr '\!IntrusiveDList[.]h!a\
\tMemArena.h \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
  if ! grep -q MemArena[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
    sed -i -E --expr '\!MemSpan!i\
\tMemArena.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
fi

if [ -f ${ATS}/src/tscore/MemArena.cc ] ; then
  (cd ${ATS}; git mv src/tscore/MemArena.cc src/tscpp/util)
  if ! grep -q \\sMemArena[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!MemSpan[.]h!i\
\tMemArena.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
fi

if [ -f ${ATS}/src/tscore/unit_tests/test_MemArena.cc ] ; then
  (cd ${ATS}; git mv src/tscore/unit_tests/test_MemArena.cc src/tscpp/util/unit_tests)
fi

if cp include/swoc/MemArena.h ${ATS}/include/tscpp/util/MemArena.h ; then
  rewrite ${ATS}/include/tscpp/util/MemArena.h
  sed -i -E --expr 's!tscpp/util/Scalar[.]h!tscore/Scalar.h!g' ${ATS}/include/tscpp/util/MemArena.h
  sed -i -E --expr '/test_MemArena[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q test_MemArena[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!test_MemSpan.cc!i\
\tunit_tests/test_MemArena.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
  if ! grep -q MemArena[.]h ${ATS}/include/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!PostScript[.]h!i\
\tMemArena.h \\\
\tMemSpan.h \\' ${ATS}/include/tscpp/util/Makefile.am
  fi
else
  echo "Failed to copy MemArena.h"
  exit 1;
fi

if cp src/MemArena.cc ${ATS}/src/tscpp/util ; then
  rewrite ${ATS}/src/tscpp/util/MemArena.cc
else
  echo "Failed to copy test_MemArena.cc"
  exit 1;
fi

if cp src/unit_tests/test_MemArena.cc ${ATS}/src/tscpp/util/unit_tests/test_MemArena.cc ; then
  rewrite ${ATS}/src/tscpp/util/unit_tests/test_MemArena.cc
  sed -i -E --expr 's!tscpp/util/Scalar[.]h!tscore/Scalar.h!g' ${ATS}src/tscpp/util/MemArena.cc
  sed -i -E --expr 'MemArena[.]cc/d' ${ATS}/src/tscore/Makefile.am
  if ! grep -q \\sMemArena[.]cc ${ATS}/src/tscpp/util/Makefile.am ; then
      sed -i -E --expr '\!PostScript[.]h!i\
\tMemArena.cc \\' ${ATS}/src/tscpp/util/Makefile.am
  fi
else
  echo "Failed to copy test_MemArena.cc"
  exit 1;
fi

find ${ATS} \( -name '*.cc' -o -name '*.h' \) -exec sed -i -E -e 's!tscore/MemArena[.]h!tscpp/util/MemArena.h!' {} \;


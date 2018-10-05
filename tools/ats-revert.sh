#!/usr/bin/env bash
# Need to run this in the libswoc directory.

function rewrite {
  sed -i -E --expr 's!swoc::!ts::!g' $1
  sed -i -E --expr 's!swoc/!ts/!g' $1
  sed -i -E --expr 's!namespace swoc!namespace ts!g' $1
  sed -i -E --expr 's!swoc_meta!ts_meta!g' $1
}

# ts_meta
if cp include/swoc/swoc_meta.h ../ats/include/tscore/ts_meta.h ; then
  rewrite ../ats/include/tscore/ts_meta.h
else
  echo "Failed to copy swoc_meta.h"
  exit 1;
fi

# test_ts_meta
if cp src/unit_tests/test_swoc_meta.cc ../ats/src/tscore/unit_tests/test_ts_meta.cc ; then
  rewrite ../ats/src/tscore/unit_tests/test_ts_meta.cc
else
  echo "Failed to copy test_swoc_meta.h"
  exit 1;
fi

exit 0

# Scalar
if cp include/swoc/Scalar.h ../ats/include/tscore ; then
  rewrite ../ats/include/tscore/Scalar.h
else
  echo "Failed to copy Scalar.h"
  exit 1;
fi

# Do IntrusiveDList
if cp include/swoc/IntrusiveDList.h ../ats/include/tscore ; then
  rewrite ../ats/include/tscore/IntrusiveDList.h
else
  echo "Failed to copy IntrusiveDList.h"
  exit 1;
fi

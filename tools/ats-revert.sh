#!/usr/bin/env bash
# Need to run this in the libswoc directory.
ATS=../work

function rewrite {
  sed -i -E --expr 's!swoc::!ts::!g' $1
  sed -i -E --expr 's!swoc/!ts/!g' $1
  sed -i -E --expr 's!namespace swoc!namespace ts!g' $1
  sed -i -E --expr 's!ts/swoc_meta!tscore/ts_meta!g' $1
  sed -i -E --expr 's!swoc_meta!ts_meta!g' $1
}

# swoc_meta.h
if [ -f ${ATS}/include/tscore/ts_meta.h ] ; then
  git mv ${ATS}/include/tscore/ts_meta.h ${ATS}/include/tscpp/util
fi

if cp include/swoc/swoc_meta.h ${ATS}/include/tscpp/util/ts_meta.h ; then
  rewrite ${ATS}/include/tscore/ts_meta.h
else
  echo "Failed to copy ts_meta.h"
  exit 1;
fi

# Scalar
if cp include/swoc/Scalar.h ${ATS}/include/tscore ; then
  rewrite ${ATS}/include/tscore/Scalar.h
else
  echo "Failed to copy Scalar.h"
  exit 1;
fi

exit 0

# Do IntrusiveDList
if cp include/swoc/IntrusiveDList.h ${ATS}/include/tscore ; then
  rewrite ${ATS}/include/tscore/IntrusiveDList.h
else
  echo "Failed to copy IntrusiveDList.h"
  exit 1;
fi

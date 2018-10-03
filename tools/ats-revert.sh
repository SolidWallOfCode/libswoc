#!/usr/bin/env bash
# Need to run this in the libswoc directory.

function rewrite {
  sed -iE --expr 's!swoc::!ts::!g' $1
  sed -iE --expr 's!swoc/!ts/!g' $1
  sed -iE --expr 's!namespace swoc!namespace ts!g' $1
  sed -iE --expr 's!swoc_meta!ts_meta!g' $1
}

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

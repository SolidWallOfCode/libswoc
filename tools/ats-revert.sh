#!/usr/bin/env bash
# Need to run this in the libswoc directory.

# Do IntrusiveDList
if cp include/swoc/IntrusiveDList.h ../ats/include/tscore ; then
  sed -iE --expr 's!swoc::!ts::!g' ../ats/include/tscore/IntrusiveDList.h
  sed -iE --expr 's!swoc/!ts/!' ../ats/include/tscore/IntrusiveDList.h
  sed -iE --expr 's!namespace swoc/!namespace ts/!' ../ats/include/tscore/IntrusiveDList.h
else
  echo "Failed to copy IntrusiveDList.h"
  exit 1;
fi

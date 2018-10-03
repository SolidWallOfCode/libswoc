#!/usr/bin/env bash

for f in src/*.cc ; do
  sed -iE --expr 's!swoc::!ts::!g' $f
  sed -iE --expr 's!swoc/!ts/!' $f
done

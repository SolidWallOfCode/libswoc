#!/usr/bin/env bash

function rewrite {
  sed -i -E --expr 's!namespace ts!namespace SWOC_NAMESPACE!g' $1
  sed -i -E --expr 's!^}\s+// namespace ts!} //namespace SWOC_NAMESPACE!g' $1
  sed -i -E --expr 's!ts::!swoc::!g' $1
  sed -i -E --expr 's!include "tscpp/util/!include "swoc/!' $1
  sed -i -E --expr 's!"[^"]*catch.hpp"!"swoc/ext/catch.hpp"!' $1
  sed -i -E --expr 's!swoc/HashFNV.h!swoc/ext/HashFNV.h!' $1
}

rewrite $1

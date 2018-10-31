#!/usr/bin/env bash
# Need to run this in the libswoc directory.
ATS=${1:-../ats}

cp src/*.cc ${ATS}/lib/swoc++/src
(cd ${ATS}; git add lib/swoc++/src/*.cc)

cp src/unit_tests/*.cc ${ATS}/lib/swoc++/src/unit_tests
(cd ${ATS}; git add lib/swoc++/src/unit_tests/*.cc)

cp include/swoc/*.h ${ATS}/lib/swoc++/include/swoc
(cd ${ATS}; git add lib/swoc++/include/swoc/*.h)

cp include/swoc/ext/*.h ${ATS}/lib/swoc++/include/swoc/ext
(cd ${ATS}; git add lib/swoc++/include/swoc/ext/*.h)

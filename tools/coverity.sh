BASE=/tmp/libswoc
COV="/home/amc/opt/coverity/cov-analysis-linux64-2022.12.2/bin"
TARGET=/tmp/libswoc.tgz

rm -rf ${BASE}
rm -f ${TARGET}
cd /tmp
git clone git@github.com:solidwallofcode/libswoc
cd libswoc
cmake . -B build
PATH="$COV:$PATH" $COV/cov-build --dir cov-int cmake --build build --target libswoc
tar czvf ${TARGET} cov-int

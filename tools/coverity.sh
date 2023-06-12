BASE=/tmp/libswoc
COV="/home/amc/opt/coverity/cov-analysis-linux64-2022.12.2/bin"

rm -rf ${BASE}
cd /tmp
git clone git@github.com:solidwallofcode/libswoc
cd libswoc
cmake . -B build
PATH="$COV:$PATH" $COV/cov-build --dir cov-int cmake --build build --target libswoc
tar czvf /tmp/libswoc.tgz cov-int

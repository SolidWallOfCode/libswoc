#! /bin/sh
echo "Filtering $1" >&2
cat $1 | sed --expr 's/inline namespace SWOC_VERSION_NS {//' | sed --expr 's/}} \/\/ namespace swoc/} \/\/ namespace swoc/'

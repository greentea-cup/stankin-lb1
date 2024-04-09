#!/bin/sh

BUILD_TYPE="${BUILD_TYPE:-${1:-Release}}"
echo "Using BUILD_TYPE=$BUILD_TYPE"

./fmt.sh
cmake --build Build/$BUILD_TYPE
exit $?

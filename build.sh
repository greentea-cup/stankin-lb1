#!/bin/sh

BUILD_TYPE="${BUILD_TYPE:-Release}"
echo "Using BUILD_TYPE=$BUILD_TYPE"

./fmt.sh
# build type == preset name
cmake --build $BUILD_TYPE
exit $?

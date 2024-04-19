#!/bin/sh

BUILD_TYPE="${BUILD_TYPE:-Release}"
./build.sh
"./Build/$BUILD_TYPE/app" $@
exit $?

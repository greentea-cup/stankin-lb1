#!/bin/sh

export CC=clang CXX=clang
# export CC=gcc CXX=g++
# for msvc use regen_cmake.bat or VS

BUILD_TYPE="${BUILD_TYPE:-${1:-Release}}"
echo "Using BUILD_TYPE=$BUILD_TYPE"
export EXPORT_COMPILE_COMMANDS="${EXPORT_COMPILE_COMMANDS:-$CCMDS}"
if [ "$EXPORT_COMPILE_COMMANDS" == "0" ]; then
	unset EXPORT_COMPILE_COMMANDS
fi
echo "Using EXPORT_COMPILE_COMMANDS=$EXPORT_COMPILE_COMMANDS"

# clear cache
rm -rf Build compile_commands.json
# regen for both Release and Debug configurations
cmake -S . -B Build/Release -DCMAKE_BUILD_TYPE=Release
cmake -S . -B Build/Debug -DCMAKE_BUILD_TYPE=Debug
# to regen only for specified configuration:
# cmake -S . -B Build/$BUILD_TYPE -DCMAKE_BUILD_TYPE=$BUILD_TYPE

if [ ! -z "$EXPORT_COMPILE_COMMANDS" ]; then
	echo "Making symlink to Build/$BUILD_TYPE/compile_commands.json"
	ln -s Build/$BUILD_TYPE/compile_commands.json .
fi

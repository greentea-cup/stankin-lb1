#!/bin/sh

export CC=${CC:-clang} CXX=${CXX:-clang}
echo "Using CC=$CC"
echo "Using CXX=$CXX"
BUILD_TYPE="${BUILD_TYPE:-${1:-Release}}"
echo "Using BUILD_TYPE=$BUILD_TYPE"
export EXPORT_COMPILE_COMMANDS="${EXPORT_COMPILE_COMMANDS:-$CCMDS}"
if [ "$EXPORT_COMPILE_COMMANDS" == "0" ]; then
	unset EXPORT_COMPILE_COMMANDS
fi
echo "Using EXPORT_COMPILE_COMMANDS=$EXPORT_COMPILE_COMMANDS"

# clear cache
echo "Clear build cache"
rm -rf Build/Release Build/Debug compile_commands.json
# regen for both Release and Debug presets
echo "Generating Release"
cmake --preset Release
echo "Generating Debug"
cmake --preset Debug

if [ ! -z "$EXPORT_COMPILE_COMMANDS" ]; then
	echo "Making symlink to Build/$BUILD_TYPE/compile_commands.json"
	ln -s Build/$BUILD_TYPE/compile_commands.json .
fi

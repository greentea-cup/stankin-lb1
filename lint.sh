#!/bin/sh
clang-tidy $(find "src" "include" -type f -regextype posix-extended -iregex ".*\.(c|cxx|cpp|h|hpp)") -- Iinclude

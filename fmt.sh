#!/bin/sh
find "src" "include" -type f -regextype posix-extended -iregex ".*\.(c|cxx|cpp|h|hpp)" | xargs astyle -Tym0xUoO


#!/bin/sh
find "src" "include" -type f -regextype posix-extended -iregex ".*\.(c|cxx|cpp|h|hpp)" | xargs astyle -n -T -y -m0 -xU -O -H -p -xg -p -U -L


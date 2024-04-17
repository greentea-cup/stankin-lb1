#!/bin/sh
find "src" "include" -type f -regextype posix-extended -iregex ".*\.(c|cxx|cpp|h|hpp)" | xargs astyle \
	-n -T4 -y -m0 -o -O -xU -H -p -xg -p -U -L -xC100 -xL -k3 --squeeze-ws --squeeze-lines=1 -w


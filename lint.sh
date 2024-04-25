#!/bin/sh
clang-tidy $(find src -type f) -p Build/Release

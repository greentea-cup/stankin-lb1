rm .\Build\x64-debug\ -r -force 2>$null; rm .\Build\x64-release\ -r -force 2>$null; clear; .\regen_cmake.bat; if (0 -eq 0) { clear; .\build.bat }

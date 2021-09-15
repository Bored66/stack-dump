rm CMakeCache.txt
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DWIN32=ON CMakeLists.txt
mingw32-make

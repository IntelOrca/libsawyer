#!/bin/sh
set -e

rm -rf bin out
cmake -G Ninja -B bin
# cmake -G Ninja -B bin -DCMAKE_BUILD_TYPE=Debug
# cmake -G Ninja -B bin
cmake --build bin
# cmake --install bin
cmake --build bin --target tests
bin/tests
# ctest --test-dir bin

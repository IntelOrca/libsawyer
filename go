#!/bin/sh
set -e

if [ -z "$CONFIGURATION" ]; then
    CONFIGURATION=Release
fi

# LIBSAWYER
rm -rf bin out
cmake -G Ninja -B bin -DCMAKE_BUILD_TYPE=$CONFIGURATION
cmake --build bin
cmake --install bin --prefix out

cmake --build bin --target tests
bin/tests

# FSAW
cd tools/fsaw
rm -rf bin out
cmake -G Ninja -B bin -DCMAKE_BUILD_TYPE=$CONFIGURATION
cmake --build bin
./bin/fsaw

# SPRITEC
cd tools/spritec
rm -rf bin out
cmake -G Ninja -B bin -DCMAKE_BUILD_TYPE=$CONFIGURATION
cmake --build bin
./bin/spritec

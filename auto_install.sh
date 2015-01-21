#!/bin/bash          
set -e

# Build Pangolin
git submodule update --init
cd external/Pangolin
cmake -DBUILD_PANGOLIN_VIDEO=OFF -DBUILD_EXAMPLES=OFF
make -j8

# Build the main
cd ../../
mkdir build
mkdir bin
cd build
cmake ../
make -j8
cd ../

#clean up
rm -rf external/Pangolin
rm -rf build

#!/bin/bash          
set -e

# Build Pangolin
printf '\n\e[1;36m==== Building Pangolin ====\e[0;39m\n'
git submodule update --init
cd external/Pangolin
cmake -DBUILD_PANGOLIN_VIDEO=OFF -DBUILD_EXAMPLES=OFF
make -j8

# Build the main
printf '\n\e[1;36m==== Building LabelBspline ====\e[0;39m\n'
cd ../../
mkdir -p build
mkdir -p bin
cd build
cmake ..
make -j8
cd ..

#clean up
printf '\n\e[1;36m==== Clean Up ====\e[0;39m\n'
git submodule deinit -f external/Pangolin
rm -rf build

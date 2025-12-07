#!/bin/bash

if [ -d build ]; then
    echo "deleting existing build directory"
    rm -rf build
fi

if [ -d install ]; then
    echo "deleting existing install directory"
    rm -rf install
fi

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DALEMBIC_SHARED_LIBS=OFF -DENABLE_TESTING=OFF -DUSE_TESTS=OFF -B build

cmake --build build

cmake --build build --target install

# create a example particle file
./install/bin/tools/particle_file_generator -box -500 -500 -500 500 500 500 -mass 1 100 -vel 0 20 -acc 0 5 -n 50000 -f particles.txt
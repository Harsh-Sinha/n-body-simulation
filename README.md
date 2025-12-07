# Requirements
I make use of CMake for my build system, and you must have a valid C++ compiler installed.

# Getting Started

Make use of the `setup_env.sh` script. This will init and update all the git submodules for this repo, build and install the solution, and create a sample particle file with 50k particles.   
   
Note: This script will call the `build.sh` script which builds and installs the solution. This script by default builds a release version with unit testing of my barnes hut algorithm disabled and perf profiling disabled. To use either of these you will need to edit the bash script and turn them from OFF to ON.  

The install directory is as the following:  
`install/` - this gets placed under the root directory of the repo  
&emsp;`bin/` - where my barnes hut and benchmark octree executables are  
&emsp;&emsp;`tools/` - this is where the utility to generate particle files are  
&emsp;`inc/` - this is where my headers are  
&emsp;`include/` - this is where alembic headers are  
&emsp;`lib/` - this is where alembic and my static libs are placed  
&emsp;`third_party/` - this is where imath include and libs are stored  

If you want to use the python tools to plot timing results and create tables from the perf section data, then you will want to execute the following:  
```
source setup_py.sh
```
This will create a virtual environment, activate it, and install the required packages.


# n-body-simulation
```
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=install -DALEMBIC_SHARED_LIBS=OFF -DENABLE_TESTING=OFF -DUSE_TESTS=OFF -B debug   
cmake --build debug -j 2    
cmake --build debug --target install -j 2
```
## TODO
make sure you do  
`git submodule update --init --recursive`  
command to build rn:  
`cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=install -DALEMBIC_SHARED_LIBS=OFF -DENABLE_TESTING=OFF -DUSE_TESTS=OFF -B debug`
## build
To configure the cmake build system execute the following commands. Note: inside a command there may be `[]` which indicate a user has to make a choice to make use of.  
`cmake -DCMAKE_BUILD_TYPE=[Release/Debug] -DCMAKE_INSTALL_PREFIX=install -DENABLE_TESTING=[TRUE/FALSE] -B build`  
  
now to build the targets  
`cmake --build build --target install`  
## tests
If configured and built with `-DENABLE_TESTING=TRUE`, then you can do the following to execute all unit tests:  
`ctest --test-dir build --output-on-failure`  

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

# Unit Tests
If configured and built with `-DENABLE_TESTING=ON`, then you can do the following to execute all unit tests:  
`ctest --test-dir build --output-on-failure`  

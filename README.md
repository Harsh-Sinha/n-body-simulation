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

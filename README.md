# Requirements
I make use of CMake for my build system, and you must have a valid C++ compiler installed.  

Note: When building on Great Lakes ensure that you DO NOT have the gcc module loaded. Run the following prior to building:  
```
module unload gcc
```

# Getting Started

Make use of the `setup_env.sh` script. This will init and update all the git submodules for this repo, build and install the solution, and create a sample `particle.txt` file with 50k particles.   
```
./setup_env.sh
```
   
Note: This script will call the `build.sh` script which builds and installs the solution. This script by default builds a release version with unit testing of my barnes hut algorithm disabled and perf profiling disabled. To use either of these you will need to edit the bash script and turn them from OFF to ON. You can ONLY have ONE enabled: either `-DPERF_PROFILING` or `-DENABLE_TESTING` 

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

# Usage
The following subsections will go over how to use the Barnes-Hut and tools. They all assume that the current working directory is the root of the repo.

## Barnes-Hut
Keep in mind that the `-p` only enables time profiling. If the project was configured and built with `-DPERF_PROFILING=ON`, then when the executable is ran perf profiling automatically happens (regardless of whether `-p` was specified. The simulation will generate up to 3 files:  
`simulationName.abc` - alembic file that will need to be imported in open source software such as [Blender](https://www.blender.org/)  
`simulationName.txt` - file that contains the time profiling data if ran with `-p`  
`simulationName.perf.txt` - file containing perf profiling data for each algorithm if configured and built with `-DPERF_PROFILING=ON`  
```
./install/bin/b_hut -t A -l B -in particleConfig -out simulationName -p
A - time step (s)
B - length of simulation (s)
particleConfig - particle config file for the simulation
simulationName - name to be assigned to this simulation... no spaces and file extension
-p - optional flag that turns on profiling for barnes hut
```

## Particle File Generator
This is the tool which can generate particle config files in the expected file format. It creates N particles inside a specified bounding box with random inital positions, velocities, and accelerations.
```
./install/bin/tools/particle_file_generator -box A B C D E F -mass H I -vel J K -acc L M -n N -f file_name
A,B,C - lower limits of bounding box
D,E,F - upper limits of bounding box
H,I - mass limits for particles
J,K - velocity limits for particles
L,M - acceleration limits for particles
file_name - output file name
```

## Plot Timing Results
This is a python script and requires that the repo's python virtual environment has been setup and activated. This takes the timing data generated using the slurm scripts and creates scaling and speedup plots.
```
tools/plot_timing_results.py [-h] [--input INPUT] [--output OUTPUT]

plot profiling data.

options:
  -h, --help       show this help message and exit
  --input INPUT    directory containing profiling .txt files
  --output OUTPUT  directory to save output plots
```

## Tabulate Perf Data
This is a python script and requires that the repo's python virtual environment has been setup and activated. This takes the perf data generated using the slurm scripts and creates latex tables of events/metrics logged for different thread counts and particle counts.
```
tools/tabulate_perf_data.py [-h] [--input INPUT] [--output OUTPUT]

plot perf data.

options:
  -h, --help       show this help message and exit
  --input INPUT    directory containing .perf.txt files
  --output OUTPUT  directory to save output tables
```

# Unit Tests
If configured and built with `-DENABLE_TESTING=ON`, then you can do the following to execute all unit tests:  
```
ctest --test-dir build --output-on-failure
```

# Slurm
This folder contains all my slurm scripts that I used to create jobs/tasks on Great Lakes to generate the timing and perf profile data. These scripts must ran or `sbatch`-ed within the `slurm` folder (current working directory must be `slurm` folder).  

Note: Ensure that you have ran the following before `sbatch`-ing any slurm script:  
```
module load gcc
```

# Timing Results
This folder contains the timing and perf profile data from my latest run on Great Lakes. It has the following folders containing:  
`impl` - this contains timing data of the barnes hut executable generated from running `./batch_all.sh`  
`non_morton` - this contains timing data of barnes hut without morton ordering of leaf nodes generated from running `./batch_all.sh`  
`octree` - this contains the octree insert strategies timing data generated from running `sbatch benchmark_octree.sh`  
`perf` - this contains the perf data for each section of the barnes hut generated from running `./batch_perf.sh` 

# Reference Barnes Hut
Under `external/barnes-hut-simulation` you will find the forked repo of reference implementation I used to verify correctness. It has its own readme on how to build and run the barnes hut executable.

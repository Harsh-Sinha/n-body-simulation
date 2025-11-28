#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)

# Set up batch job settings
#SBATCH --job-name=cse587_semester_project
#SBATCH --cpus-per-task=36
#SBATCH --exclusive
#SBATCH --time=00:15:00
#SBATCH --account=cse587f25s001_class
#SBATCH --partition=standard

export OMP_NUM_THREADS=2  && ./../install/bin/benchmark_octree > results_p2.txt
export OMP_NUM_THREADS=4  && ./../install/bin/benchmark_octree > results_p4.txt
export OMP_NUM_THREADS=9  && ./../install/bin/benchmark_octree > results_p9.txt
export OMP_NUM_THREADS=18 && ./../install/bin/benchmark_octree > results_p18.txt
export OMP_NUM_THREADS=36 && ./../install/bin/benchmark_octree > results_p36.txt

#!/bin/bash
# (See https://arc-ts.umich.edu/greatlakes/user-guide/ for command details)

# Set up batch job settings
#SBATCH --job-name=cse587_semester_project
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=36
#SBATCH --exclusive
#SBATCH --time=00:15:00
#SBATCH --account=cse587f25s001_class
#SBATCH --partition=standard

# generate particle files for this run
./../install/bin/tools/particle_file_generator -box -500 -500 -500 500 500 500 -mass 10 100 -vel 10 40 -acc 0 5 -n 10000 -f particle_ten_thousand.txt
./../install/bin/tools/particle_file_generator -box -500 -500 -500 500 500 500 -mass 10 100 -vel 10 40 -acc 0 5 -n 100000 -f particle_hundred_thousand.txt
./../install/bin/tools/particle_file_generator -box -500 -500 -500 500 500 500 -mass 10 100 -vel 10 40 -acc 0 5 -n 500000 -f particle_five_hundred_thousand.txt
./../install/bin/tools/particle_file_generator -box -500 -500 -500 500 500 500 -mass 10 100 -vel 10 40 -acc 0 5 -n 1000000 -f particle_million.txt

export OMP_NUM_THREADS=9

# perform tests (do 10 iterations of the simulation)
./../install/bin/b_hut -t 0.1 -l 1 -in particle_ten_thousand.txt -out ten_thousand_p9 -p
./../install/bin/b_hut -t 0.1 -l 1 -in particle_hundred_thousand.txt -out hundred_thousand_p9 -p
./../install/bin/b_hut -t 0.1 -l 1 -in particle_five_hundred_thousand.txt -out five_hundred_thousand_p9 -p
./../install/bin/b_hut -t 0.1 -l 1 -in particle_million.txt -out million_p9 -p

# cleanup
rm particle_ten_thousand.txt
rm particle_hundred_thousand.txt
rm particle_five_hundred_thousand.txt
rm particle_million.txt
rm ten_thousand_p9.bin
rm hundred_thousand_p9.bin
rm five_hundred_thousand_p9.bin
rm million_p9.bin
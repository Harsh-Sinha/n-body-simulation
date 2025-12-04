#!/bin/bash

rm *.txt *.zip slurm*

module load gcc

sbatch benchmark_perf_p1.sh
sbatch benchmark_perf_p9.sh
sbatch benchmark_perf_p18.sh
sbatch benchmark_perf_p36.sh

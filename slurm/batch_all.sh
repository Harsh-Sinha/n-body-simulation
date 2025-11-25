#!/bin/bash

module load gcc

sbatch benchmark_scaling_p1.sh
sbatch benchmark_scaling_p9.sh
sbatch benchmark_scaling_p18.sh
sbatch benchmark_scaling_p36.sh

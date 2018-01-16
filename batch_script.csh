#!/bin/tcsh
#
#BSUB -L /bin/tcsh
#
# This sets the total number of cores to use
# (combined with -R, below, this sets the number of nodes to use)
#BSUB -n 6
#
# This sets how many cores to use on each node
#BSUB -R "span[ptile=3]"
#
#BSUB -J my_job_name
#BSUB -oo my_job_out
#BSUB -q cosma
#BSUB -P durham
#BSUB -W 1:00

# Need to load the same MPI module which parallel_tasks was built with
module purge
module load intel_comp/c4/2015 platform_mpi/9.1.2 python/2.7.10

#
# Run the python script
#
# This runs 20 instances of hello.py, passing the job index 1-20
# as a command line parameter. The number of cpus used depends
# on the -n and -R flags at the top of this script.
#
# The number of jobs to run is independent of the number of cpus used.
# Running more jobs or using fewer cpus will just take longer.
#
# Any instances of %d in the command get replaced with the job index.
#
mpirun ./parallel_tasks 1 20 "python ./hello.py %d"


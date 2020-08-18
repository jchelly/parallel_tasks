#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>

#include "identify_format.h"
#include "job_info.h"
#include "run_job.h"
#include "get_time.h"
#include "nanosec_sleep.h"
#include "master_task.h"
#include "worker_task.h"


int main(int argc, char *argv[])
{
  int njobs_tot;
  int ijob;
  int *job_result_all;
  double *job_time_all;
  int nfailed;

  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
  if(provided<MPI_THREAD_FUNNELED)
    {
      printf("This program needs MPI with at least MPI_THREAD_FUNNELED thread support\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);
  MPI_Comm_size(MPI_COMM_WORLD, &NTask);

  /* Read command line args */
  if(ThisTask==0)
    {
      if(argc != 4)
	{
	  printf("Usage: parallel_tasks ifirst ilast command\n");
	  MPI_Abort(MPI_COMM_WORLD, 1);
	}
      sscanf(argv[1], "%d", &ifirst);
      sscanf(argv[2], "%d", &ilast);
      strncpy(command, argv[3], COMMAND_LENGTH);
      command[COMMAND_LENGTH-1] = (char) 0;

      if((ifirst < 0) || (ilast < 0))
	{
	  printf("Job index must be non-negative!\n");
	  MPI_Abort(MPI_COMM_WORLD, 1);  
	}
    }

  if(ThisTask==0)
    printf("Parallel tasks - command is: %s\n", command);

  /* Broadcast arguments */
  MPI_Bcast(&ifirst, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&ilast,  1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(command, COMMAND_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

  /* Initial number of jobs to assign */
  njobs_tot      = ilast-ifirst+1;

  /* Initialize results table */
  job_result = malloc(sizeof(int)*njobs_tot);
  job_time   = malloc(sizeof(double)*njobs_tot);
  for(ijob=0;ijob<njobs_tot;ijob+=1)
    {
      job_result[ijob] = INT_MIN;
      job_time[ijob] = -1.0;
    }

  /* Start the clock */
  double start_time = get_time();

  /* Run jobs until all are done */
  if(ThisTask==0)
    {
      master_task();
    }
  else
    {
      worker_task();
    }

  /* If not using a terminate signal, all wait here once their jobs are done */
  MPI_Barrier(MPI_COMM_WORLD);

  /* Stop the clock */
  double finish_time = get_time();

  /* Gather exit codes and run times */
  job_result_all = malloc(sizeof(int)*njobs_tot);
  MPI_Reduce(job_result, job_result_all, njobs_tot, MPI_INT, MPI_MAX, 0, 
             MPI_COMM_WORLD);
  job_time_all = malloc(sizeof(double)*njobs_tot);
  MPI_Reduce(job_time, job_time_all, njobs_tot, MPI_DOUBLE, MPI_MAX, 0, 
             MPI_COMM_WORLD);
  
  if(ThisTask==0)
    {
      /* Calculate idle time */
      double total_time = (finish_time-start_time)*NTask;
      double idle_time = total_time;
      double max_time = 0.0;
      double min_time = total_time;
      for(ijob=0;ijob<njobs_tot;ijob+=1)
        {
          idle_time -= job_time_all[ijob];
          min_time = job_time_all[ijob] < min_time ? job_time_all[ijob] : min_time;
          max_time = job_time_all[ijob] > max_time ? job_time_all[ijob] : max_time;
        }

      /* Generate a report */      
      printf("\nElapsed time:\n");
      printf("-------------\n");
      printf("\nRan jobs %d to %d in %f seconds (total no. of jobs = %d)\n", 
             ifirst, ilast, (finish_time-start_time), njobs_tot);
      printf("Shortest job: %f seconds\n", min_time);
      printf("Longest  job: %f seconds\n", max_time);
      printf("Idle time: %.2f%%\n", idle_time/total_time*100);
      printf("\nJob exit codes:\n");
      printf("---------------\n\n");
      nfailed = 0;
      for(ijob=0;ijob<njobs_tot;ijob+=1)
        {
          if(job_result_all[ijob] != 0)
            {
              nfailed += 1;
              printf("ERROR: Job %d returned non-zero exit code %d\n",
                     ijob+ifirst, job_result_all[ijob]);
            }
        }
      if(nfailed>0)printf("\n");
    }
  free(job_result_all);
  free(job_result);
  free(job_time_all);
  free(job_time);
  MPI_Bcast(&nfailed, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if(ThisTask==0)
    {
      if(nfailed==0)
        printf("All jobs completed with zero exit code\n");
      else
        printf("Number of jobs with non-zero exit code: %d of %d\n", nfailed, njobs_tot);
    }
  
  MPI_Finalize();
  
  /* All MPI tasks return non-zero exit code if any job failed */
  if(nfailed==0)
      return 0;
  else
      return 1;
}
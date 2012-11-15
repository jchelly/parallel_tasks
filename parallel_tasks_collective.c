#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>

#define SLEEP_TIME 5

static int need_job;
static int NTask;
static int ThisTask;

#define COMMAND_LENGTH 500
static char command[500];

void run_job(void *ptr)
{
  char cmd_exec[COMMAND_LENGTH];
  int ijob = *((int *) ptr);
  printf("Running job %d on process %d\n", ijob, ThisTask);
  sprintf(cmd_exec, command, ijob);
  system(cmd_exec);
  printf("Job %d on process %d finished\n", ijob, ThisTask);
  need_job = 1;
  return;
}

int main(int argc, char *argv[])
{
  int ifirst, ilast;
  int njobs_tot;
  int *proc_needs_job;
  int *job_assigned;
  int next_to_assign;
  int iproc;
  int job_received;
  int nfinished;
  int finished;

  MPI_Init(&argc, &argv);
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
    }

  /* Broadcast arguments */
  MPI_Bcast(&ifirst, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&ilast,  1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(command, COMMAND_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

  if(ThisTask==0)
    {
      proc_needs_job = malloc(sizeof(int)*NTask);      
      job_assigned   = malloc(sizeof(int)*NTask);
    }
  else
    {
      proc_needs_job = NULL;
      job_assigned   = NULL;
    }

  /* Initial number of jobs to assign */
  njobs_tot      = ilast-ifirst+1;
  next_to_assign = ifirst;

  /* Number of processors which have finished */
  nfinished = 0;
  finished  = 0;

  /* Initially all processes need to be assigned a job */
  need_job = 1;
  
  /* Loop until all jobs have been done */
  while(nfinished < NTask)
    {
      /* 
	 First process needs to determine which others need
	 to be assigned a job
      */
      MPI_Gather(&need_job,      1, MPI_INT, 
		 proc_needs_job, 1, MPI_INT, 
		 0, MPI_COMM_WORLD);
      if(ThisTask==0)
	{
	  for(iproc=0;iproc<NTask;iproc+=1)
	    {
	      if(proc_needs_job[iproc] > 0)
		{
		  if(next_to_assign <= ilast)
		    {
		      /* Assign next job to process iproc */
		      job_assigned[iproc] = next_to_assign;
		      next_to_assign += 1;
		    }
		  else
		    {
		      /* Process iproc is free, but no jobs are left */
		      job_assigned[iproc] = -1;
		    }
		}
	      else
		{
		  /* Process iproc is already running something */
		  job_assigned[iproc] = -2;
		}
	    }
	}
      MPI_Scatter(job_assigned, 1, MPI_INT, 
		  &job_received, 1, MPI_INT, 
		  0, MPI_COMM_WORLD);
      if(job_received >= 0)
	{
	  /* We've been given a job, so try to run it */
	  pthread_t job_thread;
	  need_job = 0;
	  if(pthread_create(&job_thread, NULL, 
			    (void *) &run_job, 
			    (void *) &job_received)==0);
	  else
	    {
	      printf("Unable to create thread!\n");
	      MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	}
      else if(job_received==-1)
	{
	  /* 
	     We asked for a job, but none are left in the queue
	     so this process is done
	  */
	  if(!finished)
	    printf("No jobs left for process %d\n", ThisTask);
	  finished = 1;
	}
      
      /* Count how many processors are idle with no more jobs left */
      MPI_Allreduce(&finished, &nfinished, 1, 
		    MPI_INT, MPI_SUM, MPI_COMM_WORLD);

      /* Go back to sleep for a bit if jobs are still running */
      if(nfinished < NTask)
	sleep(SLEEP_TIME);      
    }

  MPI_Barrier(MPI_COMM_WORLD);
  if(ThisTask==0)
    printf("All jobs complete.\n");

  MPI_Finalize();
  
  return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>

/* Time delay (secs) between checks for job requests */
#define SLEEP_TIME 1

/* Number of processors, ID of this processor */
static int NTask;
static int ThisTask;

/* Flag to signal job completion */
volatile int job_running;

/* Maximum length of command to execute */
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
  job_running = 0;
  return;
}

int main(int argc, char *argv[])
{
  int ifirst, ilast;
  int njobs_tot;
  int next_to_assign;
  int iproc;
  int job_received;

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

      if((ifirst < 0) || (ilast < 0))
	{
	  printf("Job index must be non-negative!\n");
	  MPI_Abort(MPI_COMM_WORLD, 1);  
	}
    }

  /* Broadcast arguments */
  MPI_Bcast(&ifirst, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&ilast,  1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(command, COMMAND_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

  /* Initial number of jobs to assign */
  njobs_tot      = ilast-ifirst+1;
  next_to_assign = ifirst;

  if(ThisTask==0)
    {
      /*
	Processor 0 needs to handle job requests and 
	run its own jobs at the same time. Jobs on this
	process are run in a separate thread so we can continue
	responding to job requests. The main thread spends most
	of its time sleeping to avoid taking cpu time from the jobs.
      */
      int nfinished  = 0;
      int proc0_done = 0;
      int local_job  = -1;
      pthread_t job_thread;
      job_running    = 0;
      while((nfinished < NTask-1) || (proc0_done==0))
	{
	  int flag;
	  MPI_Status probe_status;
	  MPI_Status recv_status;
	  /* If no local job is running, try to start one */
	  if(job_running == 0)
	    {
	      /* Wait for thread which ran the previous job to finish */
	      if(local_job >= 0)
		{
		  pthread_join(job_thread, NULL);
		  local_job = -1;
		}

	      /* Check if we have jobs left to assign */
	      if(next_to_assign <= ilast)
		{
		  /* Launch the next job in a new thread */
		  job_running = 1;
		  local_job = next_to_assign;
		  pthread_create(&job_thread, NULL, 
				 (void *) &run_job, 
				 (void *) &local_job);
		  next_to_assign += 1;
		}
	      else
		{
		  /* No more jobs left */
		  proc0_done = 1;
		}
	    }

	  /* 
	     Check for job requests from other processes.
	     May be more than one message waiting
	  */
	  flag = 1;
	  while(flag)
	    {
	      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, 
			 &flag, &probe_status);
	      if(flag)
		{
		  /* We have a job request to deal with  */
		  int ireq;
		  int ijob;
		  MPI_Recv(&ireq, 1, MPI_INT, 
			   probe_status.MPI_SOURCE, probe_status.MPI_TAG, 
			   MPI_COMM_WORLD, &recv_status);
		  /* Check if we have jobs to hand out */
		  if(next_to_assign <= ilast)
		    {
		      ijob = next_to_assign;
		      next_to_assign += 1;
		    }
		  else
		    {
		      /* No more jobs for this processor */
		      nfinished += 1;
		      ijob = -1;
		    }
		  /* Send the job index back */
		  MPI_Send(&ijob, 1, MPI_INT, probe_status.MPI_SOURCE, 
			   0, MPI_COMM_WORLD);
		}
	    }

	  /* Go back to sleep */
	  sleep(SLEEP_TIME);
	}
    }
  else
    {
      /*
	Processors other than 0 request and execute jobs
	until there are none left (signalled by -ve job ID).
      */
      while(1)
	{
	  int ireq = 1;
	  int ijob;
	  MPI_Status status;
	  /* Ask for a job */
	  MPI_Sendrecv(&ireq, 1, MPI_INT, 0, 0,
		       &ijob, 1, MPI_INT, 0, 0,
		       MPI_COMM_WORLD, &status);
	  if(ijob >= 0)
	    {
	      /* Run the job if we got one */
	      run_job(&ijob);
	    }
	  else
	    {
	      /* If there are no jobs left, we're done */
	      break;
	    }
	}
    }

  MPI_Barrier(MPI_COMM_WORLD);
  if(ThisTask==0)
    printf("All jobs complete.\n");

  MPI_Finalize();
  
  return 0;
}

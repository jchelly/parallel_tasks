CC      = mpicc
CFLAGS  = -g -O0 -Wall -Werror -pthread

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC)  $(CFLAGS)   -c $< -o $*.o

OBJS = main.o identify_format.o job_info.o run_job.o get_time.o nanosec_sleep.o master_task.o worker_task.o

all:	parallel_tasks

master_task.o:	master_task.h run_job.h job_info.h get_time.h nanosec_sleep.h

worker_task.o:	worker_task.h run_job.h job_info.h nanosec_sleep.h

identify_format.o:	identify_format.h

job_info.o:	job_info.h

run_job.o:	run_job.h job_info.h identify_format.h get_time.h

get_time.o:	get_time.h

nanosec_sleep.o:	nanosec_sleep.h

main.o:	identify_format.h job_info.h run_job.h get_time.h nanosec_sleep.h master_task.h worker_task.h

parallel_tasks:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o parallel_tasks $(LDFLAGS)

clean:
	\rm -f *.o parallel_tasks

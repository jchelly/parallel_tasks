CC      = mpicc
CFLAGS  = -g
LDFLAGS = -Wl,-rpath=/gal/jcha/jch/software/install/lib/

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	$(CC)  $(CFLAGS)   -c $< -o $*.o

parallel_tasks:	parallel_tasks.o
	$(CC) $(CFLAGS) parallel_tasks.o -o parallel_tasks $(LDFLAGS)

clean:
	\rm -f *.o parallel_tasks

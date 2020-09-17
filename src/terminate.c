#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#include "runner.h"

void terminate(int err) {
  
  runner_shutdown();
  MPI_Abort(MPI_COMM_WORLD, err);
  
}

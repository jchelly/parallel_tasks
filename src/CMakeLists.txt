add_executable(parallel_tasks 
  main.c identify_format.c job_info.c run_job.c get_time.c nanosec_sleep.c
  master_task.c worker_task.c)

target_link_libraries(parallel_tasks PRIVATE MPI::MPI_C)

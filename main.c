#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <inttypes.h>
#include "liblfds711.h"

typedef unsigned long long ull;

typedef struct 
{
  volatile int *stop;
  pthread_t thread; 
  int id;
  ull executions;
} task_t __attribute__ ((aligned (64)));;

struct lfds711_list_asu_state lasus;

void
set_cpu(int cpu) 
{
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);
  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
  if (ret != 0) {
      perror("pthread_set_affinity_np");
      exit(-1);
  }
}

void *run_test(void *arg) {
  task_t *task = (task_t *) arg;
  set_cpu(task->id);
  LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  ull reps;
  for(reps = 0; !*task->stop; reps++) {
    struct lfds711_list_asu_element *element = malloc(sizeof(struct lfds711_list_asu_element));
    int *x = malloc(sizeof(int));
    *x = task->id;
    LFDS711_LIST_ASU_SET_KEY_IN_ELEMENT(*element, NULL);
    LFDS711_LIST_ASU_SET_VALUE_IN_ELEMENT(*element, x);
    lfds711_list_asu_insert_at_position(&lasus, element, NULL, LFDS711_LIST_ASU_POSITION_END);
  }
  task->executions = reps;
  printf("Thread %02d (CPU %d) "
          "executions %llu \n",
          //"schedstat %s",
          task->id, task->id,
          task->executions);
  return 0;
}

int main(int argc, char *argv[])
{
  lfds711_list_asu_init_valid_on_current_logical_core(&lasus, NULL);
  int stop __attribute__((aligned (64))) = 0;
  int test_duration = atoi(argv[1]);
  int test_threads = atoi(argv[2]);
  task_t *tasks = malloc(sizeof(task_t) * test_threads);
  for (int i = 0; i < test_threads; i++) {
        tasks[i].stop = &stop;
        tasks[i].id = i;
        tasks[i].executions = 0;
  }
  for(int i = 0; i < test_threads; i++) {
    pthread_create(&tasks[i].thread, NULL, run_test, &tasks[i]);
  }
  sleep(test_duration);
  stop = 1;
  ull min_executions = 99999999999;
  ull max_executions = 0;
  ull total_executions = 0;
  for (int i = 0; i < test_threads; i++) { 
    pthread_join(tasks[i].thread, NULL);
    total_executions = total_executions + tasks[i].executions;
    if(tasks[i].executions > max_executions) max_executions = tasks[i].executions;
    if(tasks[i].executions < min_executions) min_executions = tasks[i].executions;
  }
  double average_executions = (1.0 * total_executions) / test_threads;
  double fairness_index = 0;
  double denominator = 0;
  double nominator = 0;
  for(int i = 0; i < test_threads; i++) {
    denominator += (tasks[i].executions * tasks[i].executions);
    nominator += tasks[i].executions;
  }
  fairness_index = (nominator * nominator) / (test_threads * denominator);
  printf("Execution: Total = %llu, Avg = %.2f Max = %llu Min = %llu\n", total_executions, average_executions, max_executions, min_executions); 

  printf("Fairness index: %f\n", fairness_index);

  printf("Average atomic execution time(ns) = %f\n", (1000.0 * 1000 * 1000 * test_duration) / total_executions);
  //lfds711_list_asu_cleanup(&lasus, NULL);
  free(tasks);
  return 0;
}

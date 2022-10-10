/*   
 *   File: ccbench.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: the main functionality of ccbench
 *   ccbench.c is part of ccbench
 *
 * The MIT License (MIT)
 *
 * Copyright (C) 2013  Vasileios Trigonakis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ccbench.h"
//#include "rdtsc.h"

#define gettid() syscall(SYS_gettid)

//#define CYCLE_PER_S 2300000000

typedef unsigned long long ull;

uint8_t ID;
unsigned long* seeds;

#if defined(__tile__)
cpu_set_t cpus;
#endif

moesi_type_t test_test = DEFAULT_TEST;
uint32_t test_cores = DEFAULT_CORES;
uint32_t test_reps = DEFAULT_REPS;
uint32_t test_duration = DEFAULT_DURATION;
uint32_t test_threads = DEFAULT_THREADS;
uint32_t test_core1 = DEFAULT_CORE1;
uint32_t test_core2 = DEFAULT_CORE2;
uint32_t test_core3 = DEFAULT_CORE3;
uint32_t test_core_others = DEFAULT_CORE_OTHERS;
uint32_t test_flush = DEFAULT_FLUSH;
uint32_t test_verbose = DEFAULT_VERBOSE;
uint32_t test_print = DEFAULT_PRINT;
uint32_t test_stride = DEFAULT_STRIDE;
uint32_t test_fence = DEFAULT_FENCE;
uint32_t test_ao_success = DEFAULT_AO_SUCCESS;
size_t   test_mem_size = CACHE_LINE_NUM * sizeof(cache_line_t);
uint32_t test_cache_line_num = CACHE_LINE_NUM;
uint32_t test_lfence = DEFAULT_LFENCE;
uint32_t test_sfence = DEFAULT_SFENCE;
uint32_t test_placement = DEFAULT_PLACEMENT;

static void store_0(volatile cache_line_t* cache_line, volatile uint64_t reps);
static void store_0_no_pf(volatile cache_line_t* cache_line, volatile uint64_t reps);
static void store_0_eventually(volatile cache_line_t* cl, volatile uint64_t reps);
static void store_0_eventually_pfd1(volatile cache_line_t* cl, volatile uint64_t reps);

static uint64_t load_0(volatile cache_line_t* cache_line, volatile uint64_t reps);
static uint64_t load_next(volatile uint64_t* cl, volatile uint64_t reps);
static uint64_t load_0_eventually(volatile cache_line_t* cl, volatile uint64_t reps);
static uint64_t load_0_eventually_no_pf(volatile cache_line_t* cl);

static void invalidate(volatile cache_line_t* cache_line, uint64_t index, volatile uint64_t reps);
static uint32_t cas(volatile cache_line_t* cache_line, volatile uint64_t reps);
static uint32_t cas_0_eventually(volatile cache_line_t* cache_line, volatile uint64_t reps);
static uint32_t cas_no_pf(volatile cache_line_t* cache_line, volatile uint64_t reps);
static uint32_t fai(volatile cache_line_t* cache_line, volatile uint64_t reps);
static uint8_t tas(volatile cache_line_t* cl, volatile uint64_t reps);
static uint32_t swap(volatile cache_line_t* cl, volatile uint64_t reps);

static size_t parse_size(char* optarg);
static void create_rand_list_cl(volatile uint64_t* list, size_t n);

#define Hyperthreading 0
#define Intra_socket 1
#define Inter_socket 2
#define Random 3
#define min_cpu 0
#define max_cpu 39

pthread_mutex_t lock;
volatile int cpu_status[40] = {};

volatile cache_line_t* cache_line;

typedef struct {
    volatile int *stop;
    pthread_t thread;
    //int priority;
    int id;
    int ncpu;
    // outputs
    ull operation_executed;
} task_t __attribute__ ((aligned (64)));

void *run_test(void *arg) {
  task_t *task = (task_t *) arg;
  int cpu = 0;
  if (task->ncpu != 0) {
        //cpu_set_t cpuset;
        //CPU_ZERO(&cpuset);
        /*for (int i = 0; i < task->ncpu; i++) {
            if (i < 8 || i >= 24)
                CPU_SET(i, &cpuset);
            else if (i < 16)
                CPU_SET(i+8, &cpuset);
            else
                CPU_SET(i-8, &cpuset);
        }*/
        
        if (test_placement == Hyperthreading) {
            if (task->id >= (test_threads / 2)) {
                cpu = (task->id / (test_threads / 2)) + 19 + (task->id % (test_threads / 2));
            } else {
                cpu = task->id;
            }
        } else if (test_placement == Intra_socket) {
            if (task->id % 20 < 10) {
                cpu = task->id % 20;
            } else {
                cpu = task->id % 20 + 10; 
            }
        } else if (test_placement == Inter_socket) {
            if ((task->id % 20) >= (test_threads / 2)) {
                cpu = ((task->id % 20) / (test_threads / 2)) + 9 + (task->id % 20  % (test_threads / 2));
            } else {
                cpu = task->id % 20;
            }
        } else if (test_placement == Random) {
            pthread_mutex_lock(&lock);
            do {
                cpu = (rand() % (max_cpu - min_cpu + 1)) + min_cpu;
            } while (cpu_status[cpu]);
            cpu_status[cpu] = 1;
            pthread_mutex_unlock(&lock);
        }else {
            cpu = task->id;
        }
        printf("thread %d = cpu %d\n", task->id, cpu);

        set_cpu(cpu);
  }
  ull reps = 0;
  uint64_t sum = 0;
  volatile uint64_t* cl = (volatile uint64_t*) cache_line;
  B0;
  PFDINIT(test_reps);
  B0;
  for (reps = 0; !*task->stop; reps++)
    {
      if (test_flush)
	{
	  _mm_mfence();
	  _mm_clflush((void*) cache_line);
	  _mm_mfence();
	}
      if(test_test != CAS &&
         test_test != FAI &&
         test_test != TAS &&
         test_test != SWAP) {
          B0;			/* BARRIER 0 */
      }
      switch (test_test)
	{
	case STORE_ON_MODIFIED: /* 0 */
	  {  
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		store_0_eventually(cache_line, reps);
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }
	    break;
	  }
	case STORE_ON_MODIFIED_NO_SYNC: /* 1 */
	  {
	    store_0_no_pf(cache_line, reps);
	    break;
	  }
	case STORE_ON_EXCLUSIVE: /* 2 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		store_0_eventually(cache_line, reps);
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }

	    if (!test_flush)
	      {
		cache_line += test_stride;
	      }
	    break;
	  }
	case STORE_ON_SHARED:	/* 3 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		break;
	      case 1:
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		store_0_eventually(cache_line, reps);
		break;
	      case 2:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;			/* BARRIER 2 */
		break;
	      default:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case STORE_ON_OWNED_MINE: /* 4 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;			/* BARRIER 2 */
		break;
	      case 1:
		store_0_eventually(cache_line, reps);
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		store_0_eventually_pfd1(cache_line, reps);
		break;
	      default:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case STORE_ON_OWNED:	/* 5 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		break;
	      case 1:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;			/* BARRIER 2 */
		store_0_eventually_pfd1(cache_line, reps);
		break;
	      default:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case STORE_ON_INVALID:	/* 6 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		B1;
		/* store_0_eventually(cache_line, reps); */
		store_0(cache_line, reps);
		if (!test_flush)
		  {
		    cache_line += test_stride;
		  }
		break;
	      case 1:
		invalidate(cache_line, 0, reps);
		if (!test_flush)
		  {
		    cache_line += test_stride;
		  }
		B1;
		break;
	      default:
		B1;
		break;
	      }
	    break;
	  }
	case LOAD_FROM_MODIFIED: /* 7 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		B1;		
		break;
	      case 1:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		break;
	      default:
		B1;
		break;
	      }
	    break;
	  }
	case LOAD_FROM_EXCLUSIVE: /* 8 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;			/* BARRIER 1 */

		if (!test_flush)
		  {
		    cache_line += test_stride;
		  }
		break;
	      case 1:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);

		if (!test_flush)
		  {
		    cache_line += test_stride;
		  }
		break;
	      default:
		B1;			/* BARRIER 1 */
		break;
	      }
	    break;
	  }
	case LOAD_FROM_SHARED:	/* 9 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		break;
	      case 1:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;			/* BARRIER 2 */
		break;
	      case 2:
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		sum += load_0_eventually(cache_line, reps);
		break;
	      default:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }

	    if (!test_flush)
	      {
		cache_line += test_stride;
	      }
	    break;
	  }
	case LOAD_FROM_OWNED:	/* 10 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		break;
	      case 1:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;			/* BARRIER 2 */
		break;
	      case 2:
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		sum += load_0_eventually(cache_line, reps);
		break;
	      default:
		B1;			/* BARRIER 1 */
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case LOAD_FROM_INVALID:	/* 11 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		B1;			/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps); 		/* sum += load_0(cache_line, reps); */
		break;
	      case 1:
		invalidate(cache_line, 0, reps);
		B1;			/* BARRIER 1 */
		break;
	      default:
		B1;			/* BARRIER 1 */
		break;
	      }

	    if (!test_flush)
	      {
		cache_line += test_stride;
	      }
	    break;
	  }
	case CAS: /* 12 */
	  {
	    sum += cas_0_eventually(cache_line, reps);
	    break;
	  }
	case FAI: /* 13 */
	  {
	    sum += fai(cache_line, reps);
	    break;
	  }
	case TAS:		/* 14 */
	  {
            sum += tas(cache_line, reps);
	    break;
	  }
	case SWAP: /* 15 */
	  {
	    sum += swap(cache_line, reps);
	    break;
	  }
	case CAS_ON_MODIFIED: /* 16 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		if (test_ao_success)
		  {
		    cache_line->word[0] = reps & 0x01;
		  }
		B1;		/* BARRIER 1 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		sum += cas_0_eventually(cache_line, reps);
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }
	    break;
	  }
	case FAI_ON_MODIFIED: /* 17 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		sum += fai(cache_line, reps);
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }
	    break;
	  }
	case TAS_ON_MODIFIED: /* 18 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		if (!test_ao_success)
		  {
		    cache_line->word[0] = 0xFFFFFFFF;
		    _mm_mfence();
		  }
		B1;		/* BARRIER 1 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		sum += tas(cache_line, reps);
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }
	    break;
	  }
	case SWAP_ON_MODIFIED: /* 19 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		store_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		sum += swap(cache_line, reps);
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }
	    break;
	  }
	case CAS_ON_SHARED: /* 20 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		sum += cas_0_eventually(cache_line, reps);
		break;
	      case 2:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;		/* BARRIER 2 */
		break;
	      default:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case FAI_ON_SHARED: /* 21 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		sum += fai(cache_line, reps);
		break;
	      case 2:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;		/* BARRIER 2 */
		break;
	      default:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case TAS_ON_SHARED: /* 22 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		if (test_ao_success)
		  {
		    cache_line->word[0] = 0;
		  }
		else
		  {
		    cache_line->word[0] = 0xFFFFFFFF;
		  }
		sum += load_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		sum += tas(cache_line, reps);
		break;
	      case 2:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;		/* BARRIER 2 */
		break;
	      default:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case SWAP_ON_SHARED: /* 23 */
	  {
	    switch (task->id % 4)
	      {
	      case 0:
		sum += load_0_eventually(cache_line, reps);
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		break;
	      case 1:
		B1;		/* BARRIER 1 */
		B2;		/* BARRIER 2 */
		sum += swap(cache_line, reps);
		break;
	      case 2:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually(cache_line, reps);
		B2;		/* BARRIER 2 */
		break;
	      default:
		B1;		/* BARRIER 1 */
		sum += load_0_eventually_no_pf(cache_line);
		B2;			/* BARRIER 2 */
		break;
	      }
	    break;
	  }
	case CAS_CONCURRENT: /* 24 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
	      case 1:
		sum += cas(cache_line, reps);
		break;
	      default:
		sum += cas_no_pf(cache_line, reps);
		break;
	      }
	    break;
	  }
	case FAI_ON_INVALID:	/* 25 */
	  {
	    switch (task->id % 3)
	      {
	      case 0:
		B1;		/* BARRIER 1 */
		sum += fai(cache_line, reps);
		break;
	      case 1:
		invalidate(cache_line, 0, reps);
		B1;		/* BARRIER 1 */
		break;
	      default:
		B1;		/* BARRIER 1 */
		break;
	      }

	    if (!test_flush)
	      {
		cache_line += test_stride;
	      }
	    break;
	  }
	case LOAD_FROM_L1:	/* 26 */
	  {
	    if (task->id == 0)
	      {
		sum += load_0(cache_line, reps);
		sum += load_0(cache_line, reps);
		sum += load_0(cache_line, reps);
	      }
	    break;
	  }
	case LOAD_FROM_MEM_SIZE: /* 27 */
	  {
	    sum += load_next(cl, reps);
	  }
	  break;
	case LFENCE:		/* 28 */
	    PFDI(0);
	    _mm_lfence();
	    PFDO(0, reps);
	  break;
	case SFENCE:		/* 29 */
	    PFDI(0);
	    _mm_sfence();
	    PFDO(0, reps);
	  break;
	case MFENCE:		/* 30 */
	    PFDI(0);
	    _mm_mfence();
	    PFDO(0, reps);
	  break;
	case PAUSE:		/* 31 */
	    PFDI(0);
	    _mm_pause();
	    PFDO(0, reps);
	  break;
	case NOP:		/* 32 */
	    PFDI(0);
	    asm volatile ("nop");
	    PFDO(0, reps);
	  break;
	case PROFILER:		/* 30 */
	default:
	  PFDI(0);
	  asm volatile ("");
	  PFDO(0, reps);
	  break;
	}
      if(test_test != CAS &&
         test_test != FAI &&
         test_test != TAS &&
         test_test != SWAP) {
          B3;			/* BARRIER 3 */
      }
    }


  task->operation_executed = reps;
  pid_t tid = gettid();
  pid_t pid = getpid();
  char path[256];
  char buffer[1024] = { 0 };
  snprintf(path, 256, "/proc/%d/task/%d/schedstat", pid, tid);
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
      perror("open");
      exit(-1);
  }
  if (read(fd, buffer, 1024) <= 0) {
      perror("read");
      exit(-1);
  }

  printf("id %02d "
          "operation_executed %llu "
          "schedstat %s",
          task->id,
          task->operation_executed,
          buffer);
  return 0;
}



int
main(int argc, char **argv) 
{

  /* before doing any allocations */
#if defined(__tile__)
  if (tmc_cpus_get_my_affinity(&cpus) != 0)
    {
      tmc_task_die("Failure in 'tmc_cpus_get_my_affinity()'.");
    }
#endif

#if defined(XEON)
  set_cpu(1);
#else
  set_cpu(0);
#endif

  struct option long_options[] = 
    {
      // These options don't set a flag
      {"help",                      no_argument,       NULL, 'h'},
      {"cores",                     required_argument, NULL, 'c'},
      {"repetitions",               required_argument, NULL, 'r'},
      {"test",                      required_argument, NULL, 't'},
      {"core1",                     required_argument, NULL, 'x'},
      {"core2",                     required_argument, NULL, 'y'},
      {"core3",                     required_argument, NULL, 'z'},
      {"core-others",               required_argument, NULL, 'o'},
      {"stride",                    required_argument, NULL, 's'},
      {"fence",                     required_argument, NULL, 'e'},
      {"mem-size",                  required_argument, NULL, 'm'},
      {"flush",                     no_argument,       NULL, 'f'},
      {"success",                   no_argument,       NULL, 'u'},
      {"verbose",                   no_argument,       NULL, 'v'},
      {"print",                     required_argument, NULL, 'p'},
      {"duration",                  required_argument, NULL, 'a'},
      {"threads",                   required_argument, NULL, 'b'},
      {NULL, 0, NULL, 0}
    };

  int i;
  char c;
  while(1) 
    {
      i = 0;
      c = getopt_long(argc, argv, "hc:r:t:x:m:y:z:o:e:fvup:s:a:b:d:", long_options, &i);

      if(c == -1)
	break;

      if(c == 0 && long_options[i].flag == 0)
	c = long_options[i].val;

      switch(c) 
	{
	case 0:
	  /* Flag is automatically set */
	  break;
	case 'h':
	  printf("ccbench  Copyright (C) 2013  Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>\n"
		 "This program comes with ABSOLUTELY NO WARRANTY.\n"
		 "This is free software, and you are welcome to redistribute it under certain conditions.\n\n"
		 "ccbecnh is an application for measuring the cache-coherence latencies, i.e., the latencies of\n"
		 "of loads, stores, CAS, FAI, TAS, and SWAP\n"
		 "\n"
		 "Usage:\n"
		 "  ./ccbench [options...]\n"
		 "\n"
		 "Options:\n"
		 "  -h, --help\n"
		 "        Print this message\n"
		 "  -c, --cores <int>\n"
		 "        Number of cores to run the test on (default=" XSTR(DEFAULT_CORES) ")\n"
		 "  -r, --repetitions <int>\n"
		 "        Repetitions of the test case (default=" XSTR(DEFAULT_REPS) ")\n"
		 "  -t, --test <int>\n"
		 "        Test case to run (default=" XSTR(DEFAULT_TEST) "). See below for supported events\n"
		 "  -x, --core1 <int>\n"
		 "        1st core to use (default=" XSTR(DEFAULT_CORE1) ")\n"
		 "  -y, --core2 <int>\n"
		 "        2nd core to use (default=" XSTR(DEFAULT_CORE2) ")\n"
		 "  -z, --core3 <int>\n"
		 "        3rd core to use. Some (most) tests use only 2 cores (default=" XSTR(DEFAULT_CORE3) ")\n"
		 "  -o, --core-others <int>\n"
		 "        Offset for core that the processes with ID > 3 should bind (default=" XSTR(DEFAULT_CORE_OTHERS) ")\n"
		 "  -f, --flush\n"
		 "        Perform a cache line flush before the test (default=" XSTR(DEFAULT_FLUSH) ")\n"
		 "  -s, --stride <int>\n"
		 "        What stride size to use when accessing the cache line(s) (default=" XSTR(DEFAULT_STRIDE) ")\n"
		 "        The application draws a random number X in the [0..(stride-1)] range and applies the target\n"
		 "        operation on this random cache line. The operation is completed when X=0. The stride is used\n"
		 "        in order to fool the hardware prefetchers that could hide the latency we want to measure.\n"
		 "  -e, --fence <int>\n"
		 "        What memory barrier (fence) lvl to use (default=" XSTR(DEFAULT_FENCE) ")\n"
		 "        0 = no fences / 1 = load-store fences / 2 = full fences / 3 = load-none fences / 4 = none-store fences\n"
		 "        5 = full-none fences / 6 = none-full fences / 7 = full-store fences / 8 = load-full fences \n"
		 "  -m, --mem-size <int>\n"
		 "        What memory size to use (in cache lines) (default=" XSTR(CACHE_LINE_NUM) ")\n"
		 "  -u, --success\n"
		 "        Make all atomic operations be successfull (e.g, TAS_ON_SHARED)\n"
		 "  -v, --verbose\n"
		 "        Verbose printing of results (default=" XSTR(DEFAULT_VERBOSE) ")\n"
		 "  -p, --print <int>\n"
		 "        If verbose, how many results to print (default=" XSTR(DEFAULT_PRINT) ")\n"
		 "  -a, --duration <int>\n"
		 "        runtime of the test(in seconds) (default=" XSTR(DEFAULT_DURATION) ")\n"
		 "  -b, --threads <int>\n"
		 "        number of threads to create (default=" XSTR(DEFAULT_THREADS) ")\n"
		 "  -d, --placement <int>\n"
		 "        how to place threads (default=" XSTR(DEFAULT_PLACEMENT) ")\n"
                 );
	  printf("Supported events: \n");
	  int ar;
	  for (ar = 0; ar < NUM_EVENTS; ar++)
	    {
	      printf("      %2d - %s\n", ar, moesi_type_des[ar]);
	    }

	  exit(0);
	case 'c':
	  test_cores = atoi(optarg);
	  break;
	case 'r':
	  test_reps = atoi(optarg);
	  break;
	case 't':
	  test_test = atoi(optarg);
	  break;
	case 'x':
	  test_core1 = atoi(optarg);
	  break;
	case 'y':
	  test_core2 = atoi(optarg);
	  break;
	case 'z':
	  test_core3 = atoi(optarg);
	  break;
	case 'o':
	  test_core_others = atoi(optarg);
	  break;
	case 'f':
	  test_flush = 1;
	  break;
	case 's':
	  test_stride = pow2roundup(atoi(optarg));
	  break;
	case 'e':
	  test_fence = atoi(optarg);
	  break;
	case 'm':
	  test_mem_size = parse_size(optarg);
	  printf("Data size : %zu KiB\n", test_mem_size / 1024);
	  break;
	case 'u':
	  test_ao_success = 1;
	  break;
	case 'v':
	  test_verbose = 1;
	  break;
	case 'p':
	  test_verbose = 1;
	  test_print = atoi(optarg);
	  break;
	case 'a':
	  test_duration = atoi(optarg);
	  break;
	case 'b':
	  test_threads = atoi(optarg);
	  break;
	case '?':
	  printf("Use -h or --help for help\n");
	  exit(0);
        case 'd':
          test_placement = atoi(optarg);
          break;
	default:
	  exit(1);
	}
    }


  test_cache_line_num = test_mem_size / sizeof(cache_line_t);

  if ((test_test == STORE_ON_EXCLUSIVE || test_test == STORE_ON_INVALID || test_test == LOAD_FROM_INVALID
       || test_test == LOAD_FROM_EXCLUSIVE || test_test == LOAD_FROM_SHARED) && !test_flush)
    {
      assert((test_reps * test_stride) <= test_cache_line_num);
    }

  if (test_test != LOAD_FROM_MEM_SIZE)
    {
      assert(test_stride < test_cache_line_num);
    }


  ID = 0;
  printf("test: %20s  / #cores: %d / #repetitions: %d / stride: %d (%u kiB)", moesi_type_des[test_test], 
	 test_cores, test_reps, test_stride, (64 * test_stride) / 1024);
  if (test_flush)
    {
      printf(" / flush");
    }

  printf("  / fence: ");

  switch (test_fence)
    {
    case 1:
      printf(" load & store");
      test_lfence = test_sfence = 1;
      break;
    case 2:
      printf(" full");
      test_lfence = test_sfence = 2;
      break;
    case 3:
      printf(" load");
      test_lfence = 1;
      test_sfence = 0;
      break;
    case 4:
      printf(" store");
      test_lfence = 0;
      test_sfence = 1;
      break;
    case 5:
      printf(" full/none");
      test_lfence = 2;
      test_sfence = 0;
      break;
    case 6:
      printf(" none/full");
      test_lfence = 0;
      test_sfence = 2;
      break;
    case 7:
      printf(" full/store");
      test_lfence = 2;
      test_sfence = 1;
      break;
    case 8:
      printf(" load/full");
      test_lfence = 1;
      test_sfence = 2;
      break;    
    case 9:
      printf(" double write");
      test_lfence = 0;
      test_sfence = 3;
      break;
    default:
      printf(" none");
      test_lfence = test_sfence = 0;
      break;
    }

  printf("\n");


  barriers_init(test_cores);
  seeds = seed_rand();

  cache_line = cache_line_open();
#if defined(__tile__)
  tmc_cmem_init(0);		/*   initialize shared memory */
#endif  /* TILERA */

  int stop __attribute__((aligned (64))) = 0;
  ull total_executions = 0;
  task_t *tasks = malloc(sizeof(task_t) * test_threads);
  for (int i = 0; i < test_threads; i++) {
        tasks[i].stop = &stop;

        tasks[i].ncpu = test_cores;
        tasks[i].id = i;

        tasks[i].operation_executed = 0;
        //printf("Task id = %d, priority = %d, ncpu = %d, atomics_executed = %llu\n", tasks[i].id, tasks[i].priority, tasks[i].ncpu, tasks[i].atomics_executed);
  }
  if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Fail to initialize the lock\n");
        return 1;
  }  
  for (int i = 0; i < test_threads; i++) {
        pthread_create(&tasks[i].thread, NULL, run_test, &tasks[i]);
  }
  sleep(test_duration);
  stop = 1;

  for (int i = 0; i < test_threads; i++) {
      pthread_join(tasks[i].thread, NULL);
      total_executions = total_executions + tasks[i].operation_executed;
  }

  printf("Total Exeuctions = %llu\n", total_executions);
  printf("Average atomic execution time(ns) = %f\n", (1000.0 * 1000 * 1000 * test_duration) / total_executions);
  printf("Per thread execution average = %f\n", (1.0 * total_executions)/test_threads);
  
  cache_line_close(ID, "cache_line");
  barriers_term(ID);
  return 0;

}

uint32_t
cas(volatile cache_line_t* cl, volatile uint64_t reps)
{
  uint8_t o = reps & 0x1;
  uint8_t no = !o; 
  volatile uint32_t r;

  PFDI(0);
  r = CAS_U32(cl->word, o, no);
  PFDO(0, reps);

  return (r == o);
}

uint32_t
cas_no_pf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  uint8_t o = reps & 0x1;
  uint8_t no = !o; 
  volatile uint32_t r;
  r = CAS_U32(cl->word, o, no);

  return (r == o);
}

uint32_t
cas_0_eventually(volatile cache_line_t* cl, volatile uint64_t reps)
{
  uint8_t o = reps & 0x1;
  uint8_t no = !o; 
  volatile uint32_t r;

  uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile cache_line_t* cl1 = cl + cln;
      PFDI(0);
      r = CAS_U32(cl1->word, o, no);
      PFDO(0, reps);
    }
  while (cln > 0);

  return (r == o);
}

uint32_t
fai(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t t = 0;

  uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile cache_line_t* cl1 = cl + cln;
      PFDI(0);
      t = FAI_U32(cl1->word);
      PFDO(0, reps);
    }
  while (cln > 0);

  return t;
}

uint8_t
tas(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint8_t r;

  uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile cache_line_t* cl1 = cl + cln;
#if defined(TILERA)
      volatile uint32_t* b = (volatile uint32_t*) cl1->word;
#else
      volatile uint8_t* b = (volatile uint8_t*) cl1->word;
#endif

      PFDI(0);
      r = TAS_U8(b);
      PFDO(0, reps);
    }
  while (cln > 0);

  return (r != 255);
}

uint32_t
swap(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t res;

  uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile cache_line_t* cl1 = cl + cln;
      PFDI(0);
      res = SWAP_U32(cl1->word, ID);
      PFDO(0, reps);
    }
  while (cln > 0);

  _mm_mfence();
  return res;
}

void
store_0(volatile cache_line_t* cl, volatile uint64_t reps)
{
  if (test_sfence == 0)
    {
      PFDI(0);
      cl->word[0] = reps;
      PFDO(0, reps);
    }
  else if (test_sfence == 1)
    {
      PFDI(0);
      cl->word[0] = reps;
      _mm_sfence();
      PFDO(0, reps);
    }
  else if (test_sfence == 2)
    {
      PFDI(0);
      cl->word[0] = reps;
      _mm_mfence();
      PFDO(0, reps);
    }
}

void
store_0_no_pf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  cl->word[0] = reps;
  if (test_sfence == 1)
    {
      _mm_sfence();
    }
  else if (test_sfence == 2)
    {
      _mm_mfence();
    }
}

static void
store_0_eventually_sf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(0);
      w[0] = cln;
      _mm_sfence();
      PFDO(0, reps);
    }
  while (cln > 0);
}

static void
store_0_eventually_mf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(0);
      w[0] = cln;
      _mm_mfence();
      PFDO(0, reps);
    }
  while (cln > 0);
}

static void
store_0_eventually_nf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(0);
      w[0] = cln;
      PFDO(0, reps);
    }
  while (cln > 0);
}

static void
store_0_eventually_dw(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(0);
      w[0] = cln;
      w[16] = cln;
      PFDO(0, reps);
    }
  while (cln > 0);
}

void
store_0_eventually(volatile cache_line_t* cl, volatile uint64_t reps)
{
  if (test_sfence == 0)
    {
      store_0_eventually_nf(cl, reps);
    }
  else if (test_sfence == 1)
    {
      store_0_eventually_sf(cl, reps);
    }
  else if (test_sfence == 2)
    {
      store_0_eventually_mf(cl, reps);
    }
  else if (test_sfence == 3)
    {
      store_0_eventually_dw(cl, reps);
    }
  /* _mm_mfence(); */
}


static void
store_0_eventually_pfd1_sf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(1);
      w[0] = cln;
      _mm_sfence();
      PFDO(1, reps);
    }
  while (cln > 0);
}

static void
store_0_eventually_pfd1_mf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(1);
      w[0] = cln;
      _mm_mfence();
      PFDO(1, reps);
    }
  while (cln > 0);
}

static void
store_0_eventually_pfd1_nf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      PFDI(1);
      w[0] = cln;
      PFDO(1, reps);
    }
  while (cln > 0);
}

void
store_0_eventually_pfd1(volatile cache_line_t* cl, volatile uint64_t reps)
{
  if (test_sfence == 0)
    {
      store_0_eventually_pfd1_nf(cl, reps);
    }
  else if (test_sfence == 1)
    {
      store_0_eventually_pfd1_sf(cl, reps);
    }
  else if (test_sfence == 2)
    {
      store_0_eventually_pfd1_mf(cl, reps);
    }
  /* _mm_mfence(); */
}

static uint64_t
load_0_eventually_lf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  volatile uint64_t val = 0;

  do
    {
      cln = clrand();
      volatile uint32_t* w = &cl[cln].word[0];
      PFDI(0);
      val = w[0];
      _mm_lfence();
      PFDO(0, reps);
    }
  while (cln > 0);
  return val;
}

static uint64_t
load_0_eventually_mf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  volatile uint64_t val = 0;

  do
    {
      cln = clrand();
      volatile uint32_t* w = &cl[cln].word[0];
      PFDI(0);
      val = w[0];
      _mm_mfence();
      PFDO(0, reps);
    }
  while (cln > 0);
  return val;
}

static uint64_t
load_0_eventually_nf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t cln = 0;
  volatile uint64_t val = 0;

  do
    {
      cln = clrand();
      volatile uint32_t* w = &cl[cln].word[0];
      PFDI(0);
      val = w[0];
      PFDO(0, reps);
    }
  while (cln > 0);
  return val;
}


uint64_t
load_0_eventually(volatile cache_line_t* cl, volatile uint64_t reps)
{
  uint64_t val = 0;
  if (test_lfence == 0)
    {
      val = load_0_eventually_nf(cl, reps);
    }
  else if (test_lfence == 1)
    {
      val = load_0_eventually_lf(cl, reps);
    }
  else if (test_lfence == 2)
    {
      val = load_0_eventually_mf(cl, reps);
    }
  _mm_mfence();
  return val;
}

uint64_t
load_0_eventually_no_pf(volatile cache_line_t* cl)
{
  uint32_t cln = 0;
  uint64_t sum = 0;
  do
    {
      cln = clrand();
      volatile uint32_t *w = &cl[cln].word[0];
      sum = w[0];
    }
  while (cln > 0);

  _mm_mfence();
  return sum;
}

static uint64_t
load_0_lf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t val = 0;
  volatile uint32_t* p = (volatile uint32_t*) &cl->word[0];
  PFDI(0);
  val = p[0];
  _mm_lfence();
  PFDO(0, reps);
  return val;
}

static uint64_t
load_0_mf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t val = 0;
  volatile uint32_t* p = (volatile uint32_t*) &cl->word[0];
  PFDI(0);
  val = p[0];
  _mm_mfence();
  PFDO(0, reps);
  return val;
}

static uint64_t
load_0_nf(volatile cache_line_t* cl, volatile uint64_t reps)
{
  volatile uint32_t val = 0;
  volatile uint32_t* p = (volatile uint32_t*) &cl->word[0];
  PFDI(0);
  val = p[0];
  PFDO(0, reps);
  return val;
}


uint64_t
load_0(volatile cache_line_t* cl, volatile uint64_t reps)
{
  uint64_t val = 0;
  if (test_lfence == 0)
    {
      val = load_0_nf(cl, reps);
    }
  else if (test_lfence == 1)
    {
      val = load_0_lf(cl, reps);
    }
  else if (test_lfence == 2)
    {
      val = load_0_mf(cl, reps);
    }
  _mm_mfence();
  return val;
}

static uint64_t
load_next_lf(volatile uint64_t* cl, volatile uint64_t reps)
{
  const size_t do_reps = test_cache_line_num;
  PFDI(0);
  int i;
  for (i = 0; i < do_reps; i++)
    {
      cl = (uint64_t*) *cl;
      _mm_lfence();
    }
  PFDOR(0, reps, do_reps);
  return *cl;

}

static uint64_t
load_next_mf(volatile uint64_t* cl, volatile uint64_t reps)
{
  const size_t do_reps = test_cache_line_num;
  PFDI(0);
  int i;
  for (i = 0; i < do_reps; i++)
    {
      cl = (uint64_t*) *cl;
      _mm_mfence();
    }
  PFDOR(0, reps, do_reps);
  return *cl;

}

static uint64_t
load_next_nf(volatile uint64_t* cl, volatile uint64_t reps)
{
  const size_t do_reps = test_cache_line_num;
  PFDI(0);
  int i;
  for (i = 0; i < do_reps; i++)
    {
      cl = (uint64_t*) *cl;
    }
  PFDOR(0, reps, do_reps);
  return *cl;
}

uint64_t
load_next(volatile uint64_t* cl, volatile uint64_t reps)
{
  uint64_t val = 0;
  if (test_lfence == 0)
    {
      val = load_next_nf(cl, reps);
    }
  else if (test_lfence == 1)
    {
      val = load_next_lf(cl, reps);
    }
  else if (test_lfence == 2)
    {
      val = load_next_mf(cl, reps);
    }
  return val;
}

void
invalidate(volatile cache_line_t* cl, uint64_t index, volatile uint64_t reps)
{
  PFDI(0);
  _mm_clflush((void*) (cl + index));
  PFDO(0, reps);
  _mm_mfence();
}

static size_t
parse_size(char* optarg)
{
  size_t test_mem_size_multi = 1;
  char multi = optarg[strlen(optarg) - 1];
  if (multi == 'b' || multi == 'B')
    {
      optarg[strlen(optarg) - 1] = optarg[strlen(optarg)];
      multi = optarg[strlen(optarg) - 1];
    }

  if (multi == 'k' || multi == 'K')
    {
      test_mem_size_multi = 1024;
      optarg[strlen(optarg) - 1] = optarg[strlen(optarg)];
    }
  else if (multi == 'm' || multi == 'M')
    {
      test_mem_size_multi = 1024 * 1024LL;
      optarg[strlen(optarg) - 1] = optarg[strlen(optarg)];
    }
  else if (multi == 'g' || multi == 'G')
    {
      test_mem_size_multi = 1024 * 1024 * 1024LL;
      optarg[strlen(optarg) - 1] = optarg[strlen(optarg)];
    }

  return test_mem_size_multi * atoi(optarg);
}

volatile cache_line_t* 
cache_line_open()
{
  uint64_t size = test_cache_line_num * sizeof(cache_line_t);

#if defined(__tile__)
  tmc_alloc_t alloc = TMC_ALLOC_INIT;
  tmc_alloc_set_shared(&alloc);
  /*   tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_HASH); */
  /*   tmc_alloc_set_home(&alloc, MAP_CACHE_NO_LOCAL); */
  tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_HERE);
  /*   tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_TASK); */
  
  volatile cache_line_t* cache_line = (volatile cache_line_t*) tmc_alloc_map(&alloc, size);
  if (cache_line == NULL)
    {
      tmc_task_die("Failed to allocate memory.");
    }

  tmc_cmem_init(0);		/*   initialize shared memory */


  cache_line->word[0] = 0;

#else	 /* !__tile__ ****************************************************************************************/
  char keyF[100];
  sprintf(keyF, CACHE_LINE_MEM_FILE);

  int ssmpfd = shm_open(keyF, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
  if (ssmpfd < 0) 
    {
      if (errno != EEXIST) 
	{
	  perror("In shm_open");
	  exit(1);
	}


      ssmpfd = shm_open(keyF, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
      if (ssmpfd < 0) 
	{
	  perror("In shm_open");
	  exit(1);
	}
    }
  else {
    //    P("%s newly openned", keyF);
    if (ftruncate(ssmpfd, size) < 0) {
      perror("ftruncate failed\n");
      exit(1);
    }
  }

  volatile cache_line_t* cache_line = 
    (volatile cache_line_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, ssmpfd, 0);
  if (cache_line == NULL)
    {
      perror("cache_line = NULL\n");
      exit(134);
    }

#endif  /* __tile ********************************************************************************************/
  memset((void*) cache_line, '1', size);

  if (ID == 0)
    {
      uint32_t cl;
      for (cl = 0; cl < test_cache_line_num; cl++)
	{
	  cache_line[cl].word[0] = 0;
	  _mm_clflush((void*) (cache_line + cl));
	}

      if (test_test == LOAD_FROM_MEM_SIZE)
	{
	  create_rand_list_cl((volatile uint64_t*) cache_line, test_mem_size / sizeof(uint64_t));
	}


    }

  _mm_mfence();
  return cache_line;
}

static void
create_rand_list_cl(volatile uint64_t* list, size_t n)
{
  size_t per_cl = sizeof(cache_line_t) / sizeof(uint64_t);
  n /= per_cl;

  unsigned long* s = seed_rand();
  s[0] = 0xB9E4E2F1F1E2E3D5L;
  s[1] = 0xF1E2E3D5B9E4E2F1L;
  s[2] = 0x9B3A0FA212342345L;

  uint8_t* used = calloc(n * per_cl, sizeof(uint8_t));
  assert (used != NULL);

  size_t idx = 0;
  size_t used_num = 0;
  while (used_num < n - 1)
    {
      used[idx] = 1;
      used_num++;
      
      size_t nxt;
      do 
	{
	  nxt = (my_random(s, s+1, s+2) % n) * per_cl;
	}
      while (used[nxt]);

      list[idx] = (uint64_t) (list + nxt);
      idx = nxt;
    }
  list[idx] = (uint64_t) (list); /* close the loop! */

  free(s);
  free(used);
} 

void
cache_line_close(const uint32_t id, const char* name)
{
#if !defined(__tile__)
  if (id == 0)
    {
      char keyF[100];
      sprintf(keyF, CACHE_LINE_MEM_FILE);
      shm_unlink(keyF);
    }
#else
  tmc_cmem_close();
#endif
}

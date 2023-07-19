// File:	worker_t.h

// List all group member's name: Atharva Patil
// username of iLab: anp166
// iLab Server: Cheese

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1
#define VALUE 10240
#define QUANTUM 1
#define LEVELS 4

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>
#include <stdint.h>

typedef uint worker_t;

typedef enum{
	RUNNABLE, RUNNING, BLOCKED, DEAD
} thread_state;

typedef struct node node;
typedef struct queue queue;
typedef struct TCB tcb;

typedef struct node{
	struct TCB *block;
	struct node *next;
}node;

typedef struct queue{
	struct node *front;
	struct node *end;
}queue;

typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE

	worker_t tid;
	thread_state status;
	ucontext_t context;
	
	int elapsed;
	worker_t waiting_on_thread;
	double entry_time;
	bool scheduled;
	
}tcb; 


/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */

	// YOUR CODE HERE
	node *block_list;
	int lock;
}worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE

// linked list structure holding tcbs in ascending order of time quantums



/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);

static void schedule();
static void sched_psjf();
static void sched_mlfq();
tcb * get_next_tcb_LL_pjsf();
void add_to_tcb_LL_pjsf(tcb *thread_control_block);
void add_to_queue_mlfq(tcb *block,int level);
tcb * remove_from_queue_mlfq(int level);
void unblock_threads(worker_t tid);
bool is_in_run_queue(tcb * block);



/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif

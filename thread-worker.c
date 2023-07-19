// File:	thread-worker.c

// List all group member's name: Atharva Patil
// username of iLab: anp166
// iLab Server: Cheese

#include "thread-worker.h"


//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;


// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
int thread_id_counter = 0;
bool is_scheduler_context = true;
bool is_main_context = false;
node *list_of_tcbs_pjsf = NULL;
queue *mlfq;
tcb *current_thread = NULL;
tcb *main_block = NULL;
bool is_pjsf = true;
struct sigaction sa;
struct itimerval timer;	
ucontext_t scheduler_context;
bool is_first = true;
int threads_count = 1;
int threads_count_response = 1;
double turn_time = 0;
double response_time = 0;
struct timespec start, end;


/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

    // - create Thread Control Block (TCB)
    // - create and initialize the context of this worker thread
    // - allocate space of stack for this thread to run
    // after everything is set, push this thread into run queue and 
    // - make it ready for the execution.

    // YOUR CODE HERE

	if(is_first){

        clock_gettime(CLOCK_REALTIME, &start);
	
		// create the thread control block
		tcb *thread_control_block = malloc(sizeof(tcb));
		thread_control_block->tid = thread_id_counter;
		thread_id_counter++;
		thread_control_block->status = RUNNABLE;
		thread_control_block->elapsed = 0;
		thread_control_block->waiting_on_thread = -1;
		thread_control_block->scheduled = false;
		clock_gettime(CLOCK_REALTIME, &end);
		thread_control_block->entry_time = ((end.tv_sec - 0) * 1000 + (end.tv_nsec - 0) / 1000000);
		current_thread = thread_control_block;
		main_block = thread_control_block;
		getcontext(&(thread_control_block->context));
	
		// malloc for mlfq
		//mlfq = malloc(LEVELS * sizeof(struct queue *));

		getcontext(&scheduler_context);
		scheduler_context.uc_link = NULL;
		scheduler_context.uc_stack.ss_sp = malloc(VALUE);
		scheduler_context.uc_stack.ss_size = VALUE;

		makecontext(&scheduler_context,(void(*)(void)) schedule, 1, arg);
	
		// create signal to call scheduler
		memset(&sa, 0, sizeof(struct sigaction));
		sa.sa_handler = schedule;
		sigaction(SIGPROF, &sa, NULL);

		// create the timer
		timer.it_interval.tv_usec = 10000; 
		timer.it_interval.tv_sec = 0;
		timer.it_value.tv_usec = 10000;
		timer.it_value.tv_sec = 0;
		
		setitimer(ITIMER_PROF, &timer, NULL);
		if(!is_in_run_queue(thread_control_block))
			add_to_tcb_LL_pjsf(thread_control_block);
		
		node * ptr = list_of_tcbs_pjsf;
		
	}
	is_first = false;


	// create the thread control block
	tcb *thread_control_block = malloc(sizeof(tcb));
	thread_control_block->tid = thread_id_counter;
	thread_id_counter++;

	thread_control_block->status = RUNNABLE;
	thread_control_block->elapsed = 0;
	thread_control_block->waiting_on_thread = -1;
	thread_control_block->scheduled = false;
	clock_gettime(CLOCK_REALTIME, &end);
	thread_control_block->entry_time = ((end.tv_sec - 0) * 1000 + (end.tv_nsec - 0) / 1000000);

	// setup context for thread
	getcontext(&(thread_control_block->context));
	thread_control_block->context.uc_link = NULL;
	thread_control_block->context.uc_stack.ss_sp = malloc(VALUE);
	thread_control_block->context.uc_stack.ss_size = VALUE;
	makecontext(&(thread_control_block->context), (void(*)(void))function, 1, arg);


	
	add_to_tcb_LL_pjsf(thread_control_block);

	node * ptr = list_of_tcbs_pjsf;



    return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	current_thread->status = RUNNABLE;
	
	swapcontext(&(current_thread->context), &scheduler_context);
	
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread
	
	// YOUR CODE HERE
	current_thread->status = DEAD;
	node *ptr = list_of_tcbs_pjsf;
	int thread_id = current_thread->tid;

	free(current_thread);

	clock_gettime(CLOCK_REALTIME, &end);
	double entry = current_thread->entry_time;
	double temp = ((end.tv_sec - 0) * 1000 + (end.tv_nsec - 0) / 1000000) - entry;
	turn_time += temp;
	avg_turn_time = turn_time / threads_count;
	threads_count++;
	current_thread = NULL;
	free(current_thread);
	unblock_threads(thread_id);
	
	schedule();
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	node *ptr = list_of_tcbs_pjsf;
	node *prev = NULL;
	while(ptr != NULL){
		if(ptr->block->tid == thread){
			tcb *thread_block = ptr->block;
			if(thread_block->status == DEAD){
				prev = ptr->next;
				free(thread_block);
				free(ptr);
				return 0;
			}
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}

	current_thread->status = BLOCKED;
	current_thread->waiting_on_thread = thread;

	schedule();
	return 0;

};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	mutex->block_list = NULL;
	mutex->lock = 0;
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

		while(__sync_lock_test_and_set(&(mutex->lock), __ATOMIC_SEQ_CST)){
			current_thread->status = BLOCKED;
			node *waiting_thread = malloc(sizeof(node));
			waiting_thread->block = current_thread;
			waiting_thread->next = mutex->block_list;
			mutex->block_list = waiting_thread;
			
			swapcontext(&(current_thread->context), &scheduler_context);
			
		}

        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	if(mutex->block_list == NULL){
		mutex->lock = 0;
		return 0;
	}
	node *ptr = mutex->block_list;
	node *prev = ptr;

	while(ptr != NULL){
	
		if(ptr->block != NULL)
			ptr->block->status = RUNNABLE;
	
		ptr = ptr->next;

		free(prev);
	
		prev = ptr;
		
	}


	mutex->lock = 0;
	mutex->block_list = NULL;
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	worker_mutex_unlock(mutex);

	return 0;
};

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

	tot_cntx_switches++;
// - schedule policy
#ifndef MLFQ
	// Choose PSJF
	sched_psjf();
#else 
	// Choose MLFQ
	sched_mlfq();
#endif

return;

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	getcontext(&scheduler_context);

	tcb *previous_thread = current_thread;
	tcb *next_thread = get_next_tcb_LL_pjsf();
	if(current_thread != NULL){
		if(next_thread != NULL){
			previous_thread->elapsed += 1;
			previous_thread->status = RUNNABLE;
			current_thread = next_thread;
			if(!is_in_run_queue(previous_thread))
				add_to_tcb_LL_pjsf(previous_thread);
			struct sigaction sa;
			memset(&sa, 0, sizeof(struct sigaction));
			sa.sa_handler = &schedule;
			sigaction(SIGPROF, &sa, NULL);

			// create the timer
			struct itimerval timer;
			timer.it_interval.tv_usec = QUANTUM; 
			timer.it_interval.tv_sec = 0;
			timer.it_value.tv_usec = QUANTUM;
			timer.it_value.tv_sec = 0;
			setitimer(ITIMER_PROF, &timer, NULL);

			next_thread->status = RUNNING;
			tot_cntx_switches += 1;

			if(next_thread->scheduled == false){
					next_thread->scheduled = true;
					clock_gettime(CLOCK_REALTIME, &end);
					double entry = next_thread->entry_time;
					double temp = ((end.tv_sec - 0) * 1000 + (end.tv_nsec - 0) / 1000000) - entry;
					response_time += temp;
					avg_resp_time = response_time / threads_count_response;
					threads_count_response++;
					
			}

			swapcontext(&(scheduler_context), &(next_thread->context));

		}
		else{
			previous_thread->elapsed += 1;

			struct sigaction sa;
			memset(&sa, 0, sizeof(struct sigaction));
			sa.sa_handler = &schedule;
			sigaction(SIGPROF, &sa, NULL);

		
			// create the timer
			struct itimerval timer;
			timer.it_interval.tv_usec = QUANTUM; 
			timer.it_interval.tv_sec = 0;
			timer.it_value.tv_usec = QUANTUM;
			timer.it_value.tv_sec = 0;
			setitimer(ITIMER_PROF, &timer, NULL);
			tot_cntx_switches++;
			swapcontext(&(scheduler_context), &(previous_thread->context));
		}
		
	}
	else{
		if(next_thread != NULL){

			current_thread = next_thread;

			struct sigaction sa;
			memset(&sa, 0, sizeof(struct sigaction));
			sa.sa_handler = &schedule;
			sigaction(SIGPROF, &sa, NULL);
			
			// create the timer
			struct itimerval timer;
			timer.it_interval.tv_usec = QUANTUM; 
			timer.it_interval.tv_sec = 0;
			timer.it_value.tv_usec = QUANTUM;
			timer.it_value.tv_sec = 0;
			setitimer(ITIMER_PROF, &timer, NULL);

			next_thread->status = RUNNING;
			tot_cntx_switches += 1;

			if(next_thread->scheduled == false){
					next_thread->scheduled = true;
					clock_gettime(CLOCK_REALTIME, &end);
					double entry = next_thread->entry_time;
					double temp = ((end.tv_sec - 0) * 1000 + (end.tv_nsec - 0) / 1000000) - entry;
					response_time += temp;
					avg_resp_time = response_time / threads_count_response;
					threads_count_response++;
					
			}
			swapcontext(&scheduler_context, &(next_thread->context));

		}
		else{

			current_thread = NULL;
			struct sigaction sa;
			memset(&sa, 0, sizeof(struct sigaction));
			sa.sa_handler = &schedule;
			sigaction(SIGPROF, &sa, NULL);

			// create the timer
			struct itimerval timer;
			timer.it_interval.tv_usec = QUANTUM; 
			timer.it_interval.tv_sec = 0;
			timer.it_value.tv_usec = QUANTUM;
			timer.it_value.tv_sec = 0;
			setitimer(ITIMER_PROF, &timer, NULL);
			tot_cntx_switches += 1;
			setcontext(&scheduler_context);

		}
	}
	
	return;
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	return;
}

//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

// psjf Linked List add
void add_to_tcb_LL_pjsf(tcb *thread_control_block){
	node *node = malloc(sizeof(node));
	node->block = thread_control_block;

	// case 1, list is empty
	if(list_of_tcbs_pjsf == NULL){
		list_of_tcbs_pjsf = node;
		return;
	}

	// case 2, one item in list
	if(list_of_tcbs_pjsf->block->elapsed >= node->block->elapsed){
		node->next = list_of_tcbs_pjsf;
		list_of_tcbs_pjsf = node;
		return;
	}
	else if(list_of_tcbs_pjsf->block->elapsed <= node->block->elapsed && list_of_tcbs_pjsf->next == NULL){
		list_of_tcbs_pjsf->next = node;
		return;
	}

	// case 3, more than one item in list. walk list with two pointers
	struct node *ptr = list_of_tcbs_pjsf;
	struct node *prev = NULL;

	while(ptr != NULL){
		if(ptr->block->elapsed > node->block->elapsed){
			prev->next = node;
			node->next = ptr;
			return;
		}
		else{
			prev = ptr;
			ptr = ptr->next;
		}
	}

	prev->next = node;

	return;
}

tcb * get_next_tcb_LL_pjsf(){
	if(list_of_tcbs_pjsf == NULL) return NULL;

	node *ptr = list_of_tcbs_pjsf;
	node *prev = NULL;

	int count = 0;

	while(ptr != NULL){
		if(ptr->block->status == RUNNABLE){
			if(prev != NULL){
				prev->next = ptr->next;
				tcb *thread_control_block = ptr->block;
				free(ptr);
				return thread_control_block;
			}
			else{
				prev = ptr->next;
				list_of_tcbs_pjsf = prev;
				tcb *thread_control_block = ptr->block;
				free(ptr);
				return thread_control_block;

			}

			
		}
		prev = ptr;
		ptr = ptr->next;
		
	}

	return NULL;

}

void add_to_queue_mlfq(tcb *block,int level){
	if(mlfq[level].front == NULL && mlfq[level].end == NULL){
		node *node = malloc(sizeof(node));
		node->block = block;
		node->next = NULL;
		mlfq[level].front = node;
		mlfq[level].end = node;
		return;
	}

	node *node = malloc(sizeof(node));

	struct node *ptr = mlfq[level].front;
	mlfq[level].front = node;
	mlfq[level].front->next = ptr;
	

	return;

}

tcb * remove_from_queue_mlfq(int level){
	node *node = mlfq[level].end;
	tcb *block = mlfq[level].end->block;

	struct node *ptr = mlfq[level].front;
	struct node *prev = mlfq[level].front;

	while(ptr != node){
		prev = ptr;
		ptr = ptr->next;
	}

	mlfq[level].end = prev;
	free(node);
	return block;
}

void unblock_threads(worker_t tid){
	node *ptr = list_of_tcbs_pjsf;
	while(ptr != NULL){
		tcb *thread_block = ptr->block;
		if(thread_block->waiting_on_thread == tid){
			thread_block->status = RUNNABLE;
			thread_block->waiting_on_thread = -1;
		}
		ptr = ptr->next;
	}
}

bool is_in_run_queue(tcb * block){
	node *ptr = list_of_tcbs_pjsf;
	if(ptr == NULL) return false;
	while(ptr != NULL){
		tcb *block_in_list = ptr->block;
		if(block_in_list == block){
			return true;
		}

		ptr = ptr->next;
	}

	return false;
}

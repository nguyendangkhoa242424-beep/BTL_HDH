/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		slot[i] = MAX_PRIO - i; 
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	running_list.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

void finish_scheduler(void) {
#ifdef MLQ_SCHED
	int i;
	for (i = 0; i < MAX_PRIO; i++)
		mlq_ready_queue[i].size = 0;
#endif
	ready_queue.size   = 0;
	run_queue.size     = 0;
	running_list.size  = 0;
	pthread_mutex_destroy(&queue_lock);
}

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t *proc = NULL;
 
	/* Persistent state across calls */
	static int curr_prio = 0;
	static int curr_slot = MAX_PRIO;
 
	pthread_mutex_lock(&queue_lock);
	//curr_prio = 0;
	//curr_slot = 1;
 
	int checked = 0;
	while (checked < MAX_PRIO) {
		if (!empty(&mlq_ready_queue[curr_prio]) && curr_slot > 0) {
			/* Serve one process from the current priority queue */
			proc = dequeue(&mlq_ready_queue[curr_prio]);
			curr_slot--;
 
			/* If slots for this level are exhausted, advance now
			 * so the *next* call starts from the next priority. */
			if (curr_slot == 0) {
				curr_prio = (curr_prio + 1) % MAX_PRIO;
				curr_slot = MAX_PRIO - curr_prio;
			}
			break;
		} else {
			/* Current queue is empty or has no slots left –
			 * move to the next priority level. */
			curr_prio = (curr_prio + 1) % MAX_PRIO;
			curr_slot = MAX_PRIO - curr_prio;
			checked++;
		}
	}
 
	if (proc != NULL)
		enqueue(&running_list, proc);
 
	pthread_mutex_unlock(&queue_lock);
	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	/* FIX (Medium): validate priority before using it as array index */
	if (proc->prio >= MAX_PRIO) {
		fprintf(stderr, "[sched] WARNING: proc pid=%d has invalid prio=%d "
		                "(MAX_PRIO=%d), clamping to 0\n",
		                proc->pid, proc->prio, MAX_PRIO);
		proc->prio = 0;
	}

	proc->krnl->ready_queue      = &ready_queue;
	proc->krnl->mlq_ready_queue  = mlq_ready_queue;
	proc->krnl->running_list     = &running_list;
 
	pthread_mutex_lock(&queue_lock);
	/* Remove from running list, then return to its priority queue */
	purgequeue(&running_list, proc);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	/* FIX (Medium): validate priority before using it as array index */
	if (proc->prio >= MAX_PRIO) {
		fprintf(stderr, "[sched] WARNING: proc pid=%d has invalid prio=%d "
		                "(MAX_PRIO=%d), clamping to 0\n",
		                proc->pid, proc->prio, MAX_PRIO);
		proc->prio = 0;
	}

	proc->krnl->ready_queue      = &ready_queue;
	proc->krnl->mlq_ready_queue  = mlq_ready_queue;
	proc->krnl->running_list     = &running_list;
 
	pthread_mutex_lock(&queue_lock);
	/* New process: add directly to its priority queue */
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

/*
 * FIX (High): finish_mlq_proc() — called by the OS when a process has
 * completed execution (before free(proc)).  Removes the process from
 * running_list so no stale pointer is left behind.
 *
 * Usage in os.c: replace  free(proc)  with
 *     finish_proc(proc);
 *     free(proc);
 */
void finish_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	purgequeue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}

void finish_proc(struct pcb_t * proc) {
	return finish_mlq_proc(proc);
}

#else
struct pcb_t * get_proc(void) {
	struct pcb_t *proc = NULL;
 
	pthread_mutex_lock(&queue_lock);
	/* Pick the highest-priority process from the ready queue */
	proc = dequeue(&ready_queue);
	if (proc != NULL)
		enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
 
	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue  = &ready_queue;
	proc->krnl->running_list = &running_list;
 
	pthread_mutex_lock(&queue_lock);
	/* Process finished its time slice: remove from running list,
	 * put back to ready queue for the next round. */
	purgequeue(&running_list, proc);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue  = &ready_queue;
	proc->krnl->running_list = &running_list;
 
	pthread_mutex_lock(&queue_lock);
	/* Brand-new process: place in ready queue */
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}

/*
 * FIX (High): finish_proc() for the non-MLQ path.
 * Call this in os.c before free(proc).
 */
void finish_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue  = &ready_queue;
	proc->krnl->running_list = &running_list;

	pthread_mutex_lock(&queue_lock);
	purgequeue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
}
#endif

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
        // Khởi tạo slot cho mỗi hàng đợi theo công thức MAX_PRIO - priority
		slot[i] = MAX_PRIO - i; 
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	running_list.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/* * Stateful design for routine calling
 * based on the priority and our MLQ policy
 * We implement stateful here using transition technique
 * State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);
	/*TODO: get a process from PRIORITY [ready_queue].
	 * It worth to protect by a mechanism.
	 * */
    
    int found_queue_idx = -1;

    // Bước 1: Tìm hàng đợi có độ ưu tiên cao nhất mà KHÔNG rỗng VÀ CÒN slot
    for (int i = 0; i < MAX_PRIO; i++) {
        if (!empty(&mlq_ready_queue[i]) && slot[i] > 0) {
            found_queue_idx = i;
            break;
        }
    }

    // Bước 2: Nếu không tìm thấy (có thể do hết slot), kiểm tra xem hệ thống có rỗng hẳn không
    if (found_queue_idx == -1) {
        int system_has_process = 0;
        for (int i = 0; i < MAX_PRIO; i++) {
            if (!empty(&mlq_ready_queue[i])) {
                system_has_process = 1;
                break;
            }
        }

        // Nếu hệ thống còn process nhưng tất cả đều hết slot -> Reset lại toàn bộ slot
        if (system_has_process) {
            for (int i = 0; i < MAX_PRIO; i++) {
                slot[i] = MAX_PRIO - i;
            }
            // Tìm lại sau khi reset
            for (int i = 0; i < MAX_PRIO; i++) {
                if (!empty(&mlq_ready_queue[i]) && slot[i] > 0) {
                    found_queue_idx = i;
                    break;
                }
            }
        }
    }

    // Bước 3: Lấy tiến trình ra khỏi hàng đợi tìm được
    if (found_queue_idx != -1) {
        proc = dequeue(&mlq_ready_queue[found_queue_idx]);
        if (proc != NULL) {
            slot[found_queue_idx]--; // Giảm slot của hàng đợi đó
        }
    }

	if (proc != NULL) {
		enqueue(&running_list, proc);
    }
    
    pthread_mutex_unlock(&queue_lock);
	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 * It worth to protect by a mechanism.
	 * */
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list
	 * It worth to protect by a mechanism.
	 * */
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
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
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);
	/*TODO: get a process from [ready_queue].
	 * It worth to protect by a mechanism.
	 * */
    if (!empty(&ready_queue)) {
        proc = dequeue(&ready_queue);
        if (proc != NULL) {
            enqueue(&running_list, proc);
        }
    }
	pthread_mutex_unlock(&queue_lock);

	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 * It worth to protect by a mechanism.
	 * */
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 * It worth to protect by a mechanism.
	 * */
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif
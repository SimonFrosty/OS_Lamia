#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "queue.h"
#include "sched.h"
#include "syscall.h"

int __sys_killall(struct pcb_t *caller, struct sc_regs *regs)
{
    struct krnl_t *krnl = caller->krnl;
    int killed = 0;

    printf("[KILLALL] invoked by PID=%u\n", caller->pid);

    while (krnl->ready_queue->size > 0)
    {
        struct pcb_t *p = dequeue(krnl->ready_queue);
        if (p && p->pid != 0)
        {
            printf("[KILLALL] Terminated PID=%u (ready)\n", p->pid);
            free(p);
            killed++;
        }
    }

    while (krnl->running_list->size > 0)
    {
        struct pcb_t *p = dequeue(krnl->running_list);
        if (p && p->pid != 0)
        {
            printf("[KILLALL] Terminated PID=%u (running)\n", p->pid);
            free(p);
            killed++;
        }
    }

    printf("[KILLALL] %d processes were terminated.\n", killed);
    return killed;
}
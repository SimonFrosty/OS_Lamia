#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
    if (q == NULL)
        return 1;
    return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
    /* TODO: put a new process to queue [q] */
    if (q == NULL || proc == NULL) return;
    
    // Kiểm tra tràn hàng đợi
    if (q->size >= MAX_QUEUE_SIZE) {
        return; 
    }
    
    // Thêm tiến trình vào cuối hàng đợi
    q->proc[q->size] = proc;
    q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
    /* TODO: return a pcb whose prioprity is the highest
     * in the queue [q] and remember to remove it from q
     * */
    if (empty(q)) return NULL;

    int highest_prio_idx = 0; // Giả sử phần tử đầu tiên có ưu tiên cao nhất

    // Duyệt qua hàng đợi để tìm tiến trình có độ ưu tiên cao nhất (prio nhỏ nhất)
    // Lưu ý: Đề bài quy định giá trị càng nhỏ thì ưu tiên càng cao
    for (int i = 1; i < q->size; i++) {
#ifdef MLQ_SCHED
        // Trong chế độ MLQ, sử dụng trường 'prio'
        if (q->proc[i]->prio < q->proc[highest_prio_idx]->prio) {
            highest_prio_idx = i;
        }
#else
        // Chế độ thường, sử dụng trường 'priority'
        if (q->proc[i]->priority < q->proc[highest_prio_idx]->priority) {
            highest_prio_idx = i;
        }
#endif 
    }

    struct pcb_t *ret_proc = q->proc[highest_prio_idx];

    // Xóa phần tử đã lấy ra bằng cách dịch chuyển các phần tử phía sau lên
    for (int i = highest_prio_idx; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i + 1];
    }
    
    // Giảm kích thước hàng đợi
    q->proc[q->size - 1] = NULL;
    q->size--;

    return ret_proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
    /* TODO: remove a specific item from queue */
    if (empty(q) || proc == NULL) return NULL;

    int found_idx = -1;

    // Tìm vị trí của tiến trình cần xóa
    for (int i = 0; i < q->size; i++) {
        if (q->proc[i] == proc) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) return NULL; // Không tìm thấy

    struct pcb_t *removed_proc = q->proc[found_idx];

    // Dịch chuyển mảng để xóa phần tử
    for (int i = found_idx; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i + 1];
    }

    q->proc[q->size - 1] = NULL;
    q->size--;

    return removed_proc;
}
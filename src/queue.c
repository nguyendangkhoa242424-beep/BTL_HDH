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
        if (q == NULL || proc == NULL)
                return;

        /* FIX (Medium): log rõ khi hàng đợi đầy thay vì âm thầm bỏ qua,
         * tránh gây ra những lỗi khó tìm như treo chương trình hoặc
         * mất process không có lý do. */
        if (q->size >= MAX_QUEUE_SIZE) {
                fprintf(stderr, "[queue] ERROR: queue full (size=%d), "
                                "dropping pid=%d\n",
                                MAX_QUEUE_SIZE, proc->pid);
                return;
        }

        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */

        if (empty(q))
                return NULL;
 
        /* Find index of process with highest priority (lowest priority value) */
        int best = 0;
        int i;
        for (i = 1; i < q->size; i++) {
                if (q->proc[i]->priority < q->proc[best]->priority)
                        best = i;
        }
 
        struct pcb_t *proc = q->proc[best];
 
        /* Shift remaining elements left to fill the gap */
        for (i = best; i < q->size - 1; i++)
                q->proc[i] = q->proc[i + 1];
 
        q->size--;
        return proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */
        if (empty(q) || proc == NULL)
                return NULL;
 
        int i;
        for (i = 0; i < q->size; i++) {
                if (q->proc[i] == proc) {
                        struct pcb_t *found = q->proc[i];
                        /* Shift remaining elements left */
                        int j;
                        for (j = i; j < q->size - 1; j++)
                                q->proc[j] = q->proc[j + 1];
                        q->size--;
                        return found;
                }
        }
        return NULL;
}
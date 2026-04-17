#ifndef __AMC_QUEUE_H__
#define __AMC_QUEUE_H__

typedef struct AMCNode {
    void *data;
    void *next;
} AMCNode;

typedef struct AMCQueue {
    unsigned int length;
    AMCNode *head;
    AMCNode *tail;
} AMCQueue;

AMCQueue *amcqueue_init(void);
int amcqueue_free(AMCQueue *q);
void amcqueue_free_full(AMCQueue *q, void (*free_func)(void*));
void amc_enqueue(AMCQueue *q, void *data);
void *amc_dequeue(AMCQueue *q);
void *amcqueue_find_custom(AMCQueue *q, void *data, int (*compare_func)(const void*, const void*));
void *amcqueue_peek_nth(AMCQueue *q, unsigned int n);
void amcqueue_remove(AMCQueue *q, void *data);

#endif

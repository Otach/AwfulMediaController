#include "amc_queue.h"
#include <stdlib.h>

AMCQueue *amcqueue_init(void) {
    AMCQueue *q = calloc(1, sizeof(AMCQueue));
    if (q == NULL) {
        return NULL;
    }

    // Probably redundant since we call calloc
    q->length = 0;
    q->head = NULL;
    q->tail = NULL;

    return q;
}

int amcqueue_free(AMCQueue *q) {
    /* 
     * Frees the amcqueue object. Returns a negative int
     * when the queue is not empty. The caller must verify
     * the return code.
     */
    if (q->length != 0) {
        return -1;
    }
    free(q);
    return 0;
}

void amcqueue_free_full(AMCQueue *q, void (free_func)(void *)) {
    void *d;
    if (q->length != 0) {
        while ((d = amc_dequeue(q)) != NULL) {
            free_func(d);
        }
    }
    free(q);
    return;
}

void amc_enqueue(AMCQueue *q, void *data) {
    AMCNode *pt, *n = calloc(1, sizeof(AMCNode));
    n->data = data;
    n->next = NULL;

    if (q->tail != NULL) {
        pt = q->tail;
        pt->next = n;
    }
    q->length++;
    q->tail = n;

    if (q->head == NULL) {
        q->head = q->tail;
    }
}

void *amc_dequeue(AMCQueue *q) {
    AMCNode *n;
    void *data;
    if (q->length < 1) {
        return NULL;
    }
    n = q->head;

    q->head = n->next;
    q->length--;

    if (q->head == NULL) {
        // If the head is null, the queue is empty,
        //  so make sure the tail is also null.
        q->tail = NULL;
    }

    data = n->data;
    free(n);
    return data;
}

void *amcqueue_find_custom(AMCQueue *q, void *data, int (*compare_func)(const void *, const void*)) {
    AMCNode *n = q->head;

    // `compare_func` should operate like strcmp (0 on match)
    while (n != NULL) {
        if (compare_func(data, (void*)n->data) == 0) {
            return n->data;
        }
        n = n->next;
    }

    return NULL;
}

void *amcqueue_peek_nth(AMCQueue *q, unsigned int n) {
    AMCNode *nd;
    unsigned int count = 0;
    if (q->length < n) {
        return NULL;
    }
    nd = q->head;
    while (nd != NULL) {
        if (count == n) {
            return nd->data;
        }
        count++;
        nd = nd->next;
    }
    return NULL;
}

void amcqueue_remove(AMCQueue *q, void *data) {
    if (q->length == 0) { return; }
    AMCNode *pn = NULL, *n = q->head;

    while (n != NULL) {
        if (n->data == data) {
            if (pn == NULL) {
                // The node we were looking for was the head node.
                q->head = n->next;
                q->length--;
                if (q->length == 0) {
                    q->tail = NULL;
                }
            } else {
                pn->next = n->next;
                q->length--;
            }
            free(n);
            break;
        }
        pn = n;
        n = n->next;
    }
}

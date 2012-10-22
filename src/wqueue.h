#ifndef work_queue_HEADER
#define work_queue_HEADER

#include <pthread.h>

#include "object.h"
#include "queue.h"

#define WQUEUE_CALLBACK(XX) (void * (*) (void *)) XX
#define WQUEUE_THREAD_N 4

struct _wqueue_item {
    const struct _object * object;

    struct _wqueue * wqueue;
    int              thread_index;
    
    void * (* callback) (void *);
    void *    argument;
};

struct _wqueue_result {
    void * data;
    struct _wqueue_result * next;
};

struct _wqueue {
    const struct _object * object;

    struct _queue         * work_queue;
    struct _wqueue_result * results;
    struct _wqueue_result * results_last;

    int             thread_status[WQUEUE_THREAD_N];

    pthread_t       pthreads[WQUEUE_THREAD_N];
    pthread_attr_t  attr;
    pthread_mutex_t lock;
};


struct _wqueue * wqueue_create ();
void             wqueue_delete (struct _wqueue * wqueue);


struct _wqueue_item * wqueue_item_create (struct _wqueue * wqueue,
                                          void * (* callback) (void *),
                                          void * argument);
void                  wqueue_item_delete (struct _wqueue_item * wqueue_item);
struct _wqueue_item * wqueue_item_copy   (struct _wqueue_item * wqueue_item);

void   wqueue_push (struct _wqueue * wqueue,
                    void * (* callback) (void *),
                    void * argument);
void   wqueue_wait (struct _wqueue * wqueue);
void * wqueue_peek (struct _wqueue * wqueue);
void   wqueue_pop  (struct _wqueue * wqueue);


struct _wqueue_result * wqueue_result_create (void * data);
void                    wqueue_result_delete (struct _wqueue_result *);


void   wqueue_run    (struct _wqueue * wqueue);
void * wqueue_launch (struct _wqueue_item * wqueue_item);

#endif
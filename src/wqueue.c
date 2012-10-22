#include "wqueue.h"

#include <stdlib.h>
#include <stdio.h>

static const struct _object wqueue_item_object = {
    (void   (*) (void *)) wqueue_item_delete, 
    (void * (*) (void *)) wqueue_item_copy, 
    NULL,
    NULL
};

static const struct _object wqueue_object = {
    (void   (*) (void *)) wqueue_delete, 
    NULL,
    NULL,
    NULL
};


struct _wqueue * wqueue_create ()
{
    struct _wqueue * wqueue = (struct _wqueue *) malloc(sizeof(struct _wqueue));

    wqueue->object       = &wqueue_object;
    wqueue->work_queue   = queue_create();
    wqueue->results      = NULL;
    wqueue->results_last = NULL;

    int i;
    for (i = 0; i < WQUEUE_THREAD_N; i++) {
        wqueue->thread_status[i] = 0;
    }

    pthread_attr_init(&(wqueue->attr));
    pthread_attr_setdetachstate(&(wqueue->attr), PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&(wqueue->lock), NULL);


    return wqueue;
}


void wqueue_delete (struct _wqueue * wqueue)
{
    pthread_mutex_destroy(&(wqueue->lock));
    object_delete(wqueue->work_queue);
    while (wqueue->results != NULL) {
        struct _wqueue_result * next = wqueue->results->next;
        wqueue_result_delete(wqueue->results);
        wqueue->results = next;
    }
    free(wqueue);
}



struct _wqueue_item * wqueue_item_create (struct _wqueue * wqueue,
                                          void * (* callback) (void *),
                                          void * argument)
{
    struct _wqueue_item * wqueue_item;

    wqueue_item = (struct _wqueue_item *) malloc(sizeof(struct _wqueue_item));

    wqueue_item->object   = &wqueue_item_object;
    wqueue_item->wqueue   = wqueue;
    wqueue_item->callback = callback;
    wqueue_item->argument = object_copy(argument);

    return wqueue_item;
}


void wqueue_item_delete (struct _wqueue_item * wqueue_item)
{
    object_delete(wqueue_item->argument);
    free(wqueue_item);
}


struct _wqueue_item * wqueue_item_copy (struct _wqueue_item * wqueue_item)
{
    return wqueue_item_create(wqueue_item->wqueue,
                              wqueue_item->callback,
                              wqueue_item->argument);
}



void wqueue_push (struct _wqueue * wqueue,
                  void * (*callback) (void *),
                  void * argument)
{
    struct _wqueue_item * wqueue_item;
    wqueue_item = wqueue_item_create(wqueue, callback, argument);

    // add work item to queue
    queue_push(wqueue->work_queue, wqueue_item);
    object_delete(wqueue_item);
}


void wqueue_wait (struct _wqueue * wqueue)
{
    // keep doing work
    pthread_mutex_lock(&(wqueue->lock));
    while (wqueue->work_queue->size > 0) {
        pthread_mutex_unlock(&(wqueue->lock));
        wqueue_run(wqueue);
        pthread_mutex_lock(&(wqueue->lock));
    }
    pthread_mutex_unlock(&(wqueue->lock));

    // all works has been launched by threads, wait for threads
    // to finish
    int i = 0;
    for (i = 0; i < WQUEUE_THREAD_N; i++) {
        void * status;
        pthread_join(wqueue->pthreads[i], &status);
    }
}


void * wqueue_peek (struct _wqueue * wqueue)
{
    if (wqueue->results == NULL)
        return NULL;
    else
        return wqueue->results->data;
}


void wqueue_pop (struct _wqueue * wqueue)
{
    if (wqueue->results == NULL)
        return;

    struct _wqueue_result * tmp = wqueue->results;
    wqueue->results = tmp->next;
    wqueue_result_delete(tmp);
}


struct _wqueue_result * wqueue_result_create (void * data)
{
    struct _wqueue_result * wqueue_result;

    wqueue_result = (struct _wqueue_result *) malloc(sizeof(struct _wqueue_result));

    wqueue_result->data = data;
    wqueue_result->next = NULL;

    return wqueue_result;
}


void wqueue_result_delete (struct _wqueue_result * wqueue_result)
{
    object_delete(wqueue_result->data);
    free(wqueue_result);
}


void wqueue_run (struct _wqueue * wqueue)
{
    if (wqueue->work_queue->size == 0) {
        return;
    }

    // search for a free thread
    pthread_mutex_lock(&(wqueue->lock));
    int free_thread;
    for (free_thread = 0; free_thread < WQUEUE_THREAD_N; free_thread++) {
        if (wqueue->thread_status[free_thread] == 0)
            break;
    }
    pthread_mutex_unlock(&(wqueue->lock));

    if (free_thread == WQUEUE_THREAD_N) {
        return;
    }

    wqueue->thread_status[free_thread] = 1;

    // get the work item
    struct _wqueue_item * wqueue_item = object_copy(queue_peek(wqueue->work_queue));
    queue_pop(wqueue->work_queue);

    // set this item's thread_index
    wqueue_item->thread_index = free_thread;
    
    // launch
    pthread_create(&(wqueue->pthreads[free_thread]),
                   &(wqueue->attr),
                   (void * (*) (void *)) wqueue_launch,
                   (void *) wqueue_item);
}


void * wqueue_launch (struct _wqueue_item * wqueue_item)
{
    void * result = wqueue_item->callback(wqueue_item->argument);

    struct _wqueue * wqueue = wqueue_item->wqueue;

    struct _wqueue_result * wqueue_result = wqueue_result_create(result);

    pthread_mutex_lock(&(wqueue->lock));

    // add result to results
    if (wqueue->results == NULL) {
        wqueue->results = wqueue_result;
        wqueue->results_last = wqueue_result;
    }
    else {
        wqueue->results_last->next = wqueue_result;
        wqueue->results_last = wqueue_result;
    }

    // set this thread as free
    wqueue->thread_status[wqueue_item->thread_index] = 0;

    // unlock and look for new work items
    pthread_mutex_unlock(&(wqueue->lock));

    return (void *) 0;
}
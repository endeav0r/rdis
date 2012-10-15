#include "queue.h"

static const struct _object queue_object = {
    (void   (*) (void *))         queue_delete, 
    (void * (*) (void *))         queue_copy,
    NULL,
    NULL
};


struct _queue * queue_create ()
{
    struct _queue * queue;

    queue = malloc(sizeof(struct _queue));
    queue->object = &queue_object;
    queue->size   = 0;
    queue->front  = NULL;
    queue->back   = NULL;

    return queue;
}



void queue_delete (struct _queue * queue)
{
    struct _queue_it * it;
    struct _queue_it * next;

    it = queue->front;
    while (it != NULL) {
        next = it->next;
        object_delete(it);
        free(it);
        it = next;
    }

    free(queue);
}



struct _queue * queue_copy (struct _queue * queue) {
    struct _queue * new_queue = queue_create();
    struct _queue_it * it;

    for (it = queue->front; it != NULL; it = it->next) {
        queue_push(new_queue, it->data);
    }

    return new_queue;
}



void queue_push (struct _queue * queue, void * data)
{
    struct _queue_it * it = queue_it_create(data);

    if (queue->back == NULL) {
        queue->back = it;
        queue->front = it;
    }
    else {
        queue->back->next = it;
        queue->back = it;
    }

    queue->size++;
}



void * queue_peek (struct _queue * queue)
{
    if (queue->front == NULL)
        return NULL;
    return queue->front->data;
}



void queue_pop (struct _queue * queue)
{
    struct _queue_it * it;

    if (queue->front == NULL)
        return;

    it = queue->front;
    queue->front = it->next;
    if (queue->front == NULL)
        queue->back = NULL;

    object_delete(it->data);
    free(it);

    queue->size--;
}



struct _queue_it * queue_it_create (void * data)
{
    struct _queue_it * it;

    it = (struct _queue_it *) malloc(sizeof(struct _queue_it));
    it->data = object_copy(data);
    it->next = NULL;

    return it;
}
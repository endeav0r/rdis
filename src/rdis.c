#include "rdis.h"

#include <stdio.h>
#include <stdlib.h>

static const struct _object rdis_callback_object = {
    (void   (*) (void *))         rdis_callback_delete, 
    (void * (*) (void *))         rdis_callback_copy,
    (int    (*) (void *, void *)) rdis_callback_cmp,
    NULL
};


static const struct _object rdis_object = {
    (void   (*) (void *))         rdis_delete, 
    NULL,
    NULL,
    NULL
};


struct _rdis * rdis_create (_loader * loader)
{
    struct _rdis * rdis;

    rdis = (struct _rdis *) malloc(sizeof(struct _rdis));

    rdis->object = &rdis_object;
    rdis->callback_counter = 0;
    rdis->callbacks        = map_create();
    rdis->loader           = loader;

    rdis->graph         = loader_graph(loader);
    rdis->labels        = loader_labels(loader);
    rdis->function_tree = loader_function_tree(loader);

    return rdis;
}


void rdis_delete (struct _rdis * rdis)
{
    object_delete(rdis->callbacks);
    object_delete(rdis->loader);
    object_delete(rdis->graph);
    object_delete(rdis->labels);
    object_delete(rdis->function_tree);
    free(rdis);
}




uint64_t rdis_add_callback (struct _rdis * rdis,
                            void (* callback) (void *),
                            void * data)
{
    struct _rdis_callback * rc = rdis_callback_create(callback, data);
    uint64_t identifier = rdis->callback_counter++;

    rc->identifier = identifier;

    printf("adding callback %p %p %llx\n",
           rc->callback, rc->data, (unsigned long long) rc->identifier);

    map_insert(rdis->callbacks, identifier, rc);
    object_delete(rc);

    return identifier;
}


void rdis_remove_callback (struct _rdis * rdis, uint64_t identifier)
{
    map_remove(rdis->callbacks, identifier);
}


void rdis_callback (struct _rdis * rdis)
{
    struct _map_it * it;

    for (it = map_iterator(rdis->callbacks); it != NULL; it = map_it_next(it)) {
        struct _rdis_callback * rc = map_it_data(it);
        printf("callback %p %llx\n",
               rc->callback, (unsigned long long) rc->identifier);
        fflush(stdout);

        rc->callback(rc->data);
    }
}





struct _rdis_callback * rdis_callback_create (void (* callback) (void *),
                                              void * data)
{
    struct _rdis_callback * rc;

    rc = (struct _rdis_callback *) malloc(sizeof(struct _rdis_callback));
    rc->object     = &rdis_callback_object;
    rc->identifier = 0;
    rc->callback   = callback;
    rc->data       = data;

    return rc;
}


void rdis_callback_delete (struct _rdis_callback * rc)
{
    free(rc);
}


struct _rdis_callback * rdis_callback_copy (struct _rdis_callback * rc)
{
    struct _rdis_callback * rc_copy;

    rc_copy = rdis_callback_create(rc->callback, rc->data);
    rc_copy->identifier = rc->identifier;

    return rc_copy;
}


int rdis_callback_cmp (struct _rdis_callback * lhs, struct _rdis_callback * rhs)
{
    if (lhs->identifier < rhs->identifier)
        return -1;
    else if (lhs->identifier > rhs->identifier)
        return 1;
    return 0;
}
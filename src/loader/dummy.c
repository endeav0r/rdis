#include "dummy.h"

#include <stdlib.h>

static const struct _loader_object dummy_object = {
    {
        (void     (*) (void *)) dummy_loader_delete,
        NULL,
        NULL,
        NULL,
        (json_t * (*) (void *)) dummy_loader_serialize
    },
    (uint64_t        (*) (void *))           dummy_loader_entry,
    (struct _graph * (*) (void *))           dummy_loader_graph,
    (struct _map *   (*) (void *))           dummy_loader_functions,
    (struct _map  *  (*) (void *))           dummy_loader_labels,
    (struct _graph * (*) (void *, uint64_t)) dummy_loader_graph_address,
    (struct _map *   (*) (void *))           dummy_loader_memory_map,
    (struct _map  *  (*) (void *, uint64_t)) dummy_loader_function_address,
    (struct _label * (*) (void *, uint64_t)) dummy_loader_label_address
};


struct _dummy_loader * dummy_loader_create ()
{
    struct _dummy_loader * dl;
    dl = (struct _dummy_loader *) malloc(sizeof(struct _dummy_loader));
    dl->loader_object = &dummy_object;
    return dl;
}


void dummy_loader_delete (struct _dummy_loader * dl)
{
    free(dl);
}


json_t * dummy_loader_serialize (struct _dummy_loader * dl)
{
    return json_null();
}


uint64_t dummy_loader_entry (struct _dummy_loader * dl)
{
    return 0;
}


struct _graph * dummy_loader_graph (struct _dummy_loader * dl)
{
    return graph_create();
}


struct _map * dummy_loader_functions (struct _dummy_loader * dl)
{
    return map_create();
}


struct _map * dummy_loader_labels (struct _dummy_loader * dl)
{
    return map_create();
}


struct _graph * dummy_loader_graph_address (struct _dummy_loader * dl, uint64_t addr)
{
    return graph_create();
}


struct _map * dummy_loader_memory_map (struct _dummy_loader * dl)
{
    return map_create();
}


struct _label * dummy_loader_label_address (struct _dummy_loader * dl, uint64_t addr)
{
    return NULL;
}


struct _map * dummy_loader_function_address (struct _dummy_loader * dl, uint64_t addr)
{
    return map_create();
}
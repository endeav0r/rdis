#include "rdis.h"

#include "index.h"
#include "instruction.h"
#include "label.h"
#include "queue.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

static const struct _object rdis_callback_object = {
    (void     (*) (void *))         rdis_callback_delete, 
    (void *   (*) (void *))         rdis_callback_copy,
    (int      (*) (void *, void *)) rdis_callback_cmp,
    NULL,
};


static const struct _object rdis_object = {
    (void   (*) (void *))         rdis_delete, 
    NULL,
    NULL,
    NULL,
    (json_t * (*) (void *))         rdis_serialize
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
    rdis->memory_map    = loader_memory_map(loader);

    return rdis;
}


void rdis_delete (struct _rdis * rdis)
{
    object_delete(rdis->callbacks);
    object_delete(rdis->graph);
    object_delete(rdis->labels);
    object_delete(rdis->function_tree);
    object_delete(rdis->memory_map);
    
    if (rdis->loader != NULL)
        object_delete(rdis->loader);
    free(rdis);
}


json_t * rdis_serialize (struct _rdis * rdis)
{
    json_t * json = json_object();

    json_object_set(json, "ot",     json_integer(SERIALIZE_RDIS));
    json_object_set(json, "graph",  object_serialize(rdis->graph));
    json_object_set(json, "labels", object_serialize(rdis->labels));
    json_object_set(json, "function_tree", object_serialize(rdis->function_tree));
    json_object_set(json, "memory_map", object_serialize(rdis->memory_map));

    return json;
}


struct _rdis * rdis_deserialize (json_t * json)
{
    json_t * graph         = json_object_get(json, "graph");
    json_t * labels        = json_object_get(json, "labels");
    json_t * function_tree = json_object_get(json, "function_tree");
    json_t * memory_map    = json_object_get(json, "memory_map");

    if (! json_is_object(graph))
        return NULL;
    if (! json_is_object(labels))
        return NULL;
    if (! json_is_object(function_tree))
        return NULL;
    if (! json_is_object(memory_map))
        return NULL;

    struct _graph * ggraph = deserialize(graph);
    if (ggraph == NULL)
        return NULL;
    struct _map * llabels = deserialize(labels);
    if (llabels == NULL) {
        object_delete(ggraph);
        return NULL;
    }
    struct _tree * ffunction_tree = deserialize(function_tree);
    if (ffunction_tree == NULL) {
        object_delete(ggraph);
        object_delete(llabels);
        return NULL;
    }
    struct _map * mmemory_map = deserialize(memory_map);
    if (mmemory_map == NULL) {
        object_delete(ggraph);
        object_delete(llabels);
        object_delete(mmemory_map);
        return NULL;
    }

    struct _rdis * rdis = (struct _rdis *) malloc(sizeof(struct _rdis));

    rdis->object           = &rdis_object;
    rdis->callback_counter = 0;
    rdis->callbacks        = map_create();
    rdis->loader           = NULL;
    rdis->graph            = ggraph;
    rdis->labels           = llabels;
    rdis->function_tree    = ffunction_tree;
    rdis->memory_map       = mmemory_map;

    return rdis;
}


int rdis_user_function (struct _rdis * rdis, uint64_t address)
{
    printf("rdis_user_function %llx\n", (unsigned long long) address);
    // create an index and label for this function
    struct _index * index = index_create(address);
    // no double dipping
    if (tree_fetch(rdis->function_tree, index) != NULL) {
        object_delete(index);
        return -1;
    }
    tree_insert(rdis->function_tree, index);
    object_delete(index);

    char tmp[128];
    snprintf(tmp, 128, "ufun_%llx", (unsigned long long) address);

    struct _label * label = label_create(address, tmp, LABEL_FUNCTION);
    map_insert(rdis->labels, address, label);
    object_delete(label);

    // if this function is already in our graph, all we need to do is make
    // sure its a separate node and then remove function predecessors
    struct _graph_node * node = graph_fetch_node_max(rdis->graph, address);
    if (node != NULL) {

        // already a node, remove function predecessors
        if (node->index == address) {
            remove_function_predecessors(rdis->graph, rdis->function_tree);
            rdis_callback(rdis);
            return 0;
        }

        // search for instruction with given address
        struct _list    * ins_list = node->data;
        struct _list_it * it;
        for (it = list_iterator(ins_list); it != NULL; it = it->next) {
            struct _ins * ins = it->data;
            // found instruction
            if (ins->address == address) {
                // create a new instruction list.
                // add remaining instructions to new list while removing them from
                // the current list
                struct _list * new_ins_list = list_create();
                while (1) {
                    list_append(new_ins_list, it->data);
                    it = list_remove(ins_list, it);
                    if (it == NULL)
                        break;
                }
                // create a new graph node for this new function
                graph_add_node(rdis->graph, address, new_ins_list);
                // all graph successors from old node are added to new node
                struct _queue   * queue      = queue_create();
                struct _list    * successors = graph_node_successors(node);
                struct _list_it * sit;
                for (sit = list_iterator(successors);
                     sit != NULL;
                     sit = sit->next) {
                    struct _graph_edge * edge = sit->data;
                    graph_add_edge(rdis->graph, address, edge->tail, edge->data);
                    queue_push(queue, edge);
                }
                object_delete(successors);

                // and removed from old node
                while (queue->size > 0) {
                    struct _graph_edge * edge = queue_peek(queue);
                    graph_remove_edge(rdis->graph, edge->head, edge->tail);
                    queue_pop(queue);
                }
                object_delete(queue);

                // that was easy
                rdis_callback(rdis);
                return 0;
            }
        }
    }

    // we need to create a new graph for this node
    struct _graph * graph = loader_graph_address(rdis->loader, address);
    graph_reduce(graph);
    graph_merge(rdis->graph, graph);
    object_delete(graph);

    rdis_callback(rdis);

    return 0;
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
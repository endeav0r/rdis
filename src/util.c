#include "util.h"

#include "function.h"
#include "graph.h"
#include "index.h"
#include "instruction.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>


void remove_function_predecessors (struct _graph * graph, struct _map * functions)
{
    struct _queue * queue = queue_create();

    struct _map_it * mit;
    for (mit = map_iterator(functions); mit != NULL; mit = map_it_next(mit)) {
        struct _function * function = map_it_data(mit);
        struct _graph_node * node = graph_fetch_node(graph, function->address);

        if (node == NULL) {
            printf("remove_function_predecessors null node %llx\n",
                   (unsigned long long) function->address);
            continue;
        }

        struct _list * predecessors = graph_node_predecessors(node);
        struct _list_it * lit;
        for (lit = list_iterator(predecessors); lit != NULL; lit = lit->next) {
            struct _graph_edge * edge = lit->data;
            queue_push(queue, edge);
        }
    }

    while (queue->size > 0) {
        struct _graph_edge * edge = queue_peek(queue);
        graph_remove_edge(graph, edge->head, edge->tail);
        queue_pop(queue);
    }

    object_delete(queue);
}


void create_call_graph_list (struct _graph_node * node, struct _list * call_list)
{
    struct _list * ins_list = node->data;
    struct _list_it * it;
    for (it = list_iterator(ins_list); it != NULL; it = it->next) {
        struct _ins * ins = it->data;
        if (ins->flags & INS_FLAG_CALL) {
            list_append(call_list, ins);
        }
    }
}


struct _graph * create_call_graph (struct _graph * graph, uint64_t indx)
{
    struct _graph_node * node = graph_fetch_node(graph, indx);

    if (node == NULL)
        return NULL;

    struct _graph * call_graph     = graph_create();
    struct _queue * function_queue = queue_create();
    struct _queue * edge_queue     = queue_create();

    struct _index * index = index_create(node->index);
    queue_push(function_queue, index);
    object_delete(index);

    while (function_queue->size > 0) {
        struct _index * index = queue_peek(function_queue);

        if (graph_fetch_node(call_graph, index->index) != NULL) {
            queue_pop(function_queue);
            continue;
        }

        struct _list * call_list = list_create();

        if (graph_fetch_node(graph, index->index) == NULL) {
            printf("didn't find node %llx\n", (unsigned long long) index->index);
        }
        
        graph_bfs_data(graph, index->index, call_list,
             (void (*) (struct _graph_node *, void *)) create_call_graph_list);

        // for every call we are making
        struct _list_it * it;
        for (it = list_iterator(call_list); it != NULL; it = it->next) {
            // if it has a target and the target points to a valid node and
            // we haven't already added that function
            struct _ins * ins = it->data;
            if (    (ins->flags & INS_FLAG_TARGET_SET)
                 && (graph_fetch_node(graph, ins->target) != NULL)
                 && (graph_fetch_node(call_graph, ins->target) == NULL)) {
                // add the target to the function queue
                struct _index * new_index = index_create(ins->target);
                queue_push(function_queue, new_index);
                object_delete(new_index);
                // and add an edge for later
                struct _ins_edge * ins_edge = ins_edge_create(INS_EDGE_NORMAL);
                struct _graph_edge * edge;
                edge = graph_edge_create(index->index, ins->target, ins_edge);
                queue_push(edge_queue, edge);
                object_delete(edge);
                object_delete(ins_edge);
            }
        }

        // create this node in the call graph
        graph_add_node(call_graph, index->index, call_list);

        object_delete(call_list);

        queue_pop(function_queue);
    }
    object_delete(function_queue);

    // add all the edges
    while (edge_queue->size > 0) {
        struct _graph_edge * edge = queue_peek(edge_queue);

        graph_add_edge(call_graph, edge->head, edge->tail, edge->data);

        queue_pop(edge_queue);
    }

    return call_graph;
}


int is_string (uint8_t * data, size_t data_size)
{
    int result = STRING_ASCII;
    size_t i;

    for (i = 0; i < data_size; i++) {
        if (data[i] & 0x80) {
            result = 0;
            break;
        }
    }

    if (i < 4)
        return 0;
    return result;
}


size_t rdstrcat (char * dst, char * src, size_t size)
{
    size_t len = strlen(dst);
    char * s = src;
    char * d = &(dst[len]);

    while (*s != '\0') {
        if (len >= size)
            break;
        *d = *s;
        d++;
        s++;
        len++;
    }

    if (len < size)
        dst[len] = '\0';
    else
        dst[size - 1] = '\0';

    return len;
}
#include "util.h"

#include "index.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>


void remove_function_predecessors (struct _graph * graph, struct _tree * functions)
{
    struct _queue * queue = queue_create();

    struct _tree_it * tit;
    for (tit = tree_iterator(functions); tit != NULL; tit = tree_it_next(tit)) {
        struct _index * index = tree_it_data(tit);
        struct _graph_node * node = graph_fetch_node(graph, index->index);

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
#include "graph.h"

#include "instruction.h"
#include "queue.h"

#include <stdio.h>

static const struct _object graph_index_object = {
    (void   (*) (void *))         graph_index_delete, 
    (void * (*) (void *))         graph_index_copy,
    (int    (*) (void *, void *)) graph_index_cmp,
    NULL
};

static const struct _object graph_edge_object = {
    (void   (*) (void *))         graph_edge_delete, 
    (void * (*) (void *))         graph_edge_copy,
    (int    (*) (void *, void *)) graph_edge_cmp,
    NULL
};

static const struct _object graph_node_object = {
    (void   (*) (void *))         graph_node_delete,
    (void * (*) (void *))         graph_node_copy,
    (int    (*) (void *, void *)) graph_node_cmp,
    NULL,
};

static const struct _object graph_object = {
    (void   (*) (void *))         graph_delete,
    (void * (*) (void *))         graph_copy,
    NULL,
    (void   (*) (void *, void *)) graph_merge,
};

struct _graph * graph_create ()
{
    struct _graph * graph;

    graph = (struct _graph *) malloc(sizeof(struct _graph));
    graph->object = &graph_object;
    graph->nodes = tree_create(graph_cmp);

    return graph;
}



void graph_delete (struct _graph * graph)
{
    tree_delete(graph->nodes);
    free(graph);
}


struct _graph * graph_copy (struct _graph * graph)
{
    struct _graph * new_graph;

    new_graph = graph_create();

    // we have to manually enter the new graph nodes. copying them will cause
    // them to point to the old graph
    struct _graph_it * it;
    for (it = graph_iterator(graph); it != NULL; it = graph_it_next(it)) {
        graph_add_node(new_graph, graph_it_index(it), graph_it_data(it));
    }

    for (it = graph_iterator(graph); it != NULL; it = graph_it_next(it)) {
        struct _list * edges = graph_it_edges(it);
        struct _list_it * eit;
        for (eit = list_iterator(edges); eit != NULL; eit = eit->next) {
            struct _graph_edge * edge = eit->data;
            graph_add_edge(new_graph, edge->head, edge->tail, edge->data);
        }
    }

    return new_graph;
}



void graph_merge_node_edges (struct _graph_node * lhs, struct _graph_node * rhs)
{
    struct _list_it *    rhs_it;
    struct _graph_edge * rhs_edge;
    struct _list_it *    lhs_it;
    struct _graph_edge * lhs_edge;
    int found;

    for (rhs_it =  list_iterator(rhs->edges);
         rhs_it != NULL;
         rhs_it =  rhs_it->next) {
        rhs_edge = rhs_it->data;

        found = 0;
        for (lhs_it =  list_iterator(lhs->edges);
             lhs_it != NULL;
             lhs_it =  lhs_it->next) {

            lhs_edge = lhs_it->data;
            if (    (lhs_edge->head == rhs_edge->head)
                 && (lhs_edge->tail == rhs_edge->tail)) {
                found = 1;
                break;
            }
        }

        if (found == 0) {
            list_append(lhs->edges, rhs_edge);
        }
    }
}



void graph_merge (struct _graph * graph, struct _graph * rhs)
{
    struct _graph_it * it;
    struct _graph_node * node;
    struct _graph_node * node_ptr;

    for (it = graph_iterator(rhs); it != NULL; it = graph_it_next(it)) {
        // does this node exist already?
        node_ptr = graph_fetch_node(graph, graph_it_index(it));
        if (node_ptr) {
            // do we have new edges?
            graph_merge_node_edges(node_ptr, graph_it_node(it));
            continue;
        }

        // node doesn't exist, create it
        node = graph_node_copy(graph_it_node(it));
        node->graph = graph;

        tree_insert(graph->nodes, node);

        object_delete(node);
    }
}


// this is the slowest function in rdis, so things may be done a bit
// differently here
void graph_reduce (struct _graph * graph)
{
    // if a node has one successor and that node has one predecessor,
    // merge the nodes into one

    // this is a list of the node indexes we need to check
    struct _list        * node_list;
    struct _graph_it    * graph_it;
    struct _graph_index * index;

    node_list = list_create();
    for (graph_it = graph_iterator(graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        index = graph_index_create(graph_it_index(graph_it));
        list_append(node_list, index);
        object_delete(index);
    }

    struct _list_it * node_it;
    struct _graph_node * head_node;
    struct _graph_node * tail_node;
    node_it = list_iterator(node_list);
    while (node_it != NULL) {

        index = node_it->data;

        head_node = graph_fetch_node(graph, index->index);

        // we have removed this node from the graph
        if (head_node == NULL) {
            node_it = node_it->next;
            continue;
        }

        // if this node has only one successor
        if (graph_node_successors_n(head_node) != 1) {
            node_it = node_it->next;
            continue;
        }

        // get that successor
        struct _list * successors = graph_node_successors(head_node);
        struct _graph_edge * successor_edge = list_first(successors);
        tail_node = graph_fetch_node(graph, successor_edge->tail);
        object_delete(successors);

        if (tail_node == NULL) {
            fprintf(stderr,
                    "graph_reduce error, could not find tail node %llx\n",
                    (unsigned long long) successor_edge->tail);
            exit(-1);
        }

        // how many predecessors does the tail node have?
        if (graph_node_predecessors_n(tail_node) != 1) {
            node_it = node_it->next;
            continue;
        }

        // merge the tail information into head information
        object_merge(head_node->data, tail_node->data);

        // head removes its successor
        struct _list_it * suc_it;
        for (suc_it = list_iterator(head_node->edges);
             suc_it != NULL;
             suc_it = suc_it->next) {
            successor_edge = suc_it->data;
            if (successor_edge->head == head_node->index) {
                list_remove(head_node->edges, suc_it);
                break;
            }
        }

        // head creates a new successor using tail's successors
        // meanwhile, patch tail's successors
        successors = graph_node_successors(tail_node);
        // for each of tail's successors
        for (suc_it = list_iterator(successors);
             suc_it != NULL;
             suc_it = suc_it->next) {

            successor_edge = suc_it->data;

            // create an edge in head_node pointing to this successor
            struct _graph_edge * new_edge;
            new_edge = object_copy(successor_edge);
            new_edge->head = head_node->index;
            list_append(head_node->edges, new_edge);
            object_delete(new_edge);

            // patch tail's successors
            struct _graph_node * tail_suc_node;
            // tail_suc_node is the successor node
            tail_suc_node = graph_fetch_node(graph, successor_edge->tail);
            struct _list_it * tail_suc_it;
            // for each of the successor's edges
            for (tail_suc_it  = list_iterator(tail_suc_node->edges);
                 tail_suc_it != NULL;
                 tail_suc_it  = tail_suc_it->next) {
                struct _graph_edge * tail_suc_edge = tail_suc_it->data;
                // if the head of this edge was tail, it is now head
                if (tail_suc_edge->head == tail_node->index)
                    tail_suc_edge->head = head_node->index;
            }
        }
        object_delete(successors);

        // remove tail node from graph

        tree_delete_node(graph->nodes, tail_node);

        // continue processing this node
    }
    object_delete(node_list);
}



struct _graph * graph_family (struct _graph * graph, uint64_t indx)
{
    struct _graph       * new_graph = graph_create();
    struct _graph_index * index;
    struct _queue       * queue = queue_create();

    index = graph_index_create(indx);
    queue_push(queue, index);
    object_delete(index);

    while (queue->size > 0) {
        index = object_copy(queue_peek(queue));
        queue_pop(queue);

        if (graph_fetch_node(new_graph, index->index) != NULL) {
            object_delete(index);
            continue;
        }

        struct _graph_node * node = graph_fetch_node(graph, index->index);
        if (node == NULL) {
            fprintf(stderr, "graph_family could not find node %llx\n",
                    (unsigned long long) index->index);
            exit(-1);
        }
        tree_insert(new_graph->nodes, node);

        struct _list_it * it;
        object_delete(index);
        for (it = list_iterator(node->edges); it != NULL; it = it->next) {
            struct _graph_edge * edge = it->data;
            if (edge->head == node->index)
                index = graph_index_create(edge->tail);
            else
                index = graph_index_create(edge->head);
            queue_push(queue, index);
            object_delete(index);
        }
    }

    object_delete(queue);

    return new_graph;
}



void graph_add_node (struct _graph * graph, uint64_t index, void * data)
{
    struct _graph_node * node;

    node = graph_node_create(graph, index, data);

    tree_insert(graph->nodes, node);

    object_delete(node);
}



struct _graph_node * graph_fetch_node (struct _graph * graph,
                                       uint64_t index)
{
    struct _graph_node * node        = graph_node_create(graph, index, NULL);
    struct _graph_node * target_node = tree_fetch(graph->nodes, node);
    object_delete(node);
    return target_node;
}



void * graph_fetch_data (struct _graph * graph, uint64_t index)
{
    struct _graph_node * node = graph_fetch_node(graph, index);
    if (node == NULL)
        return NULL;
    return node->data;
}



struct _list * graph_fetch_edges (struct _graph * graph, uint64_t index)
{
    struct _graph_node * node = graph_fetch_node(graph, index);
    if (node == NULL)
        return NULL;
    return node->edges;
}



struct _graph_node * graph_fetch_node_max (struct _graph * graph,
                                           uint64_t index)
{
    struct _graph_node * node        = graph_node_create(graph, index, NULL);
    struct _graph_node * target_node = tree_fetch_max(graph->nodes, node);
    object_delete(node);
    return target_node;
}



int graph_add_edge (struct _graph * graph,
                    uint64_t head_needle,
                    uint64_t tail_needle,
                    void * data)
{
    struct _graph_edge * edge;
    struct _graph_node * head_node;
    struct _graph_node * tail_node;

    head_node = graph_fetch_node(graph, head_needle);
    tail_node = graph_fetch_node(graph, tail_needle);

    if ((head_node == NULL) || (tail_node == NULL))
        return -1;

    edge = graph_edge_create(head_node->index, tail_node->index, data);

    // do not add a duplicate edge
    struct _list_it * it;
    for (it = list_iterator(head_node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge_ptr = it->data;
        if (object_cmp(edge, edge_ptr) == 0) {
            object_delete(edge);
            return -1;
        }
    }

    list_append(head_node->edges, edge);
    list_append(tail_node->edges, edge);

    object_delete(edge);

    return 0;
}



int graph_remove_edge (struct _graph * graph,
                       uint64_t head_needle,
                       uint64_t tail_needle)
{
    struct _graph_edge edge;
    struct _graph_node * head_node;
    struct _graph_node * tail_node;
    struct _graph_edge * edge_ptr;

    head_node = graph_fetch_node(graph, head_needle);
    tail_node = graph_fetch_node(graph, tail_needle);
    if ((head_node == NULL) || (tail_node == NULL))
        return -1;

    edge.head = head_node->index;
    edge.tail = tail_node->index;

    struct _list_it * it;
    for (it = list_iterator(head_node->edges); it != NULL; it = it->next) {
        edge_ptr = (struct _graph_edge *) it->data;
        if ((edge_ptr->head == edge.head) && (edge_ptr->tail == edge.tail)) {
            list_remove(head_node->edges, it);
            break;
        }
    }

    for (it = list_iterator(tail_node->edges); it != NULL; it = it->next) {
        edge_ptr = (struct _graph_edge *) it->data;
        if ((edge_ptr->head == edge.head) && (edge_ptr->tail == edge.tail)) {
            list_remove(tail_node->edges, it);
            break;
        }
    }

    return 0;
}



void graph_map (struct _graph * graph, void (* callback) (struct _graph_node *))
{
    tree_map(graph->nodes, (void (*) (void *)) callback);
}


void graph_bfs (struct _graph * graph,
                uint64_t        indx,
                void  (* callback) (struct _graph *, struct _graph_node *))
{
    struct _queue       * queue   = queue_create();
    struct _tree        * visited = tree_create();
    struct _graph_index * index;

    // add the first index to the graph
    index = graph_index_create(indx);
    queue_push(queue, index);
    object_delete(index);

    while (queue->size > 0) {
        index = object_copy(queue_peek(queue));
        queue_pop(queue);
        if (tree_fetch(visited, index) != NULL) {
            object_delete(index);
            continue;
        }
        tree_insert(visited, index);

        struct _graph_node * node = graph_fetch_node(graph, index->index);
        if (node == NULL) {
            printf("graph_bfs didn't find node %llx\n",
                   (unsigned long long) index->index);
        }
        object_delete(index);

        printf("graph_bfs node: %p graph: %p\n", node, graph);
        callback(graph, node);

        // add successors to the queue
        struct _list * successors = graph_node_successors(node);
        struct _list_it * it;
        for (it = list_iterator(successors); it != NULL; it = it->next) {
            struct _graph_edge * edge = it->data;
            index = graph_index_create(edge->tail);
            queue_push(queue, index);
            object_delete(index);
        }
        object_delete(successors);
    }
    object_delete(queue);
    object_delete(visited);
}


struct _graph_index * graph_index_create (uint64_t index)
{
    struct _graph_index * index_ptr;
    index_ptr = (struct _graph_index *) malloc(sizeof(struct _graph_index));
    index_ptr->object = &graph_index_object;
    index_ptr->index = index;
    return index_ptr;
}



void graph_index_delete (struct _graph_index * index)
{
    free(index);
}



struct _graph_index * graph_index_copy (struct _graph_index * index)
{
    return graph_index_create(index->index);
}



int graph_index_cmp (struct _graph_index * lhs, struct _graph_index * rhs)
{
    if (lhs->index < rhs->index)
        return -1;
    else if (lhs->index > rhs->index)
        return 1;
    return 0;
}



struct _graph_edge * graph_edge_create (uint64_t head, uint64_t tail, void * data)
{
    struct _graph_edge * edge;
    edge = (struct _graph_edge *) malloc(sizeof(struct _graph_edge));
    if (data == NULL)
        edge->data = NULL;
    else
        edge->data = object_copy(data);
    edge->object = &graph_edge_object;
    edge->head = head;
    edge->tail = tail;
    return edge;
}



void graph_edge_delete (struct _graph_edge * edge)
{
    if (edge->data != NULL)
        object_delete(edge->data);
    free(edge);
}



struct _graph_edge * graph_edge_copy (struct _graph_edge * edge)
{
    return graph_edge_create(edge->head, edge->tail, edge->data);
}



int graph_edge_cmp (struct _graph_edge * lhs, struct _graph_edge * rhs)
{
    if ((lhs->head == rhs->head) && (lhs->tail == rhs->tail))
        return 0;
    else if (lhs->head < rhs->head)
        return -1;
    else if (lhs->head > rhs->head)
        return 1;
    else if (lhs->tail < rhs->tail)
        return -1;
    else
        return 1;
}



struct _graph_node * graph_node_create (struct _graph * graph,
                                        uint64_t        index,
                                        void *          data)
{
    struct _graph_node * node;
    node = (struct _graph_node *) malloc(sizeof(struct _graph_node));
    node->object = &graph_node_object;
    node->graph  = graph;
    node->index  = index;
    if (data == NULL)
        node->data = NULL;
    else
        node->data = object_copy(data);
    node->edges = list_create();

    return node;
}



void graph_node_delete (struct _graph_node * node)
{
    if (node->data != NULL)
        object_delete(node->data);
    object_delete(node->edges);
    free(node);
}



// removes all edges to and from this node, and then deletes
// node from the graph tree which will call graph_node_delete
/*
void graph_node_delete_tree (struct _graph_node * node)
{
    struct _graph      * graph = node->graph;
    struct _graph_node * sibling;
    struct _list_it    * sib_it;

    // remove edges to and from this node
    struct _list_it * it;
    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;

        if (edge->head == node->index) {
            sibling = graph_fetch_node(graph, edge->tail);
            for (sib_it  = list_iterator(sibling->edges);
                 sib_it != NULL;
                 sib_it  = sib_it->next) {
                struct _graph_edge * sib_edge = sib_it->data;
                if (sib_edge->head == node->index) {
                    list_remove(sibling->edges, sib_it);
                    break;
                }
            }
        }
        else {
            sibling = graph_fetch_node(graph, edge->head);
            for (sib_it  = list_iterator(sibling->edges);
                 sib_it != NULL;
                 sib_it  = sib_it->next) {
                struct _graph_edge * sib_edge = sib_it->data;
                if (sib_edge->tail == node->index) {
                    list_remove(sibling->edges, sib_it);
                    break;
                }
            }
        }
    }

    // delete node from tree, which will also delete node
    tree_delete_node(graph->nodes, node);
}
*/



struct _graph_node * graph_node_copy (struct _graph_node * node)
{
    struct _graph_node * new_node;
    new_node = graph_node_create(node->graph, node->index, node->data);
    object_delete(new_node->edges);
    new_node->edges = object_copy(node->edges);
    return new_node;
}



int graph_node_cmp (struct _graph_node * lhs, struct _graph_node * rhs)
{
    return lhs->index - rhs->index;
}



inline size_t graph_node_successors_n (struct _graph_node * node)
{
    struct _list_it * it;
    size_t successors = 0;
    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;
        if (edge->head == node->index)
            successors++;
    }
    return successors;
}


/*
*   It's very possible that a node will have two edges, both with head and tail
*   set to itself. This is actually correct, as one edge is for predecessor to
*   successor, and one is for successor to predecessor. We have to account for
*   this and insure we only return one successor node.
*/
struct _list * graph_node_successors (struct _graph_node * node)
{
    int loop_count = 0;
    struct _list_it * it;
    struct _list * successors = list_create();
    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;
        if (edge->head == node->index) {
            if (edge->head == edge->tail) {
                if (loop_count == 0)
                    loop_count = 1;
                else
                    continue;
            }
            list_append(successors, edge);
        }
    }
    return successors;
}



size_t graph_node_predecessors_n (struct _graph_node * node)
{
    struct _list_it * it;
    size_t predecessors = 0;
    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;
        if (edge->tail == node->index)
            predecessors++;
    }
    return predecessors;
}


/*
*   See note for graph_node_predecessors
*/
struct _list * graph_node_predecessors (struct _graph_node * node)
{
    int loop_count = 0;
    struct _list_it * it;
    struct _list * predecessors = list_create();
    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;
        if (edge->tail == node->index) {
            if (edge->head == edge->tail) {
                if (loop_count == 0)
                    loop_count = 1;
                else
                    continue;
            }
            list_append(predecessors, edge);
        }
    }
    return predecessors;
}



struct _graph_it * graph_iterator (struct _graph * graph)
{
    struct _graph_it * it;

    it = malloc(sizeof(struct _graph_it));

    it->it = tree_iterator(graph->nodes);
    if (it->it == NULL) {
        free(it);
        return NULL;
    }
    return it;
}


void graph_it_delete (struct _graph_it * graph_it)
{
    tree_it_delete(graph_it->it);
    free(graph_it);
}


struct _graph_it * graph_it_next (struct _graph_it * graph_it)
{
    graph_it->it = tree_it_next(graph_it->it);
    if (graph_it->it == NULL) {
        free(graph_it);
        return NULL;
    }
    return graph_it;
}


void * graph_it_data  (struct _graph_it * graph_it)
{
    struct _graph_node * node;
    node = tree_it_data(graph_it->it);
    if (node == NULL)
        return NULL;
    return node->data;
}


struct _graph_node * graph_it_node  (struct _graph_it * graph_it)
{
    struct _graph_node * node;
    node = tree_it_data(graph_it->it);
    if (node == NULL)
        return NULL;
    return node;
}


uint64_t graph_it_index (struct _graph_it * graph_it)
{
    struct _graph_node * node;
    node = tree_it_data(graph_it->it);
    if (node == NULL)
        return 0;
    return node->index;
}


struct _list * graph_it_edges (struct _graph_it * graph_it)
{
    struct _graph_node * node;
    node = tree_it_data(graph_it->it);
    if (node == NULL)
        return NULL;
    return node->edges;
}


int graph_cmp (void * a, void * b)
{
    struct _graph_node * node_a = (struct _graph_node *) a;
    struct _graph_node * node_b = (struct _graph_node *) b;
    if (node_a->index == node_b->index)
        return 0;
    else if (node_a->index < node_b->index)
        return -1;
    else
        return 1;
}
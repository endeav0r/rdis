#ifndef graph_HEADER
#define graph_HEADER

#include <inttypes.h>

#include "list.h"
#include "object.h"
#include "tree.h"

// generic function pointers that need to be implemented for graph_node->data
// delete, copy, merge

struct _graph_it {
    struct _tree_it * it;
};

struct _graph_index {
    const struct _object * object;
    uint64_t index;
};

struct _graph_edge {
    const struct _object * object;
    void * data;
    uint64_t head;
    uint64_t tail;
};

struct _graph_node {
    const struct _object * object;
    struct _graph * graph;
    uint64_t       index;
    void         * data;
    struct _list * edges;
};

struct _graph {
    const struct _object * object;
    struct _tree * nodes;
};


void graph_debug (struct _graph * graph);


/*
* GRAPH OPERATOR INSTRUCTIONS
*/
struct _graph * graph_create ();
void            graph_delete (struct _graph * graph);
int             graph_cmp    (void * a, void * b);
struct _graph * graph_copy   (struct _graph * graph);


/*
* GENERAL GRAPH METHODS
*/
// merges graph rhs into graph (lhs)
void graph_merge (struct _graph * graph, struct _graph * rhs);

// removes edges between nodes that are singly-linked and merges
// their data
void graph_reduce (struct _graph * graph);

// returns a graph that contains all reachable nodes from the given
// index
struct _graph * graph_family (struct _graph * graph, uint64_t index);

void graph_add_node (struct _graph * graph,
                     uint64_t        index,
                     void *          data);

struct _graph_node * graph_fetch_node  (struct _graph * graph,
                                        uint64_t        index);

void               * graph_fetch_data  (struct _graph * graph,
                                        uint64_t        index);

struct _list       * graph_fetch_edges (struct _graph * graph,
                                        uint64_t        index);

struct _graph_node * graph_fetch_node_max (struct _graph * graph,
                                           uint64_t        index);

// returns -1 on error, 0 on success
int graph_add_edge (struct _graph * graph,
                    uint64_t        head_needle,
                    uint64_t        tail_needle,
                    void          * data);

int graph_remove_edge (struct _graph * graph,
                       uint64_t head_needle,
                       uint64_t tail_needle);

void graph_map (struct _graph * graph, void (* callback) (struct _graph_node *));
void graph_bfs (struct _graph * graph,
                uint64_t        index,
                void  (* callback) (struct _graph *, struct _graph_node *));

/*
* GRAPH INDEX OPERATOR FUNCTIONS
*/
struct _graph_index * graph_index_create (uint64_t index);
void                  graph_index_delete (struct _graph_index * index);
struct _graph_index * graph_index_copy   (struct _graph_index * index);
int                   graph_index_cmp    (struct _graph_index * lhs,
                                          struct _graph_index * rhs);

/*
* GRAPH EDGE OPERATOR FUNCTIONS
*/
struct _graph_edge * graph_edge_create (uint64_t head,
                                        uint64_t tail,
                                        void * data);
void                 graph_edge_delete (struct _graph_edge * edge);
struct _graph_edge * graph_edge_copy   (struct _graph_edge * edge);
int                  graph_edge_cmp    (struct _graph_edge * lhs,
                                        struct _graph_edge * rhs);

/*
* GRAPH NODE OPERATOR FUNCTIONS
*/
struct _graph_node * graph_node_create (struct _graph * graph,
                                        uint64_t        index,
                                        void *          data);
void                 graph_node_delete (struct _graph_node * node);
struct _graph_node * graph_node_copy   (struct _graph_node * node);
int                  graph_node_cmp    (struct _graph_node * lhs,
                                        struct _graph_node * rhs);

//void                 graph_node_delete_tree (struct _graph_node * node);

/*
* GRAPH NODE EDGE ACCESSORS
*/
size_t         graph_node_successors_n   (struct _graph_node * node);
size_t         graph_node_predecessors_n (struct _graph_node * node);

struct _list * graph_node_successors     (struct _graph_node * node);
struct _list * graph_node_predecessors   (struct _graph_node * node);


/*
* GRAPH ITERATION
* Graph iterators are freed when graph_it_next returns free, so normally
* you do not need to free them. If you need to manually free them, call
* graph_it_delete
* DO NOT MODIFY THE GRAPH DURING ITERATION (jackass)
*/
struct _graph_it *   graph_iterator  (struct _graph * graph);
void                 graph_it_delete (struct _graph_it * graph_it);
struct _graph_it *   graph_it_next   (struct _graph_it * graph_it);
void *               graph_it_data   (struct _graph_it * graph_it);
uint64_t             graph_it_index  (struct _graph_it * graph_it);
struct _graph_node * graph_it_node   (struct _graph_it * graph_it);
struct _list *       graph_it_edges  (struct _graph_it * graph_it);


#endif
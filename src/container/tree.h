#ifndef tree_HEADER
#define tree_HEADER

#include <stdlib.h>

#include "object.h"

#define TREE_NEUTRAL 0
#define TREE_LEFT    1
#define TREE_RIGHT   2

struct _tree_it {
    struct _tree_it *   parent;
    int                 direction;
    struct _tree_node * node;
};

struct _tree_node {
    unsigned int level;
    void * data;
    struct _tree_node * left;
    struct _tree_node * right;
};

struct _tree {
    const struct _object * object;
    struct _tree_node * nodes;
};


struct _tree * tree_create      ();
void           tree_delete      (struct _tree * tree);
struct _tree * tree_copy        (struct _tree * tree);
json_t *       tree_serialize   (struct _tree * tree);
struct _tree * tree_deserialize (json_t * json);

void           tree_map (struct _tree * tree, void (* callback) (void *));

void           tree_insert      (struct _tree * tree, void * data);
void *         tree_fetch       (struct _tree * tree, void * data);
void *         tree_fetch_max   (struct _tree * tree, void * data);
void           tree_delete_node (struct _tree * tree, void * data);

struct _tree_node * tree_node_create  (void * data);
void                tree_node_map     (struct _tree_node * node,
                                       void (* callback) (void *));

struct _tree_node * tree_node_insert  (struct _tree * tree, 
                                       struct _tree_node * node,
                                       struct _tree_node * new_node);

struct _tree_node * tree_node_fetch   (struct _tree * tree,
                                       struct _tree_node * node,
                                       void * data);

struct _tree_node * tree_node_fetch_max (struct _tree * tree,
                                         struct _tree_node * node,
                                         void * data);

// deletes a node from a tree
struct _tree_node * tree_node_delete  (struct _tree * tree,
                                       struct _tree_node * node,
                                       void * data);

struct _tree_node * tree_node_skew    (struct _tree_node * node);
struct _tree_node * tree_node_split   (struct _tree_node * node);

struct _tree_it * tree_iterator   (struct _tree * tree);
struct _tree_it * tree_it_next    (struct _tree_it * tree_it);
void *            tree_it_data    (struct _tree_it * tree_it);
void              tree_it_delete  (struct _tree_it * tree_it);

#endif
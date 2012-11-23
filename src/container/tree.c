#include "tree.h"

#include "serialize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct _object tree_object = {
    (void     (*) (void *)) tree_delete, 
    (void *   (*) (void *)) tree_copy,
    NULL,
    NULL,
    (json_t * (*) (void *)) tree_serialize
};

struct _tree * tree_create ()
{
    struct _tree * tree;

    tree = (struct _tree *) malloc(sizeof(struct _tree));
    tree->object = &tree_object;
    tree->nodes = NULL;

    return tree;
}



void tree_delete_node_delete (struct _tree_node * node)
{
    if (node == NULL)
        return;
    tree_delete_node_delete(node->left);
    tree_delete_node_delete(node->right);
    object_delete(node->data);
    free(node);
}



void tree_delete (struct _tree * tree)
{
    tree_delete_node_delete(tree->nodes);
    free(tree); 
}


json_t * tree_serialize (struct _tree * tree)
{
    json_t * json = json_object();

    json_object_set(json, "ot", json_integer(SERIALIZE_TREE));

    json_t * nodes = json_array();
    struct _tree_it * it;
    for (it = tree_iterator(tree); it != NULL; it = tree_it_next(it)) {
        json_array_append(nodes, object_serialize(tree_it_data(it)));
    }

    json_object_set(json, "nodes", nodes);

    return json;
}


struct _tree * tree_deserialize (json_t * json)
{
    json_t * nodes = json_object_get(json, "nodes");
    if (! json_is_array(nodes)) {
        serialize_error = SERIALIZE_TREE;
        return NULL;
    }

    struct _tree * tree = tree_create();

    size_t i;
    for (i = 0; i < json_array_size(nodes); i++) {
        void * data = deserialize(json_array_get(nodes, i));
        if (data == NULL) {
            serialize_error = SERIALIZE_TREE;
            object_delete(tree);
            return NULL;
        }
        tree_insert(tree, data);
        object_delete(data);
    }

    return tree;
}


struct _tree * tree_copy (struct _tree * tree)
{
    struct _tree    * new_tree;
    struct _tree_it * it;

    new_tree = tree_create();
    for (it = tree_iterator(tree); it != NULL; it = tree_it_next(it)) {
        tree_insert(new_tree, tree_it_data(it));
    }

    return new_tree;
}



void tree_map (struct _tree * tree, void (* callback) (void *))
{
    tree_node_map(tree->nodes, callback);
}



void tree_insert (struct _tree * tree, void * data)
{
    struct _tree_node * node;

    node = tree_node_create(data);

    tree->nodes = tree_node_insert(tree, tree->nodes, node);
}



void * tree_fetch (struct _tree * tree, void * data)
{
    struct _tree_node * node;

    node = tree_node_fetch (tree, tree->nodes, data);
    if (node == NULL)
        return NULL;

    return node->data;
}



void * tree_fetch_max (struct _tree * tree, void * data)
{
    struct _tree_node * node;

    node = tree_node_fetch_max (tree, tree->nodes, data);
    if (node == NULL)
        return NULL;

    return node->data;
}


void tree_remove (struct _tree * tree, void * data)
{
    tree->nodes = tree_node_delete(tree, tree->nodes, data);
}



struct _tree_node * tree_node_create (void * data)
{
    struct _tree_node * node;

    node = (struct _tree_node *) malloc(sizeof(struct _tree_node));
    node->data = object_copy(data);

    node->level     = 0;
    node->left      = NULL;
    node->right     = NULL;

    return node;
}



void tree_node_map (struct _tree_node * node, void (* callback) (void *))
{
    if (node == NULL)
        return;
    callback(node->data);
    tree_node_map(node->left,  callback);
    tree_node_map(node->right, callback);
}



struct _tree_node * tree_node_insert (struct _tree * tree,
                                      struct _tree_node * node,
                                      struct _tree_node * new_node)
{
    if (node == NULL)
        return new_node;
    else if (object_cmp(new_node->data, node->data) < 0)
        node->left  = tree_node_insert(tree, node->left,  new_node);
    else
        node->right = tree_node_insert(tree, node->right, new_node);

    node = tree_node_skew (node);
    node = tree_node_split(node);

    return node;
}



struct _tree_node * tree_node_fetch   (struct _tree * tree,
                                       struct _tree_node * node,
                                       void * data)
{
    if (node == NULL)
        return NULL;
    if (object_cmp(data, node->data) == 0)
        return node;
    else if (object_cmp(data, node->data) < 0)
        return tree_node_fetch(tree, node->left, data);
    else
        return tree_node_fetch(tree, node->right, data);
}



struct _tree_node * tree_node_fetch_max (struct _tree * tree,
                                         struct _tree_node * node,
                                         void * data)
{
    struct _tree_node * found;

    if (node == NULL)
        return NULL;
    if (object_cmp(data, node->data) == 0)
        return node;
    else if (object_cmp(data, node->data) < 0)
        found = tree_node_fetch_max(tree, node->left, data);
    else
        found = tree_node_fetch_max(tree, node->right, data);

    if ((found == NULL) && (object_cmp(data, node->data) > 0))
        return node;
    return found;
}



struct _tree_node * tree_node_predecessor (struct _tree_node * node)
{
    if (node->left == NULL)
        return node;
    node = node->left;
    while (node->right != NULL)
        node = node->right;
    return node;
}



struct _tree_node * tree_node_successor (struct _tree_node * node)
{
    if (node->right == NULL)
        return node;
    node = node->right;
    while (node->left != NULL)
        node = node->left;
    return node;
}



struct _tree_node * tree_node_decrease_level (struct _tree_node * node)
{
    unsigned int should_be;

    if ((node->left == NULL) || (node->right == NULL))
        return node;

    should_be =   node->left->level < node->right->level 
                ? node->left->level : node->right->level;
    should_be++;

    if (should_be < node->level) {
        node->level = should_be;
        if (should_be < node->right->level)
            node->right->level = should_be;
    }

    return node;
}



struct _tree_node * tree_node_delete (struct _tree * tree,
                                      struct _tree_node * node,
                                      void * data)
{
    struct _tree_node * tmp;

    if (node == NULL)
        return node;
    if (object_cmp(data, node->data) < 0)
        node->left = tree_node_delete(tree, node->left, data);
    else if (object_cmp(data, node->data) > 0)
        node->right = tree_node_delete(tree, node->right, data);
    else {
        if ((node->left == NULL) && (node->right == NULL)) {
            tree_delete_node_delete(node);
            return NULL;
        }
        else if (node->left == NULL) {
            tmp = tree_node_successor(node);
            object_delete(node->data);
            node->data = object_copy(tmp->data);
            node->right = tree_node_delete(tree, node->right, tmp->data);
        }
        else {
            tmp = tree_node_predecessor(node);
            object_delete(node->data);
            node->data = object_copy(tmp->data);
            node->left = tree_node_delete(tree, node->left, tmp->data);
        }
    }

    node = tree_node_decrease_level(node);
    node = tree_node_skew(node);
    if (node->right != NULL) {
        node->right = tree_node_skew(node->right);
        if (node->right->right != NULL)
            node->right->right = tree_node_skew(node->right->right);
    }
    node = tree_node_split(node);
    if (node->right != NULL)
        node->right = tree_node_split(node->right);

    return node;
}



struct _tree_node * tree_node_skew (struct _tree_node * node)
{
    struct _tree_node * L;
    if ((node == NULL) || (node->left == NULL))
        return node;
    if (node->level == node->left->level) {
        L = node->left;
        node->left = L->right;
        L->right = node;
        return L;
    }
    return node;
}



struct _tree_node * tree_node_split (struct _tree_node * node)
{
    struct _tree_node * R;
    if ((node == NULL) || (node->right == NULL) || (node->right->right == NULL))
        return node;
    if (node->level == node->right->right->level) {
        R = node->right;
        node->right = R->left;
        R->left = node;
        R->level++;
        return R;
    }
    return node;
}



struct _tree_it * tree_it_create (struct _tree_node * node)
{
    struct _tree_it * tree_it;

    tree_it = (struct _tree_it *) malloc(sizeof(struct _tree_it));
    tree_it->direction = TREE_LEFT;
    tree_it->node      = node;
    tree_it->parent    = NULL;

    return tree_it;
}



// creates _tree_its all the way down the left side of this tree
struct _tree_it * tree_it_left (struct _tree_it * top)
{
    struct _tree_it * parent = top;
    struct _tree_it * it = top;
    while (parent->node->left != NULL) {
        it = tree_it_create(parent->node->left);
        it->parent = parent;
        parent = it;
    }

    it->direction = TREE_NEUTRAL;
    
    return it;
}


struct _tree_it * tree_iterator (struct _tree * tree)
{
    struct _tree_it * top;

    if (tree->nodes == NULL)
        return NULL;

    top = tree_it_create(tree->nodes);
    return tree_it_left(top);
}


struct _tree_it * tree_it_next (struct _tree_it * tree_it)
{
    struct _tree_it * it;
    if (tree_it->direction == TREE_LEFT) {
        tree_it->direction = TREE_NEUTRAL;
        return tree_it;
    }
    else if (    (tree_it->direction == TREE_NEUTRAL)
              && (tree_it->node->right != NULL)) {
        tree_it->direction = TREE_RIGHT;
        it = tree_it_create(tree_it->node->right);
        it->parent = tree_it;
        return tree_it_left(it);
    }
    else {
        while (tree_it != NULL) {
            it = tree_it->parent;
            free(tree_it);
            if (it == NULL)
                return NULL;
            if (it->direction == TREE_LEFT)
                break;
            tree_it = it;
        }

        it->direction = TREE_NEUTRAL;
        return it;
    }
}


void * tree_it_data (struct _tree_it * tree_it)
{
    return tree_it->node->data;
}


void tree_it_delete (struct _tree_it * tree_it)
{
    if (tree_it != NULL) {
        tree_it_delete(tree_it->parent);
        free(tree_it);
    }
}
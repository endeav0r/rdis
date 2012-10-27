#include "map.h"

#include <stdio.h>
#include <stdlib.h>

static const struct _object map_object = {
    (void   (*) (void *))         map_delete, 
    (void * (*) (void *))         map_copy,
    NULL,
    NULL
};

static const struct _object map_node_object = {
    (void   (*) (void *))         map_node_delete, 
    (void * (*) (void *))         map_node_copy,
    (int    (*) (void *, void *)) map_node_cmp,
    NULL
};


int map_insert (struct _map * map, uint64_t key, void * value)
{
    int result = 0;

    struct _map_node * map_node = map_node_create(key, value);

    if (tree_fetch(map->tree, map_node) == NULL) {
        tree_insert(map->tree, map_node);
        map->size++;
    }
    else
        result = -1;

    object_delete(map_node);

    return result;
}



void * map_fetch (struct _map * map, uint64_t key)
{
    struct _map_node * needle = map_node_create(key, NULL);
    struct _map_node * map_node;

    map_node = tree_fetch(map->tree, needle);

    object_delete(needle);

    if (map_node == NULL)
        return NULL;
    else
        return map_node->value;
}



void * map_fetch_max (struct _map * map, uint64_t key)
{
    struct _map_node * needle = map_node_create(key, NULL);
    struct _map_node * map_node;

    map_node = tree_fetch_max(map->tree, needle);

    object_delete(needle);

    if (map_node == NULL)
        return NULL;
    else
        return map_node->value;
}



uint64_t map_fetch_max_key (struct _map * map, uint64_t key)
{
    struct _map_node * needle = map_node_create(key, NULL);
    struct _map_node * map_node;

    map_node = tree_fetch_max(map->tree, needle);

    object_delete(needle);

    if (map_node == NULL)
        return -1;
    else
        return map_node->key;
}



int map_remove (struct _map * map, uint64_t key)
{
    int result = 0;

    struct _map_node * needle = map_node_create(key, NULL);

    if (tree_fetch(map->tree, needle) == NULL)
        result = -1;
    else {
        tree_delete_node(map->tree, needle);
        map->size--;
    }

    object_delete(needle);
    return result;
}



struct _map * map_create ()
{
    struct _map * map;

    map = (struct _map *) malloc(sizeof(struct _map));
    map->object = &map_object;
    map->tree = tree_create();
    map->size = 0;

    return map;
}


void map_delete (struct _map * map)
{
    object_delete(map->tree);
    free(map);
}


struct _map * map_copy (struct _map * map)
{
    struct _map * new_map = map_create();
    object_delete(new_map->tree);
    new_map->tree = object_copy(map->tree);
    new_map->size = map->size;

    return new_map;
}


struct _map_node * map_node_create (uint64_t key, void * value)
{
    struct _map_node * map_node;

    map_node = (struct _map_node *) malloc(sizeof(struct _map_node));
    map_node->object = &map_node_object;
    map_node->key    = key;
    if (value == NULL)
        map_node->value = NULL;
    else
        map_node->value = object_copy(value);

    return map_node;
}


void map_node_delete (struct _map_node * map_node)
{
    if (map_node->value != NULL)
        object_delete(map_node->value);
    free(map_node);
}


struct _map_node * map_node_copy (struct _map_node * map_node)
{
    return map_node_create(map_node->key, map_node->value);
}


int map_node_cmp (struct _map_node * lhs, struct _map_node * rhs)
{
    if (lhs->key < rhs->key)
        return -1;
    else if (lhs->key > rhs->key)
        return 1;
    return 0;
}


struct _map_it * map_iterator (struct _map * map)
{
    struct _map_it * map_it;

    map_it = (struct _map_it *) malloc(sizeof(struct _map_it));

    map_it->it = tree_iterator(map->tree);

    if (map_it->it == NULL) {
        free(map_it);
        return NULL;
    }

    return map_it;
}


struct _map_it * map_it_next (struct _map_it * map_it)
{
    map_it->it = tree_it_next(map_it->it);

    if (map_it->it == NULL) {
        free(map_it);
        return NULL;
    }

    return map_it;
}


void * map_it_data (struct _map_it * map_it)
{
    struct _map_node * map_node = tree_it_data(map_it->it);

    if (map_node == NULL)
        return NULL;

    return map_node->value;
}


uint64_t map_it_key (struct _map_it * map_it)
{
    struct _map_node * map_node = tree_it_data(map_it->it);

    if (map_node == NULL)
        return -1;

    return map_node->key;
}


void map_it_delete (struct _map_it * map_it)
{
    tree_it_delete(map_it->it);
    free(map_it);
}
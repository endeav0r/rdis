#ifndef map_HEADER
#define map_HEADER

// a 1-to-1 mapping of uint64_t to values

#include <inttypes.h>

#include "object.h"
#include "tree.h"

struct _map_node {
    const struct _object * object;
    uint64_t key;
    void   * value;
};


struct _map {
    const struct _object * object;
    size_t size;
    struct _tree * tree;
};


struct _map_it {
    struct _tree_it * it;
};


// returns 0 on success, -1 on error (key already exists)
int      map_insert        (struct _map *, uint64_t key, void * value);
void *   map_fetch         (struct _map *, uint64_t key);
void *   map_fetch_max     (struct _map *, uint64_t key);
uint64_t map_fetch_max_key (struct _map *, uint64_t key);
int      map_remove        (struct _map *, uint64_t key);


struct _map * map_create      ();
void          map_delete      (struct _map *);
struct _map * map_copy        (struct _map *);
json_t      * map_serialize   (struct _map *);
struct _map * map_deserialize (json_t * json);

struct _map_node * map_node_create      (uint64_t key, void * value);
void               map_node_delete      (struct _map_node *);
struct _map_node * map_node_copy        (struct _map_node *);
int                map_node_cmp         (struct _map_node *, struct _map_node *);
json_t *           map_node_serialize   (struct _map_node * map_node);
struct _map_node * map_node_deserialize (json_t * json);

struct _map_it * map_iterator  (struct _map * map);
struct _map_it * map_it_next   (struct _map_it * map_it);
void *           map_it_data   (struct _map_it * map_it);
uint64_t         map_it_key    (struct _map_it * map_it);
void             map_it_delete (struct _map_it * map_it);


#endif
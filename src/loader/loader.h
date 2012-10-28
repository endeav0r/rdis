#ifndef loader_HEADER
#define loader_HEADER

#include "graph.h"
#include "map.h"
#include "object.h"
#include "tree.h"

#include <inttypes.h>

struct _loader_object_ptr {
    struct _loader_object * loader_object;
};

struct _loader_object {
    const struct _object object;
    uint64_t        (* entry)                 (void *);
    struct _graph * (* graph)                 (void *);
    struct _tree  * (* function_tree)         (void *);
    struct _map   * (* labels)                (void *);
    struct _graph * (* graph_address)         (void *, uint64_t);
    struct _map   * (* memory_map)            (void *);
    struct _tree  * (* function_tree_address) (void *, uint64_t);
    struct _label * (* label_address)         (void *, uint64_t);
};


typedef void * _loader;


_loader * loader_create (const char * filename);

uint64_t        loader_entry                 (_loader *);
struct _graph * loader_graph                 (_loader *);
struct _tree  * loader_function_tree         (_loader *);
struct _map   * loader_labels                (_loader *);
struct _graph * loader_graph_address         (_loader *, uint64_t);
struct _map   * loader_memory_map            (_loader *);
struct _tree *  loader_function_tree_address (_loader *, uint64_t);
struct _label * loader_label_address         (_loader *, uint64_t);

#endif
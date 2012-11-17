#ifndef loader_HEADER
#define loader_HEADER

#include "function.h"
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
    uint64_t        (* entry)            (void *);
    struct _graph * (* graph)            (void *);
    struct _map   * (* functions)        (void *);
    struct _map   * (* labels)           (void *);
    struct _graph * (* graph_address)    (void *, uint64_t);
    struct _map   * (* memory_map)       (void *);
    struct _map   * (* function_address) (void *, uint64_t);
    struct _label * (* label_address)    (void *, uint64_t);
    struct _graph * (* graph_functions)  (void *, struct _map *);
    struct _map   * (* labels_functions) (void *, struct _map *);
};


typedef void * _loader;


_loader * loader_create (const char * filename);

uint64_t        loader_entry            (_loader *);
struct _graph * loader_graph            (_loader *);
struct _map  *  loader_functions        (_loader *);
struct _map *   loader_labels           (_loader *);
struct _graph * loader_graph_address    (_loader *, uint64_t);
struct _map *   loader_memory_map       (_loader *);
struct _map  *  loader_function_address (_loader *, uint64_t);
struct _label * loader_label_address    (_loader *, uint64_t);
struct _graph * loader_graph_functions  (_loader *, struct _map *);
struct _map *   loader_labels_functions (_loader *, struct _map *);

#endif
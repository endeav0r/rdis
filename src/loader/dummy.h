#ifndef dummy_HEADER
#define dummy_HEADER

#include "rdis_lua.h"
#include "serialize.h"

struct _dummy_loader {
    const struct _loader_object * loader_object;
};

// a lua loader should be at the top of the stack
struct _dummy_loader * dummy_loader_create      ();
void                   dummy_loader_delete      (struct _dummy_loader *);
json_t *               dummy_loader_serialize   (struct _dummy_loader *);

uint64_t        dummy_loader_entry            (struct _dummy_loader *);
struct _graph * dummy_loader_graph            (struct _dummy_loader *, struct _map *);
struct _map   * dummy_loader_functions        (struct _dummy_loader *, struct _map *);
struct _map   * dummy_loader_labels           (struct _dummy_loader *, struct _map *);
struct _graph * dummy_loader_graph_address    (struct _dummy_loader *, struct _map *, uint64_t);
struct _map   * dummy_loader_memory_map       (struct _dummy_loader *);
struct _label * dummy_loader_label_address    (struct _dummy_loader *, struct _map *, uint64_t);
struct _map   * dummy_loader_function_address (struct _dummy_loader *, struct _map *, uint64_t);
struct _graph * dummy_loader_graph_functions  (struct _dummy_loader *, struct _map *, struct _map *);
struct _map   * dummy_loader_labels_functions (struct _dummy_loader *, struct _map *, struct _map *);

#endif
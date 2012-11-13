#ifndef lua_HEADER
#define lua_HEADER

#include "rdis_lua.h"
#include "serialize.h"

struct _lua_loader {
    const struct _loader_object * loader_object;

    int                loader_index;
    struct _rdis_lua * rdis_lua;
};

// a lua loader should be at the top of the stack
struct _lua_loader * lua_loader_create      (lua_State * L);
void                 lua_loader_delete      (struct _lua_loader * lua_loader);
json_t *             lua_loader_serialize   (struct _lua_loader * lua_loader);
struct _lua_loader * lua_loader_deserialize (json_t * json);

uint64_t        lua_loader_entry         (struct _lua_loader *);
struct _graph * lua_loader_graph         (struct _lua_loader *);
struct _tree  * lua_loader_function_tree (struct _lua_loader *);
struct _map   * lua_loader_labels        (struct _lua_loader *);
struct _graph * lua_loader_graph_address (struct _lua_loader *, uint64_t);
struct _map   * lua_loader_memory_map    (struct _lua_loader *);
struct _label * lua_loader_label_address (struct _lua_loader *, uint64_t);
struct _tree  * lua_loader_function_tree_address (struct _lua_loader *, uint64_t);

#endif
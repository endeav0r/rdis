#include "lua.h"

#include "buffer.h"
#include "function.h"
#include "index.h"
#include "label.h"
#include "map.h"
#include "queue.h"
#include "util.h"

#include <stdio.h>

static int lua_loader_index = 0;

static const struct _loader_object lua_loader_object = {
    {
        (void     (*) (void *)) lua_loader_delete, 
        NULL,
        NULL,
        NULL,
        (json_t * (*) (void *)) lua_loader_serialize
    },
    (uint64_t        (*) (void *))                lua_loader_entry,
    (struct _graph * (*) (void *))                lua_loader_graph,
    (struct _map  *  (*) (void *))                lua_loader_functions,
    (struct _map  *  (*) (void *))                lua_loader_labels,
    (struct _graph * (*) (void *, uint64_t))      lua_loader_graph_address,
    (struct _map *   (*) (void *))                lua_loader_memory_map,
    (struct _map  *  (*) (void *, uint64_t))      lua_loader_function_address,
    (struct _label * (*) (void *, uint64_t))      lua_loader_label_address,
    (struct _graph * (*) (void *, struct _map *)) lua_loader_graph_functions,
    (struct _map *   (*) (void *, struct _map *)) lua_loader_labels_functions
};


const char * lua_loader_index_name (int index)
{
    static char name[32];
    snprintf(name, 32, "ll_%d", index);
    return name;
}


struct _lua_loader * lua_loader_create (lua_State * L)
{
    // sanity checks on lua loader table
    if (! lua_istable(L, -1)) {
        luaL_error(L, "expected table");
    }

    lua_pushstring(L, "ins");
    lua_gettable(L, -2);
    if (! lua_isfunction(L, -1)) {
        luaL_error(L, "loader object requires ins method");
        return NULL;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "functions");
    lua_gettable(L, -2);
    if (! lua_isfunction(L, -1)) {
        luaL_error(L, "loader object requires functions method");
        return NULL;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "function_address");
    lua_gettable(L, -2);
    if (! lua_isfunction(L, -1)) {
        luaL_error(L, "loader object requires function_address method");
        return NULL;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "label");
    lua_gettable(L, -2);
    if (! lua_isfunction(L, -1)) {
        luaL_error(L, "loader object requires label method");
        return NULL;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "memory_map");
    lua_gettable(L, -2);
    if (! lua_isfunction(L, -1)) {
        luaL_error(L, "loader object requires label method");
        return NULL;
    }
    lua_pop(L, 1);

    struct _lua_loader * ll;
    ll = (struct _lua_loader *) malloc(sizeof(struct _lua_loader));

    ll->loader_object = &lua_loader_object;
    ll->loader_index = lua_loader_index++;

    // store the loader in the LUA_REGISTRY
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);
    lua_setfield(L, LUA_REGISTRYINDEX, ll_index_name);

    ll->rdis_lua = rl_get_rdis_lua(L);

    return ll;
}


void lua_loader_delete (struct _lua_loader * ll)
{
    lua_State * L = ll->rdis_lua->L;

    // remove loader from lua environment
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, ll_index_name);

    free(ll);
}


json_t * lua_loader_serialize (struct _lua_loader * lua_loader)
{
    return NULL;
}


struct _lua_loader * lua_loader_deserialize (json_t * json)
{
    return NULL;
}


uint64_t lua_loader_entry (struct _lua_loader * ll)
{
    return 0;
}


struct _graph * lua_loader_graph (struct _lua_loader * ll)
{
    struct _map * functions = lua_loader_functions(ll);
    if (functions == NULL)
        return NULL;

    struct _graph * graph = lua_loader_graph_functions(ll, functions);

    object_delete(functions);

    return graph;
}


// the result to a call to funtions() or function_address() should be at
// the top of the stack. returns a function tree, or NULL on error. leaves
// the result at the top of the stack
struct _map * lua_loader_functions_table (lua_State * L)
{
    // we should have a table on the top of the stack
    if (lua_istable(L, -1) == 0) {
        luaL_error(L, "expected table of function addresses");
        lua_pop(L, 2);
        return NULL;
    }

    struct _map * functions = map_create();

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        uint64_t address = rl_check_uint64(L, -1);
        printf("lua_loader_functions: %llx\n", (unsigned long long) address);

        struct _function * function = function_create(address);
        if (map_fetch(functions, address) == 0)
            map_insert(functions, address, function);
        object_delete(function);

        lua_pop(L, 1);
    }

    return functions;
}


struct _map * lua_loader_functions (struct _lua_loader * ll)
{
    lua_State * L = ll->rdis_lua->L;

    // get the lua loader object
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);
    lua_getfield(L, LUA_REGISTRYINDEX, ll_index_name);

    // get the functions function
    lua_pushstring(L, "functions");
    lua_gettable(L, -2);

    // functions(loader)
    lua_getfield(L, LUA_REGISTRYINDEX, ll_index_name);

    // make the call
    if (lua_pcall(L, 1, 1, 0) != 0) {
        const char * error_msg = luaL_checkstring(L, -1);
        rdis_console(ll->rdis_lua->rdis, error_msg);
        lua_pop(L, 1);
        return NULL;
    }

    struct _map * map = lua_loader_functions_table(L);

    // pop the functions table and the lua loader object
    lua_pop(L, 2);

    return map;
}


struct _map * lua_loader_labels (struct _lua_loader * ll)
{
    struct _map * functions = lua_loader_functions(ll);
    if (functions == NULL)
        return NULL;

    struct _map * labels = lua_loader_labels_functions(ll, functions);

    object_delete(functions);

    return labels;
}


// there should be an ins table at the top of the stack
// will leave ins table on top of stack and returns an ins object,
// or NULL on error
struct _ins * lua_loader_ins (lua_State * L)
{
    uint64_t     address;
    uint8_t    * bytes;
    size_t       size;
    const char * description;
    const char * comment = NULL;

    if (lua_istable(L, -1) == 0)
        return NULL;

    lua_pushstring(L, "address");
    lua_gettable(L, -2);
    address = rl_check_uint64(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "bytes");
    lua_gettable(L, -2);
    if (lua_istable(L, -1) == 0) {
        luaL_error(L, "expected table for bytes");
        return NULL;
    }

    size  = lua_objlen(L, -1);
    bytes = (uint8_t *) malloc(size);
    size_t i;
    for (i = 1; i <= size; i++) {
        lua_rawgeti(L, -1, i);
        if (lua_isnumber(L, -1) == 0) {
            luaL_error(L, "bytes must be table of numbers");
            free(bytes);
            return NULL;
        }
        int b = luaL_checkinteger(L, -1);
        bytes[i] = (uint8_t) b;
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_pushstring(L, "description");
    lua_gettable(L, -2);
    if (lua_isstring(L, -1) == 0) {
        luaL_error(L, "description must be string");
        free(bytes);
        return NULL;
    }
    description = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    struct _ins * ins = ins_create(address, bytes, size, description, comment);

    lua_pushstring(L, "target");
    lua_gettable(L, -2);
    if (lua_isnil(L, -1) == 0) {
        uint64_t target = rl_check_uint64(L, -1);
        ins_s_target(ins, target);
    }
    lua_pop(L, 1);

    lua_pushstring(L, "call");
    lua_gettable(L, -2);
    if (lua_toboolean(L, -1)) {
        ins_s_call(ins);
    }
    lua_pop(L, 1);

    return ins;
}


struct _graph * lua_loader_graph_address (struct _lua_loader * ll, uint64_t addr)
{
    lua_State     * L          = ll->rdis_lua->L;
    struct _queue * queue      = queue_create();
    struct _queue * edge_queue = queue_create();
    struct _graph * graph      = graph_create();
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);
    int error                  = 0;

    struct _index * index = index_create(addr);
    queue_push(queue, index);
    object_delete(index);

    while (queue->size > 0) {
        struct _index * index = queue_peek(queue);
        if (graph_fetch_node(graph, index->index) != NULL) {
            queue_pop(queue);
            continue;
        }

        // make lua loader ins call
        lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
        lua_pushstring(L, "ins");
        lua_gettable  (L, -2);
        lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
        rl_uint64_push(L, index->index);
        if (lua_pcall(L, 2, 1, 0) != 0) {
            error = 1;
            break;
        }

        // stack is: result, lua loader object
        if (lua_istable(L, -1) == 0) {
            error = 1;
            break;
        }

        // create the instruction
        struct _ins * ins = lua_loader_ins(L);
        if (ins == NULL) {
            error = 1;
            break;
        }

        // create the graph node
        struct _list * ins_list = list_create();
        list_append(ins_list, ins);
        graph_add_node(graph, ins->address, ins_list);
        object_delete(ins_list);

        // get the successors
        lua_pushstring(L, "successors");
        lua_gettable(L, -2);
        if (lua_istable(L, -1)) {
            int n = lua_objlen(L, -1);
            int i;
            for (i = 1; i <= n; i++) {
                lua_rawgeti(L, -1, i);
                uint64_t successor = rl_check_uint64(L, -1);

                struct _index * sindex = index_create(successor);
                queue_push(queue, sindex);
                object_delete(sindex);

                struct _ins_edge * ins_edge = ins_edge_create(INS_EDGE_NORMAL);
                struct _graph_edge * edge;
                edge = graph_edge_create(ins->address, successor, ins_edge);
                queue_push(edge_queue, edge);
                objects_delete(edge, ins_edge, NULL);

                lua_pop(L, 1);
            }
        }
        else
            printf("no successors\n");
        lua_pop(L, 1);

        object_delete(ins);

        lua_pop(L, 2);

        queue_pop(queue);
    }
    object_delete(queue);

    if (error) {
        const char * error_msg = luaL_checkstring(L, -1);
        rdis_console(ll->rdis_lua->rdis, error_msg);

        object_delete(edge_queue);
        object_delete(graph);
        return NULL;
    }

    while (edge_queue->size > 0) {
        struct _graph_edge * edge = queue_peek(edge_queue);
        printf("adding edge %llx -> %llx\n",
               (unsigned long long) edge->head,
               (unsigned long long) edge->tail);
        graph_add_edge(graph, edge->head, edge->tail, edge->data);
        queue_pop(edge_queue);
    }

    object_delete(edge_queue);

    return graph;
}


struct _map * lua_loader_memory_map (struct _lua_loader * ll)
{
    lua_State     * L          = ll->rdis_lua->L;
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);

    // make lua loader ins call
    lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
    lua_pushstring(L, "memory_map");
    lua_gettable  (L, -2);
    lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
    if (lua_pcall(L, 1, 1, 0) != 0) {
        const char * error_msg = luaL_checkstring(L, -1);
        rdis_console(ll->rdis_lua->rdis, error_msg);
        return NULL;
    }

    // stack should be table, lua loader object
    if (lua_istable(L, -1) == 0) {
        luaL_error(L, "expected a table from memory_map");
        lua_pop(L, 2);
        return NULL;
    }

    // iterate over the table to fill the memory map
    struct _map * memory_map = map_create();

    // iterate over the table we got back from lua
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_istable(L, -1) == 0) {
            luaL_error(L, "memory_map returns a table of uint64={integer}");
        }
        // key at -2, value at -1
        uint64_t base_address = rl_check_uint64(L, -2);
        size_t size = lua_objlen(L, -1);
        uint8_t * tmpbuf = (uint8_t *) malloc(size);

        size_t i;
        for (i = 1; i <= size; i++) {
            lua_rawgeti(L, -1, i);
            uint8_t b = luaL_checkinteger(L, -1);
            tmpbuf[i - 1] = b;
            lua_pop(L, 1);
        }

        struct _buffer * buffer = buffer_create(tmpbuf, size);
        free(tmpbuf);
        map_insert(memory_map, base_address, buffer);
        object_delete(buffer);
        // pop value
        lua_pop(L, 1);
    }

    lua_pop(L, 2);

    return memory_map;
}


struct _label * lua_loader_label_address (struct _lua_loader * ll, uint64_t addr)
{
    lua_State     * L          = ll->rdis_lua->L;
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);

    // make lua loader label call
    lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
    lua_pushstring(L, "label");
    lua_gettable  (L, -2);
    lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
    rl_uint64_push(L, addr);

    if (lua_pcall(L, 2, 1, 0) != 0) {
        const char * error_msg = luaL_checkstring(L, -1);
        rdis_console(ll->rdis_lua->rdis, error_msg);
        lua_pop(L, 1);
        return NULL;
    }

    // stack is string, lua loader object
    const char * text = luaL_checkstring(L, -1);
    struct _label * label = label_create(addr, text, LABEL_FUNCTION);

    lua_pop(L, 2);

    return label;
}


struct _map * lua_loader_function_address (struct _lua_loader * ll,
                                                 uint64_t addr)
{
    lua_State     * L          = ll->rdis_lua->L;
    const char * ll_index_name = lua_loader_index_name(ll->loader_index);

    // make lua loader label call
    lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
    lua_pushstring(L, "function_address");
    lua_gettable  (L, -2);
    lua_getfield  (L, LUA_REGISTRYINDEX, ll_index_name);
    rl_uint64_push(L, addr);

    if (lua_pcall(L, 2, 1, 0) != 0) {
        const char * error_msg = luaL_checkstring(L, -1);
        rdis_console(ll->rdis_lua->rdis, error_msg);
        lua_pop(L, 1);
        return NULL;
    }

    struct _map * functions = lua_loader_functions_table(L);
    
    // pop the functions table and the lua loader object
    lua_pop(L, 2);

    return functions;
}


struct _graph * lua_loader_graph_functions (struct _lua_loader * ll, struct _map * functions)
{
    if (functions == NULL)
        return NULL;

    struct _graph * graph = graph_create();

    struct _map_it * it;
    for (it = map_iterator(functions); it != NULL; it = map_it_next(it)) {
        struct _function * function = map_it_data(it);
        struct _graph * func_graph  = lua_loader_graph_address(ll, function->address);
        if (func_graph == NULL) {
            objects_delete(functions, graph, NULL);
            return NULL;
        }
        graph_merge(graph, func_graph);
        object_delete(func_graph);
    }

    graph_reduce(graph);
    remove_function_predecessors(graph, functions);

    return graph;
}


struct _map * lua_loader_labels_functions (struct _lua_loader * ll,
                                           struct _map * functions)
{
    if (functions == NULL)
        return NULL;

    struct _map * labels = map_create();
    struct _map_it * it;
    for (it = map_iterator(functions); it != NULL; it = map_it_next(it)) {
        struct _function * function = map_it_data(it);
        struct _label * label = lua_loader_label_address(ll, function->address);
        if (label == NULL) {
            objects_delete(functions, labels, NULL);
            return NULL;
        }
        map_insert(labels, function->address, label);
        object_delete(label);
    }

    return labels;
}
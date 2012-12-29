#include "rdis_lua.h"

#include "buffer.h"
#include "index.h"
#include "label.h"
#include "lua.h"
#include "map.h"
#include "tree.h"

#include <stdlib.h>

static const struct luaL_Reg rl_uint64_lib_m [] = {
    {"__add", rl_uint64_add},
    {"__sub", rl_uint64_sub},
    {"__mul", rl_uint64_mul},
    {"__div", rl_uint64_div},
    {"__mod", rl_uint64_mod},
    {"__eq",  rl_uint64_eq},
    {"__lt",  rl_uint64_lt},
    {"__le",  rl_uint64_le},
    {"__tostring", rl_uint64_tostring},
    {"string", rl_uint64_tostring},
    {"number", rl_uint64_tostring},
    {NULL, NULL}
};


static const struct luaL_Reg rl_ins_lib_m [] = {
    {"__gc",        rl_ins_gc},
    {"address",     rl_ins_address},
    {"target",      rl_ins_target},
    {"bytes",       rl_ins_bytes},
    {"description", rl_ins_description},
    {"comment",     rl_ins_comment},
    {NULL, NULL}
};


static const struct luaL_Reg rl_graph_edge_lib_m [] = {
    {"__gc",    rl_graph_edge_gc},
    {"head",    rl_graph_edge_head},
    {"tail",    rl_graph_edge_tail},
    {NULL, NULL}
};


static const struct luaL_Reg rl_graph_node_lib_m [] = {
    {"__gc",         rl_graph_node_gc},
    {"index",        rl_graph_node_index},
    {"edges",        rl_graph_node_edges},
    {"instructions", rl_graph_node_instructions},
    {NULL, NULL}
};


static const struct luaL_Reg rl_rdis_lib_f [] = {
    {"console",   rl_rdis_console},
    {"functions", rl_rdis_functions},
    {"peek",      rl_rdis_peek},
    {"poke",      rl_rdis_poke},
    {"node",      rl_rdis_node},
    {"load",      rl_rdis_load},
    {"loader",    rl_rdis_loader},
    {NULL, NULL}
};


struct _rdis_lua * rdis_lua_create (struct _rdis * rdis)
{
    struct _rdis_lua * rdis_lua;
    rdis_lua = (struct _rdis_lua *) malloc(sizeof(struct _rdis_lua));

    rdis_lua->rdis = rdis;
    rdis_lua->L = lua_open();
    luaL_openlibs(rdis_lua->L);

    luaL_newmetatable(rdis_lua->L, "rdis.uint64");
    luaL_register(rdis_lua->L, NULL, rl_uint64_lib_m);
    lua_pushstring(rdis_lua->L, "__index");
    lua_pushvalue(rdis_lua->L, -2);
    lua_settable(rdis_lua->L, -3);

    lua_register(rdis_lua->L, "uint64", rl_uint64);

    luaL_newmetatable(rdis_lua->L, "rdis.ins");
    luaL_register(rdis_lua->L, NULL, rl_ins_lib_m);
    lua_pushstring(rdis_lua->L, "__index");
    lua_pushvalue(rdis_lua->L, -2);
    lua_settable(rdis_lua->L, -3);

    luaL_newmetatable(rdis_lua->L, "rdis.graph_edge");
    luaL_register(rdis_lua->L, NULL, rl_graph_edge_lib_m);
    lua_pushstring(rdis_lua->L, "__index");
    lua_pushvalue(rdis_lua->L, -2);
    lua_settable(rdis_lua->L, -3);

    luaL_newmetatable(rdis_lua->L, "rdis.graph_node");
    luaL_register(rdis_lua->L, NULL, rl_graph_node_lib_m);
    lua_pushstring(rdis_lua->L, "__index");
    lua_pushvalue(rdis_lua->L, -2);
    lua_settable(rdis_lua->L, -3);

    lua_pushlightuserdata(rdis_lua->L, rdis_lua);
    lua_setfield(rdis_lua->L, LUA_REGISTRYINDEX, "rdis_lua");

    luaL_register(rdis_lua->L, "rdis", rl_rdis_lib_f);

    char config_filename[512];
    snprintf(config_filename, 512, "%s/.rdis.lua", getenv("HOME"));

    FILE * fh = fopen(config_filename, "r");
    if (fh != NULL) {
        rdis_lua_dofile(rdis_lua, config_filename);
        fclose(fh);
    }
    else {
        char errormsg[600];
        snprintf(errormsg, 600, "config file (%s) not found", config_filename);
        rdis_console(rdis, errormsg);
    }


    return rdis_lua;
}


void rdis_lua_delete (struct _rdis_lua * rdis_lua)
{
    lua_close(rdis_lua->L);
    free(rdis_lua);
}


int rdis_lua_execute (struct _rdis_lua * rdis_lua, const char * string)
{
    int error = luaL_dostring(rdis_lua->L, string);
    if (error) {
        const char * error_msg = luaL_checkstring(rdis_lua->L, -1);
        rdis_console(rdis_lua->rdis, error_msg);
        return -1;
    }
    return error;
}


int rdis_lua_dofile (struct _rdis_lua * rdis_lua, const char * filename)
{
    int error = luaL_dofile(rdis_lua->L, filename);
    if (error) {
        const char * error_msg = luaL_checkstring(rdis_lua->L, -1);
        rdis_console(rdis_lua->rdis, error_msg);
        return -1;
    }
    return error;
}


/****************************************************************
* rl_uint64
****************************************************************/

int rl_uint64 (lua_State * L)
{
    uint64_t uint64_value = 0;

    if (lua_isnumber(L, -1)) {
        uint64_value = luaL_checkinteger(L, -1);
        lua_pop(L, -1);
    }
    else if (lua_isstring(L, -1)) {
        const char * s = luaL_checkstring(L, -1);
        uint64_value = strtoull(s, NULL, 0);
        lua_pop(L, -1);
    }
    else {
        luaL_error(L, "uint64 must be called with a number or string");
    }

    return rl_uint64_push(L, uint64_value);
}


int rl_uint64_push (lua_State * L, uint64_t value)
{
    uint64_t * value_ptr = lua_newuserdata(L, sizeof(uint64_t));
    luaL_getmetatable(L, "rdis.uint64");
    lua_setmetatable(L, -2);

    *value_ptr = value;

    return 1;
}


uint64_t rl_check_uint64 (lua_State * L, int position)
{
    if (lua_isnumber(L, position))
        return (uint64_t) luaL_checkinteger(L, position);

    void * data = luaL_checkudata(L, position, "rdis.uint64");
    luaL_argcheck(L, data != NULL, position, "expected uint64");
    return *((uint64_t *) data);
}


int rl_uint64_add (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs + rhs);

    return 1;
}


int rl_uint64_sub (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs - rhs);

    return 1;
}


int rl_uint64_mul (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs * rhs);

    return 1;
}


int rl_uint64_div (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs / rhs);

    return 1;
}


int rl_uint64_mod (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs % rhs);

    return 1;
}


int rl_uint64_eq  (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);

    if (lhs == rhs)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return 1;
}


int rl_uint64_lt  (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);

    if (lhs < rhs)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return 1;
}


int rl_uint64_le  (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -2);
    uint64_t rhs = rl_check_uint64(L, -1);
    lua_pop(L, 2);

    if (lhs <= rhs)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return 1;
}


int rl_uint64_tostring (lua_State * L)
{
    char tmp[64];

    uint64_t value = rl_check_uint64(L, -1);
    lua_pop(L, -1);

    snprintf(tmp, 64, "0x%llx", (unsigned long long) value);

    lua_pushstring(L, tmp);

    return 1;
}


int rl_uint64_number (lua_State * L)
{
    uint64_t value = rl_check_uint64(L, -1);
    lua_pop(L, -1);

    lua_pushnumber(L, (lua_Number) value);

    return 1;
}


/****************************************************************
* rl_instruction
****************************************************************/


int rl_ins_push (lua_State * L, struct _ins * ins)
{
    struct _ins ** ins_ptr = lua_newuserdata(L, sizeof(struct _ins *));
    luaL_getmetatable(L, "rdis.ins");
    lua_setmetatable(L, -2);

    *ins_ptr = object_copy(ins);

    return 1;
}


struct _ins * rl_check_ins (lua_State * L, int position)
{
    void ** data = luaL_checkudata(L, position, "rdis.ins");
    luaL_argcheck(L, data != NULL, position, "expected ins");
    return *((struct _ins **) data);
}


int rl_ins_gc (lua_State * L)
{
    struct _ins * ins = rl_check_ins(L, -1);
    lua_pop(L, 1);

    object_delete(ins);

    return 0;
}


int rl_ins_address (lua_State * L)
{
    struct _ins * ins = rl_check_ins(L, -1);
    lua_pop(L, 1);

    rl_uint64_push(L, ins->address);
    return 1;
}


int rl_ins_target (lua_State * L)
{
    struct _ins * ins = rl_check_ins(L, -1);
    lua_pop(L, 1);

    if (ins->flags & INS_FLAG_TARGET_SET)
        rl_uint64_push(L, ins->target);
    else
        lua_pushnil(L);

    return 1;
}


int rl_ins_bytes (lua_State * L)
{
    struct _ins * ins = rl_check_ins(L, -1);
    lua_pop(L, 1);

    lua_pushlstring(L, (const char *) ins->bytes, ins->size);
    return 1;
}


int rl_ins_description (lua_State * L)
{
    struct _ins * ins = rl_check_ins(L, -1);
    lua_pop(L, 1);

    if (ins->description == NULL)
        lua_pushnil(L);
    else
        lua_pushstring(L, ins->description);

    return 1;
}


int rl_ins_comment (lua_State * L)
{
    struct _ins * ins = rl_check_ins(L, -1);
    lua_pop(L, 1);

    if (ins->comment == NULL)
        lua_pushnil(L);
    else
        lua_pushstring(L, ins->comment);

    return 1;
}


/****************************************************************
* rl_graph_edge
****************************************************************/


int rl_graph_edge_push (lua_State * L, struct _graph_edge * edge)
{
    struct _graph_edge ** edge_ptr;
    edge_ptr = lua_newuserdata(L, sizeof(struct _graph_edge *));
    luaL_getmetatable(L, "rdis.graph_edge");
    lua_setmetatable(L, -2);

    *edge_ptr = object_copy(edge);

    return 1;
}


struct _graph_edge * rl_check_graph_edge (lua_State * L, int position)
{
    void ** data = luaL_checkudata(L, position, "rdis.graph_edge");
    luaL_argcheck(L, data != NULL, position, "expected graph edge");
    return *((struct _graph_edge **) data);
}


int rl_graph_edge_gc (lua_State * L)
{
    struct _graph_edge * edge = rl_check_graph_edge(L, -1);
    lua_pop(L, 1);

    object_delete(edge);

    return 0;
}


int rl_graph_edge_head (lua_State * L)
{
    struct _graph_edge * edge = rl_check_graph_edge(L, -1);
    lua_pop(L, 1);

    rl_uint64_push(L, edge->head);
    return 1;
}


int rl_graph_edge_tail (lua_State * L)
{
    struct _graph_edge * edge = rl_check_graph_edge(L, -1);
    lua_pop(L, 1);

    rl_uint64_push(L, edge->tail);
    return 1;
}


/****************************************************************
* rl_graph_node
****************************************************************/


int rl_graph_node_push (lua_State * L, struct _graph_node * node)
{
    struct _graph_node ** node_ptr;
    node_ptr = lua_newuserdata(L, sizeof(struct _graph_node *));
    luaL_getmetatable(L, "rdis.graph_node");
    lua_setmetatable(L, -2);

    *node_ptr = object_copy(node);

    return 1;
}


struct _graph_node * rl_check_graph_node (lua_State * L, int position)
{
    void ** data = luaL_checkudata(L, position, "rdis.graph_node");
    luaL_argcheck(L, data != NULL, position, "expected graph node");
    return *((struct _graph_node **) data);
}


int rl_graph_node_gc (lua_State * L)
{
    struct _graph_node * node = rl_check_graph_node(L, -1);
    lua_pop(L, 1);

    object_delete(node);

    return 0;
}


int rl_graph_node_index (lua_State * L)
{
    struct _graph_node * node = rl_check_graph_node(L, -1);
    lua_pop(L, 1);

    rl_uint64_push(L, node->index);
    return 1;
}


int rl_graph_node_edges (lua_State * L)
{
    struct _graph_node * node = rl_check_graph_node(L, -1);
    lua_pop(L, 1);

    struct _list_it * it;

    lua_newtable(L);
    int table_index = 1;

    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        lua_pushinteger(L, table_index++);
        rl_graph_edge_push(L, it->data);
        lua_settable(L, -3);
    }

    return 1;
}


int rl_graph_node_instructions (lua_State * L)
{
    struct _graph_node * node = rl_check_graph_node(L, -1);
    lua_pop(L, 1);

    struct _list_it * it;

    lua_newtable(L);
    int table_index = 1;

    for (it = list_iterator(node->data); it != NULL; it = it->next) {
        lua_pushinteger(L, table_index++);
        rl_ins_push(L, it->data);
        lua_settable(L, -3);
    }

    return 1;
}


/****************************************************************
* rl_rdis
****************************************************************/


struct _rdis_lua * rl_get_rdis_lua (lua_State * L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "rdis_lua");
    struct _rdis_lua * rdis_lua = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return rdis_lua;
}


int rl_rdis_console (lua_State * L)
{
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    const char * string = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    rdis_console(rdis_lua->rdis, string);

    return 0;
}


int rl_rdis_functions (lua_State * L)
{
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    lua_newtable(L);

    struct _map_it * it;
    for (it  = map_iterator(rdis_lua->rdis->functions);
         it != NULL;
         it  = map_it_next(it)) {
        struct _function * function = map_it_data(it);

        rl_uint64_push(L, function->address);

        struct _label * label = map_fetch(rdis_lua->rdis->labels, function->address);
        lua_pushstring(L, label->text);

        lua_settable(L, -3);
    }

    return 1;
}


int rl_rdis_peek (lua_State * L)
{
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    uint64_t addr = rl_check_uint64(L, -1);
    lua_pop(L, 1);

    struct _buffer * buffer = map_fetch_max(rdis_lua->rdis->memory, addr);
    uint64_t base = map_fetch_max_key(rdis_lua->rdis->memory, addr);

    if ((buffer == NULL) || (buffer->size + base > addr))
        luaL_error(L, "%llx not in memory", addr);

    uint8_t c = buffer->bytes[addr - base];

    lua_pushinteger(L, c);

    return 1;
}


int rl_rdis_poke (lua_State * L)
{
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    uint64_t         address    = rl_check_uint64(L, -1);
    const uint8_t *  bytes      = (const uint8_t *) luaL_checkstring(L, -2);
    size_t           bytes_size = lua_objlen(L, -2);

    struct _buffer * buffer = buffer_create(bytes, bytes_size);
    lua_pop(L, 2);

    rdis_update_memory(rdis_lua->rdis, address, buffer);

    object_delete(buffer);

    return 0;
}


int rl_rdis_node (lua_State * L)
{
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    uint64_t index = rl_check_uint64(L, -1);
    lua_pop(L, 1);

    struct _graph_node * node = graph_fetch_node(rdis_lua->rdis->graph, index);
    if (node == NULL)
        luaL_error(L, "did not find node");

    rl_graph_node_push(L, node);

    return 1;
}


int rl_rdis_load (lua_State * L)
{
    printf("rdis load\n");
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    // see if we get a valid loader
    struct _lua_loader * ll = lua_loader_create(L);

    if (ll == NULL)
        rdis_console(rdis_lua->rdis, "did not get a valid lua loader object");

    struct _map * memory    = lua_loader_memory_map(ll);
    if (memory == NULL) {
        rdis_console(rdis_lua->rdis, "could not get a memory map from lua loader");
    }

    struct _graph * graph = lua_loader_graph(ll, memory);
    if (graph == NULL) {
        rdis_console(rdis_lua->rdis, "did not get a valid graph from lua loader");
        object_delete(memory);
    }

    struct _map * functions = lua_loader_functions(ll, memory);
    if (functions == NULL) {
        objects_delete(memory, graph, NULL);
        rdis_console(rdis_lua->rdis, "did not get a valid list of functions from lua loader");
    }

    struct _map * labels = lua_loader_labels(ll, memory);
    if (labels == NULL) {
        objects_delete(memory, graph, functions, NULL);
        rdis_console(rdis_lua->rdis, "could not get labels for all functions from lua loader");
    }

    printf("got all valid objects from lua loader\n");

    // clear all gui windows
    rdis_clear_gui(rdis_lua->rdis);

    // clear rdis
    objects_delete(rdis_lua->rdis->loader,
                   rdis_lua->rdis->graph,
                   rdis_lua->rdis->labels,
                   rdis_lua->rdis->functions,
                   rdis_lua->rdis->memory,
                   NULL);

    // reset rdis
    rdis_lua->rdis->loader    = (_loader *) ll;
    rdis_lua->rdis->graph     = graph;
    rdis_lua->rdis->labels    = labels;
    rdis_lua->rdis->functions = functions;
    rdis_lua->rdis->memory    = memory;

    return 0;
}


int rl_rdis_loader (lua_State * L)
{
    printf("rdis loader\n");
    struct _rdis_lua * rdis_lua = rl_get_rdis_lua(L);

    const char * filename = luaL_checkstring(L, -1);

    // do we get a valid loader from this file?
    _loader * loader = loader_create(filename);

    if (loader == NULL) {
        char tmp[256];
        snprintf(tmp, 256, "No valid loader for %s\n", filename);
        rdis_console(rdis_lua->rdis, tmp);
        lua_pop(L, 1);
        lua_pushboolean(L, 0);
        return 1;
    }

    // clear all gui windows
    rdis_clear_gui(rdis_lua->rdis);

    // clear rdis
    objects_delete(rdis_lua->rdis->loader,
                   rdis_lua->rdis->graph,
                   rdis_lua->rdis->labels,
                   rdis_lua->rdis->functions,
                   rdis_lua->rdis->memory,
                   NULL);

    // reset rdis
    rdis_lua->rdis->loader    = loader;
    rdis_lua->rdis->memory    = loader_memory_map(loader);
    rdis_lua->rdis->functions = loader_functions(loader,
                                                 rdis_lua->rdis->memory);
    rdis_lua->rdis->graph     = loader_graph_functions(loader,
                                                       rdis_lua->rdis->memory,
                                                       rdis_lua->rdis->functions);
    rdis_lua->rdis->labels    = loader_labels_functions(loader,
                                                        rdis_lua->rdis->memory,
                                                        rdis_lua->rdis->functions);

    lua_pop(L, 1);
    lua_pushboolean(L, 1);
    return 1;
}
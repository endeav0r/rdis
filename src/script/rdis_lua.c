#include "rdis_lua.h"

#include "index.h"
#include "label.h"
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
    {NULL, NULL}
};


static const struct luaL_Reg rl_rdis_lib_f [] = {
    {"console",   rl_rdis_console},
    {"functions", rl_rdis_functions},
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

    lua_pushlightuserdata(rdis_lua->L, rdis_lua);
    lua_setfield(rdis_lua->L, LUA_REGISTRYINDEX, "rdis_lua");

    luaL_register(rdis_lua->L, "rdis", rl_rdis_lib_f);

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
    return 0;
}


/****************
* rl_uint64
****************/

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

    printf("a\n");
    return rl_uint64_push(L, uint64_value);
}


int rl_uint64_push (lua_State * L, uint64_t value)
{
    uint64_t * value_ptr = lua_newuserdata(L, sizeof(uint64_t));
    luaL_getmetatable(L, "rdis.uint64");
    lua_setmetatable(L, -2);

    *value_ptr = value;

    printf("push\n");

    return 1;
}


uint64_t rl_check_uint64 (lua_State * L, int position)
{
    void * data = luaL_checkudata(L, position, "rdis.uint64");
    luaL_argcheck(L, data != NULL, position, "expected uint64");
    return *((uint64_t *) data);
}


int rl_uint64_add (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs + rhs);

    return 1;
}


int rl_uint64_sub (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs - rhs);

    return 1;
}


int rl_uint64_mul (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs * rhs);

    return 1;
}


int rl_uint64_div (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs / rhs);

    return 1;
}


int rl_uint64_mod (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);
    
    rl_uint64_push(L, lhs % rhs);

    return 1;
}


int rl_uint64_eq  (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);

    if (lhs == rhs)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return 1;
}


int rl_uint64_lt  (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
    lua_pop(L, 2);

    if (lhs < rhs)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return 1;
}


int rl_uint64_le  (lua_State * L)
{
    uint64_t lhs = rl_check_uint64(L, -1);
    uint64_t rhs = rl_check_uint64(L, -2);
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


/****************
* rl_rdis
****************/


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

    struct _tree_it * it;
    for (it = tree_iterator(rdis_lua->rdis->function_tree);
         it != NULL;
         it = tree_it_next(it)) {
        struct _index * index = tree_it_data(it);

        rl_uint64_push(L, index->index);

        struct _label * label = map_fetch(rdis_lua->rdis->labels, index->index);
        lua_pushstring(L, label->text);

        lua_settable(L, -3);
    }

    return 1;
}
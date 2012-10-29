#include "rdis_lua.h"

#include "index.h"
#include "label.h"
#include "tree.h"

#include <stdlib.h>

static const struct luaL_Reg rl_rdis_lib_f [] = {
    {"console",   rl_rdis_console},
    {"functions",   rl_rdis_functions},
    {NULL, NULL}
};


struct _rdis_lua * rdis_lua_create (struct _rdis * rdis)
{
    struct _rdis_lua * rdis_lua;
    rdis_lua = (struct _rdis_lua *) malloc(sizeof(struct _rdis_lua));

    rdis_lua->rdis = rdis;
    rdis_lua->L = lua_open();
    luaL_openlibs(rdis_lua->L);

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

        char tmp[32];
        snprintf(tmp, 32, "0x%04llx", (unsigned long long) index->index);
        lua_pushstring(L, tmp);

        struct _label * label = map_fetch(rdis_lua->rdis->labels, index->index);
        lua_pushstring(L, label->text);

        lua_settable(L, -3);
    }

    return 1;
}
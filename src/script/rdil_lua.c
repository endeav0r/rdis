#include "rdil_lua.h"

static const struct luaL_Reg rl_rdil_lib_m [] = {
    {"__gc",       rl_rdil_gc},
    {"address",    rl_rdil_address},
    {"__tostring", rl_rdil_tostring},
    {NULL, NULL}
};

int rdil_lua_load (lua_State * L)
{
    luaL_newmetatable(L, "rdis.rdil");
    luaL_register(L, NULL, rl_rdil_lib_m);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    return 0;
}


struct _rdil * rl_check_rdil (lua_State * L, int position)
{
    void ** data = luaL_checkudata(L, position, "rdis.rdil");
    luaL_argcheck(L, data != NULL, position, "expected rdil");
    return *((struct _rdil **) data);
}


int rl_rdil_gc (lua_State * L)
{
    struct _rdil * rdil = rl_check_rdil(L, -1);
    lua_pop(L, 1);

    object_delete(rdil);

    return 0;
}
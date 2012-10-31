#ifndef rdis_lua_HEADER
#define rdis_lua_HEADER

#include "rdis.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

struct _rdis_lua {
	struct _rdis * rdis;
	lua_State * L;
};

struct _rdis_lua * rdis_lua_create  (struct _rdis * rdis);
void               rdis_lua_delete  (struct _rdis_lua * rdis_lua);

int                rdis_lua_execute (struct _rdis_lua * rdis_lua,
									 const char * string);

int      rl_uint64_push  (lua_State * L, uint64_t value);
uint64_t rl_check_uint64 (lua_State * L, int position);
int rl_uint64          (lua_State *L);
int rl_uint64_add      (lua_State *L);
int rl_uint64_sub      (lua_State *L);
int rl_uint64_mul      (lua_State *L);
int rl_uint64_div      (lua_State *L);
int rl_uint64_mod      (lua_State *L);
int rl_uint64_eq       (lua_State *L);
int rl_uint64_lt       (lua_State *L);
int rl_uint64_le       (lua_State *L);
int rl_uint64_tostring (lua_State *L);


int rl_rdis_console   (lua_State * L);
int rl_rdis_functions (lua_State * L);

#endif
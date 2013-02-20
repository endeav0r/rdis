#ifndef rl_redis_x86_HEADER
#define rl_redis_x86_HEADER

/*
* This lua object is kind of weird....
*/

#include "rdis.h"
#include "redis_x86.h"

struct _rl_redis_x86 {
    const struct _object    * object;
    struct _redis_x86 * redis_x86;
    struct _graph     * graph;
    struct _map       * ins_map;

    uint64_t last_index; // index of last node used for disassembly
    uint64_t next_index; // index of next node to create

    int has_false_stack; // attempt to protect user from himself/herself
};

struct _rl_redis_x86 * rl_redis_x86_create (struct _redis_x86 * redis_x86);
void                   rl_redis_x86_delete (struct _rl_redis_x86 *);
struct _rl_redis_x86 * rl_redis_x86_copy   (struct _rl_redis_x86 *);

int                    rl_redis_x86_push     (lua_State * L, struct _redis_x86 * redis_x86);
struct _rl_redis_x86 * rl_check_rl_redis_x86 (lua_State * L, int position);

int rl_redis_x86_gc           (lua_State * L);
int rl_redis_x86_variables    (lua_State * L);
int rl_redis_x86_graph        (lua_State * L);
int rl_redis_x86_step         (lua_State * L);
int rl_redis_x86_set_variable (lua_State * L);
int rl_redis_x86_false_stack  (lua_State * L);

#endif
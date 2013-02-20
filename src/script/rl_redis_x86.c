#include "rl_redis_x86.h"

#include <string.h>

/****************************************************************
* rl_redis_x86 object
****************************************************************/

static const struct _object rl_redis_x86_object = {
    (void     (*) (void *)) rl_redis_x86_delete, 
    (void *   (*) (void *)) rl_redis_x86_copy,
    NULL,
    NULL,
    NULL
};


struct _rl_redis_x86 * rl_redis_x86_create (struct _redis_x86 * redis_x86)
{
    struct _rl_redis_x86 * rl_redis_x86;
    rl_redis_x86 = (struct _rl_redis_x86 *) malloc(sizeof(struct _rl_redis_x86));

    rl_redis_x86->object = &rl_redis_x86_object;
    rl_redis_x86->redis_x86 = object_copy(redis_x86);
    rl_redis_x86->graph     = graph_create();
    rl_redis_x86->ins_map   = map_create();

    rl_redis_x86->last_index      = 0;
    rl_redis_x86->next_index      = 1;
    rl_redis_x86->has_false_stack = 0;

    return rl_redis_x86;
}


void rl_redis_x86_delete (struct _rl_redis_x86 * rl_redis_x86)
{
    objects_delete(rl_redis_x86->redis_x86,
                   rl_redis_x86->graph,
                   rl_redis_x86->ins_map,
                   NULL);
    free(rl_redis_x86);
}


struct _rl_redis_x86 * rl_redis_x86_copy (struct _rl_redis_x86 * rl_redis_x86)
{
    struct _rl_redis_x86 * new_rl_redis_x86;
    new_rl_redis_x86 = (struct _rl_redis_x86 *) malloc(sizeof(struct _rl_redis_x86));

    new_rl_redis_x86->object = &rl_redis_x86_object;
    new_rl_redis_x86->redis_x86 = object_copy(rl_redis_x86->redis_x86);
    new_rl_redis_x86->graph     = object_copy(rl_redis_x86->graph);
    new_rl_redis_x86->ins_map   = object_copy(rl_redis_x86->ins_map);

    new_rl_redis_x86->last_index      = rl_redis_x86->last_index;
    new_rl_redis_x86->next_index      = rl_redis_x86->next_index;
    new_rl_redis_x86->has_false_stack = rl_redis_x86->has_false_stack;

    return new_rl_redis_x86;
}


/****************************************************************
* rl_redis_x86 convenience functions & data
****************************************************************/

struct rl_redis_x86_variables_item {
    char * name;
    int index;
};

struct rl_redis_x86_variables_item rl_redis_x86_variables_items[] = {
    {"eax", RED_EAX},
    {"ebx", RED_EBX},
    {"ecx", RED_ECX},
    {"edx", RED_EDX},
    {"esp", RED_ESP},
    {"ebp", RED_EBP},
    {"esi", RED_ESI},
    {"edi", RED_EDI},
    {"eip", RED_EIP},
    {"CF",  RED_CF},
    {"OF",  RED_OF},
    {"SF",  RED_SF},
    {"ZF",  RED_ZF},
    {NULL, -1}
};

struct _ins * rl_redis_x86_create_ins (struct _redis_x86 * redis_x86)
{
    ud_t ud_obj;
    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, redis_x86->ins_bytes, redis_x86->ins_size);

    if (ud_disassemble(&ud_obj) == 0) {
        fprintf(stderr, "disassembly error %p %d\n",
                redis_x86->ins_bytes,
                (int) redis_x86->ins_size);
        return NULL;
    }

    struct _ins * ins = ins_create(redis_x86->ins_addr,
                                   ud_insn_ptr(&ud_obj),
                                   ud_insn_len(&ud_obj),
                                   ud_insn_asm(&ud_obj),
                                   NULL);

    return ins;
}

/****************************************************************
* rl_redis_x86 lua API
****************************************************************/

int rl_redis_x86_push (lua_State * L, struct _redis_x86 * redis_x86)
{
    struct _rl_redis_x86 ** rl_redis_x86_ptr;
    rl_redis_x86_ptr = lua_newuserdata(L, sizeof(struct _rl_redis_x86 * ));
    luaL_getmetatable(L, "rdis.redis_x86");
    lua_setmetatable(L, -2);

    *rl_redis_x86_ptr = rl_redis_x86_create(redis_x86);

    return 1;
}


struct _rl_redis_x86 * rl_check_rl_redis_x86 (lua_State * L, int position)
{
    void ** data = luaL_checkudata(L, position, "rdis.redis_x86");
    luaL_argcheck(L, data != NULL, position, "expected redis_x86");
    return *((struct _rl_redis_x86 **) data);
}


int rl_redis_x86_gc (lua_State * L)
{
    struct _rl_redis_x86 * rl_redis_x86 = rl_check_rl_redis_x86(L, -1);
    lua_pop(L, 1);

    object_delete(rl_redis_x86);

    return 0;
}


int rl_redis_x86_variables (lua_State * L)
{
    struct _rl_redis_x86 * rl_redis_x86 = rl_check_rl_redis_x86(L, -1);

    lua_newtable(L);

    int i = 0;
    while (rl_redis_x86_variables_items[i].name != NULL) {
        lua_pushstring(L, rl_redis_x86_variables_items[i].name);
        rl_uint64_push(L, 
         rl_redis_x86->redis_x86->regs[rl_redis_x86_variables_items[i].index]);
        lua_settable(L, -3);
        i++;
    }

    return 1;
}


int rl_redis_x86_graph (lua_State * L)
{
    struct _rl_redis_x86 * rl_redis_x86 = rl_check_rl_redis_x86(L, -1);

    lua_pop(L, 1);

    rl_graph_push(L, rl_redis_x86->graph);

    return 1;
}


int rl_redis_x86_step (lua_State * L)
{
    struct _rl_redis_x86 * rl_redis_x86 = rl_check_rl_redis_x86(L, -1);

    uint64_t this_index; // index of node disassembled
    int new_instruction = 0;

    // convenience pointers
    struct _redis_x86 * redis_x86 = rl_redis_x86->redis_x86;
    struct _graph     * graph     = rl_redis_x86->graph;
    struct _map       * ins_map   = rl_redis_x86->ins_map;

    if (redis_x86_step(redis_x86) == REDIS_SUCCESS) {
        uint64_t address = redis_x86->ins_addr;

        // do we have an instruction at this address already?
        struct _index * index = map_fetch(ins_map, address);
        // we have an instruction, fetch it, make sure it matches
        if (index) {
            struct _graph_node * node = graph_fetch_node(graph, index->index);
            struct _ins * ins = list_first(node->data);

            // instructions diverge, create new instruction
            if (    (ins->size != redis_x86->ins_size)
                 || (memcmp(ins->bytes, redis_x86->ins_bytes, redis_x86->ins_size))) {
                ins = rl_redis_x86_create_ins(redis_x86);
                if (ins == NULL) {
                    luaL_error(L, "could not create ins");
                    lua_pop(L, 1);
                    return 0;
                }

                struct _list * list = list_create();
                list_append(list, ins);
                object_delete(ins);

                graph_add_node(graph, rl_redis_x86->next_index, list);
                object_delete(list);

                map_remove(ins_map, redis_x86->ins_addr);
                index = index_create(rl_redis_x86->next_index++);
                map_insert(ins_map, redis_x86->ins_addr, index);
                this_index = index->index;
                object_delete(index);

                new_instruction = 1;
            }
            else
                this_index = index->index;
        }
        // no instruction at this address, create it
        else {
            struct _ins * ins = rl_redis_x86_create_ins(redis_x86);
            if (ins == NULL) {
                luaL_error(L, "could not create ins");
                lua_pop(L, 1);
                return 0;
            }
            struct _list * list = list_create();
            list_append(list, ins);
            object_delete(ins);

            graph_add_node(graph, rl_redis_x86->next_index, list);
            object_delete(list);

            index = index_create(rl_redis_x86->next_index++);
            map_insert(ins_map, redis_x86->ins_addr, index);
            this_index = index->index;
            object_delete(index);

            new_instruction = 1;
        }

        /*
        * create an edge from last index to this index
        * because our graph library enforces both the condition that the head
        * and tail nodes are valid, and that there exists only one edge per
        * head->tail combination, we can blindly add edges here and let the
        * graph library work out the details
        */
        struct _ins_edge * ins_edge = ins_edge_create(INS_EDGE_NORMAL);
        graph_add_edge(graph, rl_redis_x86->last_index, this_index, ins_edge);
        object_delete(ins_edge);
        rl_redis_x86->last_index = this_index;
    }

    lua_pop(L, 1);

    lua_pushboolean(L, new_instruction);
    return 1;
}


int rl_redis_x86_set_variable (lua_State * L)
{
    uint64_t               value         = rl_check_uint64(L, -1);
    const char *           variable_name = luaL_checkstring(L, -2);
    struct _rl_redis_x86 * rl_redis_x86  = rl_check_rl_redis_x86(L, -3);

    int i = 0;
    while (rl_redis_x86_variables_items[i].name != NULL) {
        if (strcmp(variable_name, rl_redis_x86_variables_items[i].name) == 0) {
            rl_redis_x86->redis_x86->regs[rl_redis_x86_variables_items[i].index] = value;
            lua_pop(L, 3);
            return 0;
        }
        i++;
    }

    lua_pop(L, 3);
    luaL_error(L, "set_variable called with invalid variable name");
    return 0;
}


int rl_redis_x86_false_stack (lua_State * L)
{
    struct _rl_redis_x86 * rl_redis_x86 = rl_check_rl_redis_x86(L, -1);

    if (rl_redis_x86->has_false_stack) {
        lua_pop(L, 1);
        luaL_error(L, "redis_x86 already has false stack");
        return 0;
    }

    redis_x86_false_stack(rl_redis_x86->redis_x86);
    lua_pop(L, 1);
    return 0;
}
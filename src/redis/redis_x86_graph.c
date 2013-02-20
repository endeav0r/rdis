#include "redis_x86_graph.h"

#include "index.h"
#include "instruction.h"
#include "list.h"

#include <string.h>
#include <udis86.h>

struct _ins * redis_x86_create_ins (struct _redis_x86 * redis_x86)
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


struct _graph * redis_x86_graph (uint64_t address, struct _map * memory)
{
    uint64_t next_index = 1;
    uint64_t this_index = 0;
    uint64_t last_index = 0;

    struct _redis_x86 * redis_x86 = redis_x86_create();
    redis_x86_mem_from_mem_map(redis_x86, memory);
    redis_x86->regs[RED_EIP] = address;
    redis_x86_false_stack(redis_x86);

    struct _map * ins_map = map_create();
    struct _graph * graph = graph_create();

    while (redis_x86_step(redis_x86) == REDIS_SUCCESS) {
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
                ins = redis_x86_create_ins(redis_x86);
                if (ins == NULL) {
                    fprintf(stderr, "could not create ins, eip=%llx\n",
                            (unsigned long long) redis_x86->ins_addr);
                    break;
                }

                struct _list * list = list_create();
                list_append(list, ins);
                object_delete(ins);

                graph_add_node(graph, next_index, list);
                object_delete(list);

                map_remove(ins_map, redis_x86->ins_addr);
                index = index_create(next_index++);
                map_insert(ins_map, redis_x86->ins_addr, index);
                this_index = index->index;
                object_delete(index);
            }
            else
                this_index = index->index;
        }
        // no instruction at this address, create it
        else {
            struct _ins * ins = redis_x86_create_ins(redis_x86);
            if (ins == NULL) {
                fprintf(stderr, "could not create ins eip=%llx\n",
                        (unsigned long long) redis_x86->ins_addr);
                break;
            }
            struct _list * list = list_create();
            list_append(list, ins);
            object_delete(ins);

            graph_add_node(graph, next_index, list);
            object_delete(list);

            index = index_create(next_index++);
            map_insert(ins_map, redis_x86->ins_addr, index);
            this_index = index->index;
            object_delete(index);
        }

        /*
        * create an edge from last index to this index
        * because our graph library enforces both the condition that the head
        * and tail nodes are valid, and that there exists only one edge per
        * head->tail combination, we can blindly add edges here and let the
        * graph library work out the details
        */
        printf("[edge] %llx -> %llx\n",
               (unsigned long long) last_index,
               (unsigned long long) this_index);
        struct _ins_edge * ins_edge = ins_edge_create(INS_EDGE_NORMAL);
        graph_add_edge(graph, last_index, this_index, ins_edge);
        object_delete(ins_edge);
        last_index = this_index;
        printf("%llx -> %llx\n",
               (unsigned long long) last_index,
               (unsigned long long) this_index);
    }

    object_delete(ins_map);
    object_delete(redis_x86);

    return graph;
}
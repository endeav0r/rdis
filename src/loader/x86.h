#ifndef x86_HEADER
#define x86_HEADER

#include <udis86.h>

#include "instruction.h"
#include "graph.h"
#include "map.h"
#include "x8664.h"

#define _x86_graph_wqueue       _x8664_graph_wqueue
#define x86_graph_wqueue_create x8664_graph_wqueue_create
#define x86_graph_wqueue_delete x8664_graph_wqueue_delete
#define x86_graph_wqueue_copy   x8664_graph_wqueue_copy

void * x86_graph_wqueue (struct _x86_graph_wqueue * x86_graph_wqueue);

struct _ins *   x86_ins   (uint64_t address, ud_t * ud_obj);

/*
* @param address   the base address at the beginning of data
* @param offset    the offset into data where we begin disassembly
* @param data      the data to disassemble
* @param data_size the size of data in bytes
*/
struct _graph * x86_graph (uint64_t address,
                           size_t offset,
                           void * data,
                           size_t data_size);


struct _map * x86_functions (uint64_t address,
                             size_t offset,
                             void * data,
                             size_t data_size);

#endif
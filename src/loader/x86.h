#ifndef x86_HEADER
#define x86_HEADER

#include <udis86.h>

#include "instruction.h"
#include "graph.h"
#include "map.h"
#include "x8664.h"

#define _x86_wqueue _x8664_wqueue
#define x86_wqueue_create x8664_wqueue_create
#define x86_wqueue_delete x8664_wqueue_delete
#define x86_wqueue_copy   x8664_wqueue_copy

void * x86_graph_wqueue     (struct _x86_wqueue * x86_wqueue);
void * x86_functions_wqueue (struct _x86_wqueue * x86_wqueue);

struct _ins *   x86_ins   (uint64_t address, ud_t * ud_obj);

/*
* @param address   the address disassembly will begin at
* @param memory    loader map of memory
*/
struct _graph * x86_graph     (uint64_t address, struct _map * memory);
struct _map *   x86_functions (uint64_t address, struct _map * memory);

#endif
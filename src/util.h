#ifndef util_HEADER
#define util_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "buffer.h"
#include "graph.h"
#include "map.h"
#include "tree.h"


#define STRING_ASCII 1


/*
* For each function in functions, gets the corresponding graph node and removes
* predecessors to that node from the graph.
* This fixes issues with compiler optimizations jumping directly to functions
*/
void remove_function_predecessors (struct _graph * graph, struct _map * functions);


/*
* Starting with the function of which node index is a the head, create a
* call graph. The call graph will be of the same internal structure as a loader
* graph.
*/
struct _graph * create_call_graph (struct _graph * graph, uint64_t index);

int is_string (uint8_t * data, size_t data_size);

size_t rdstrcat (char * dst, char * src, size_t size);

struct _ins * graph_fetch_ins (struct _graph * graph, uint64_t address);

int mem_map_byte (struct _map * mem_map, uint64_t address);

int mem_map_set (struct _map * mem_map, uint64_t address, struct _buffer * buffer);

#endif
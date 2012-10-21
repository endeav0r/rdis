#ifndef util_HEADER
#define util_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "graph.h"
#include "tree.h"


/*
* For each function in functions, gets the corresponding graph node and removes
* predecessors to that node from the graph.
* This fixes issues with compiler optimizations jumping directly to functions
*/
void remove_function_predecessors (struct _graph * graph, struct _tree * functions);

#endif
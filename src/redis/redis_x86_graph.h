#ifndef redis_x86_graph_HEADER
#define redis_x86_graph_HEADER

#include "graph.h"
#include "map.h"
#include "redis_x86.h"

struct _graph * redis_x86_graph (uint64_t address, struct _map * memory);

#endif
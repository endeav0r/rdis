#ifndef gui_HEADER
#define gui_HEADER

#include "loader.h"
#include "graph.h"
#include "tree.h"

struct _gui {
    _loader * loader;
    uint64_t        entry;
    struct _graph * graph;
    struct _tree  * function_tree;
    struct _map   * labels;
};


struct _gui * gui_create (const char * filename);
void          gui_delete (struct _gui * gui);

void          gui_rdgwindow  (struct _gui * gui, uint64_t top_index);
void          gui_funcwindow (struct _gui * gui);

#endif
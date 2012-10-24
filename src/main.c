#include "elf64.h"

#include "graph.h"
#include "instruction.h"
#include "list.h"
#include "rdg.h"
#include "rdstring.h"
#include "util.h"

#include <cairo.h>
#include <fontconfig/fontconfig.h>

#include <stdio.h>
#include <string.h>


int main (int argc, char * argv[])
{
    _loader * loader;

    loader = loader_create(argv[1]);
    if (loader == NULL) {
        fprintf(stderr, "failed to load file %s\n", argv[1]);
        return -1;
    }

    printf("entry: %llx\n", (unsigned long long) loader_entry(loader));

    struct _graph * graph = loader_graph(loader);

    graph_reduce(graph);

//    graph_debug(graph);

    object_delete(graph);

    object_delete(loader);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
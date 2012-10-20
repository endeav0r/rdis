#include "elf64.h"

#include "graph.h"
#include "instruction.h"
#include "list.h"
#include "rdgraph.h"
#include "rdstring.h"
#include "util.h"

#include <cairo.h>
#include <fontconfig/fontconfig.h>

#include <stdio.h>
#include <string.h>


void graphviz_out (struct _graph * graph)
{
    char * graphviz_string = rdgraph_graphviz_string(graph);

    FILE * fh;
    fh = fopen("graph.dot", "w");
    if (fh == NULL) {
        fprintf(stderr, "could not open file graph.dot\n");
        exit(-1);
    }

    fwrite(graphviz_string, 1, strlen(graphviz_string), fh);

    fclose(fh);

    free(graphviz_string);
}


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

    struct _graph * family = graph_family(graph, 0x403160);

    graph_debug(family);

    object_delete(family);
    object_delete(graph);

    object_delete(loader);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
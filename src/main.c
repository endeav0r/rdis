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
    struct _elf64 * elf64;

    elf64 = elf64_create(argv[1]);
    printf("entry: %llx\n", (unsigned long long) elf64_entry(elf64));

    struct _graph * graph = elf64_graph(elf64);
    graph_reduce(graph);
    struct _graph * graph_copy = object_copy(graph);
    graph_delete(graph);

    struct _graph * family = graph_family(graph_copy, 0x402fc0);

    graph_debug(family);

    /*
    struct _rdg_graph * rdg_graph;

    rdg_graph = rdg_graph_create(elf64_entry(elf64), family);
    rdg_graph_delete(rdg_graph);
    */

    graph_delete(family);
    graph_delete(graph_copy);
    elf64_delete(elf64);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
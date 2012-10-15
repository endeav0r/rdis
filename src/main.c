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


void graph_debug (struct _graph * graph)
{
    struct _graph_it * it;
    for (it = graph_iterator(graph); it != NULL; it = graph_it_next(it)) {
        struct _list_it * edge_it;
        struct _graph_edge * edge;
        printf("%llx [ ", (unsigned long long) graph_it_index(it));
        for (edge_it = list_iterator(graph_it_edges(it)); edge_it != NULL; edge_it = edge_it->next) {
            edge = edge_it->data;
            printf("(%llx -> %llx) ",
                   (unsigned long long) edge->head,
                   (unsigned long long) edge->tail);
        }
        printf("]\n");

        struct _list *    ins_list = graph_it_data(it);
        struct _list_it * ins_it;

        for (ins_it = list_iterator(ins_list); ins_it != NULL; ins_it = ins_it->next) {
            struct _ins * ins;
            ins = ins_it->data;
            printf("  %llx  ", (unsigned long long) ins->address);

            printf("%s\n", ins->description);
        }
    }
}


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

    elf64 = elf64_create("one");
    printf("entry: %llx\n", (unsigned long long) elf64_entry(elf64));

    struct _graph * graph = elf64_graph(elf64);

    graph_reduce(graph);

    struct _graph * family = graph_family(graph, 0x4004fc);

    graph_debug(family);

    struct _rdg_graph * rdg_graph;

    rdg_graph = rdg_graph_create(0x4004fc, family);
    rdg_graph_delete(rdg_graph);

    graph_delete(family);
    graph_delete(graph);
    elf64_delete(elf64);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
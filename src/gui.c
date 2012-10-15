#include <gtk/gtk.h>

#include "elf64.h"
#include "graph.h"
#include "hexwindow.h"
#include "inswindow.h"
#include "tree.h"

int main (int argc, char * argv[])
{
    gtk_init(&argc, &argv);

    struct _elf64 * elf64 = elf64_create(argv[1]);

    struct _graph * graph = elf64_graph(elf64);
    graph_reduce(graph);
    struct _tree * function_tree = elf64_function_tree(elf64);

    struct _inswindow * inswindow;
    inswindow = inswindow_create(graph, function_tree, elf64_entry(elf64));

    object_delete(function_tree);
    object_delete(graph);
    elf64_delete(elf64);

    gtk_widget_show(inswindow_window(inswindow));

    gtk_main();

    return 0;
}
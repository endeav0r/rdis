#include <gtk/gtk.h>

#include "elf64.h"
#include "graph.h"
#include "hexwindow.h"
#include "inswindow.h"
#include "tree.h"

int main (int argc, char * argv[])
{
    gtk_init(&argc, &argv);

    _loader * loader = loader_create(argv[1]);
    if (loader == NULL) {
        fprintf(stderr, "could not load file %s\n", argv[1]);
        return -1;
    }

    struct _graph * graph = loader_graph(loader);
    graph_reduce(graph);

    struct _tree * function_tree = loader_function_tree(loader);

    struct _inswindow * inswindow;
    inswindow = inswindow_create(graph, function_tree, loader_entry(loader));

    object_delete(function_tree);
    object_delete(graph);
    object_delete(loader);

    gtk_widget_show(inswindow_window(inswindow));

    gtk_main();

    return 0;
}
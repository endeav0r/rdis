#include <gtk/gtk.h>

#include "gui.h"

#include "funcwindow.h"
#include "hexwindow.h"
#include "inswindow.h"
#include "rdgwindow.h"

int main (int argc, char * argv[])
{
    gtk_init(&argc, &argv);

    struct _gui * gui;

    gui = gui_create(argv[1]);
    if (gui == NULL) {
        fprintf(stderr, "could not load %s\n", argv[1]);
        return -1;
    }

    struct _funcwindow * funcwindow = funcwindow_create(gui);

    gtk_widget_show(funcwindow_window(funcwindow));

    gtk_main();

    gui_delete(gui);

    return 0;
}


struct _gui * gui_create (const char * filename)
{
    struct _gui * gui;

    gui = (struct _gui *) malloc(sizeof(struct _gui));
    gui->loader = loader_create(filename);

    if (gui->loader == NULL) {
        free(gui);
        return NULL;
    }

    gui->entry         = loader_entry(gui->loader);
    gui->graph         = loader_graph(gui->loader);
    gui->function_tree = loader_function_tree(gui->loader);
    gui->labels        = loader_labels(gui->loader);

    graph_reduce(gui->graph);

    return gui;
}


void gui_delete (struct _gui * gui)
{
    object_delete(gui->loader);
    object_delete(gui->graph);
    object_delete(gui->function_tree);
    object_delete(gui->labels);
}


void gui_rdgwindow (struct _gui * gui, uint64_t top_index)
{
    struct _rdgwindow * rdgwindow = rdgwindow_create(gui, top_index);
    gtk_widget_show(rdgwindow_window(rdgwindow));
}
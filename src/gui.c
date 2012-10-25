#include <gtk/gtk.h>
#include <fontconfig/fontconfig.h>

#include "gui.h"

#include "funcwindow.h"
#include "hexwindow.h"
#include "inswindow.h"
#include "rdgwindow.h"
#include "rdis.h"

int main (int argc, char * argv[])
{
    gtk_init(&argc, &argv);

    _loader * loader;

    loader = loader_create(argv[1]);

    if (loader == NULL) {
        fprintf(stderr, "could not load %s\n", argv[1]);
        return -1;
    }

    struct _rdis * rdis = rdis_create(loader);
    graph_reduce(rdis->graph);
    struct _gui * gui = gui_create(rdis);


    struct _funcwindow * funcwindow = funcwindow_create(gui);

    gtk_widget_show(funcwindow_window(funcwindow));

    gtk_main();

    rdis_delete(rdis);
    gui_delete(gui);

    return 0;
}


struct _gui * gui_create (struct _rdis * rdis)
{
    struct _gui * gui;

    gui = (struct _gui *) malloc(sizeof(struct _gui));

    gui->rdis = rdis;

    return gui;
}


void gui_delete (struct _gui * gui)
{
    free(gui);
}


void gui_rdgwindow (struct _gui * gui, uint64_t top_index)
{
    struct _rdgwindow * rdgwindow = rdgwindow_create(gui, top_index);
    gtk_widget_show(rdgwindow_window(rdgwindow));
}
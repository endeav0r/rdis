#include "gui.h"

#include "funcwindow.h"
#include "hexwindow.h"
#include "inswindow.h"
#include "queue.h"
#include "rdgwindow.h"
#include "rdiswindow.h"
#include "rdis.h"


static const struct _object gui_window_object = {
    (void   (*) (void *))         gui_window_delete, 
    (void * (*) (void *))         gui_window_copy,
    NULL,
    NULL,
    NULL
};


struct _gui * gui_create ()
{
    struct _gui * gui;

    gui = (struct _gui *) malloc(sizeof(struct _gui));
    
    gui->rdis              = NULL;
    gui->rdiswindow        = rdiswindow_create(gui);
    gui->windows           = map_create();
    gui->next_window_index = 0;

    gtk_widget_show(rdiswindow_window(gui->rdiswindow));

    return gui;
}


void gui_delete (struct _gui * gui)
{
    if (gui->rdis != NULL)
        object_delete(gui->rdis);
    object_delete(gui->windows);
    free(gui);
    // rdiswindow frees itself
}


void gui_set_rdis (struct _gui * gui, struct _rdis * rdis)
{
    if (gui->rdis != NULL)
        object_delete(gui->rdis);
    gui->rdis = rdis;

    rdis_set_console(gui->rdis,
                     (void (*) (void *, const char *)) gui_console,
                     gui);
}


uint64_t gui_add_window (struct _gui * gui, GtkWidget * window)
{
    uint64_t identifier = gui->next_window_index++;

    struct _gui_window * gui_window = gui_window_create(window);

    map_insert(gui->windows, identifier, gui_window);

    object_delete(gui_window);

    return identifier;
}


void gui_remove_window (struct _gui * gui, uint64_t identifier)
{
    map_remove(gui->windows, identifier);
}


void gui_close_windows (struct _gui * gui){
    // we are going to be making calls that modify gui->windows, so we need
    // to save its contents first
    struct _queue * queue = queue_create();
    struct _map_it * it;
    for (it = map_iterator(gui->windows); it != NULL; it = map_it_next(it)) {
        queue_push(queue, map_it_data(it));
    }

    while (queue->size > 0) {
        struct _gui_window * gui_window = queue_peek(queue);
        gtk_widget_destroy(gui_window->window);
        queue_pop(queue);
    }

    object_delete(queue);
}


void gui_rdgwindow (struct _gui * gui, struct _graph * graph)
{
    if (gui->rdis == NULL) {
        gui_console(gui, "can't create rdgwindow, no rdis loaded");
        return;
    }
    struct _rdgwindow * rdgwindow = rdgwindow_create(gui, graph);
    gtk_widget_show(rdgwindow_window(rdgwindow));
}


void gui_funcwindow (struct _gui * gui)
{
    if (gui->rdis == NULL) {
        gui_console(gui, "can't create funcwindow, no rdis loaded");
        return;
    }
    struct _funcwindow * funcwindow = funcwindow_create(gui);
    gtk_widget_show(funcwindow_window(funcwindow));
}


void gui_hexwindow (struct _gui * gui)
{
    if (gui->rdis == NULL) {
        gui_console(gui, "can't create hexwindow, no rdis loaded");
        return;
    }
    struct _hexwindow * hexwindow = hexwindow_create(gui);
    gtk_widget_show(hexwindow_window(hexwindow));
}


void gui_console (struct _gui * gui, const char * line)
{
    rdiswindow_console(gui->rdiswindow, line);
}


struct _gui_window * gui_window_create (GtkWidget * window)
{
    struct _gui_window * gui_window;
    gui_window = (struct _gui_window *) malloc(sizeof(struct _gui_window));

    gui_window->object = &gui_window_object;
    gui_window->window = window;

    return gui_window;
}


void gui_window_delete (struct _gui_window * gui_window)
{
    free(gui_window);
}


struct _gui_window * gui_window_copy (struct _gui_window * gui_window)
{
    return gui_window_create(gui_window->window);
}
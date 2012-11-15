#ifndef refwindow_HEADER
#define refwindow_HEADER

#include <gtk/gtk.h>
#include <glib.h>

#include "rdis.h"

struct _refwindow {
    GtkWidget    * window;
    GtkWidget    * scrolledWindow;
    GtkListStore * listStore;
    GtkWidget    * treeView;

    struct _gui  * gui;
    uint64_t       gui_identifier;
    uint64_t       callback_identifier;
};


struct _refwindow * refwindow_create (struct _gui * gui);
void                refwindow_delete (struct _refwindow * refwindow);
GtkWidget *         refwindow_window (struct _refwindow * refwindow);

void refwindow_destroy_event (GtkWidget * widget,
                              struct _refwindow * refwindow);

void refwindow_refresh (struct _refwindow * refwindow);

void refwindow_rdis_callback (struct _refwindow * refwindow);

#endif
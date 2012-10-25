#ifndef funcwindow_HEADER
#define funcwindow_HEADER

#include <gtk/gtk.h>
#include <glib.h>

#include "rdis.h"

struct _funcwindow {
    GtkWidget    * window;
    GtkWidget    * scrolledWindow;
    GtkListStore * listStore;
    GtkWidget    * treeView;

    struct _gui  * gui;
    uint64_t       callback_identifier;
};


struct _funcwindow * funcwindow_create (struct _gui * gui);
void                 funcwindow_delete (struct _funcwindow * funcwindow);
GtkWidget *          funcwindow_window (struct _funcwindow * funcwindow);

void funcwindow_append_row (struct _funcwindow *, uint64_t index);

void funcwindow_destroy_event (GtkWidget * widget,
                                struct _funcwindow * funcwindow);
void funcwindow_row_activated (GtkTreeView * treeView,
                               GtkTreePath * treePath,
                               GtkTreeViewColumn * treeViewColumn,
                               struct _funcwindow * funcwindow);
void funcwindow_edited        (GtkCellRendererText * renderer,
                               char * path,
                               char * new_text,
                               struct _funcwindow * funcwindow);

void funcwindow_rdis_callback (struct _funcwindow * funcwindow);

#endif
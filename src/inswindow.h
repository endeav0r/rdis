#ifndef inswindow_HEADER
#define inswindow_HEADER

#include <gtk/gtk.h>
#include <glib.h>

#include "graph.h"
#include "gui.h"
#include "tree.h"

#define DEFAULT_IMAGE_WIDTH  8.0
#define DEFAULT_IMAGE_HEIGHT 12.0

struct _inswindow {
    GtkWidget         * window;
    GtkWidget         * scrolledWindow;
    GtkWidget         * treeView;
    GtkListStore      * listStore;
    struct _gui       * gui;
};


struct _inswindow * inswindow_create (struct _gui * gui);

void                inswindow_delete (struct _inswindow * inswindow);

GtkWidget *         inswindow_window  (struct _inswindow * inswindow);

void inswindow_image_update (struct _inswindow * inswindow);
void inswindow_graph_update (struct _inswindow * inswindow, uint64_t index);

void inswindow_select_index (struct _inswindow * inswindow,
                             uint64_t index);

void inswindow_row_activated (GtkTreeView * treeView,
                             GtkTreePath * treePath,
                             GtkTreeViewColumn * treeViewColumn,
                             struct _inswindow * inswindow);

#endif
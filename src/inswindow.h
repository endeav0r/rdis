#ifndef inswindow_HEADER
#define inswindow_HEADER

#include <gtk/gtk.h>
#include <glib.h>

#include "graph.h"
#include "tree.h"

#define DEFAULT_IMAGE_WIDTH  8.0
#define DEFAULT_IMAGE_HEIGHT 12.0

struct _inswindow {
    GtkWidget         * window;
    GtkWidget         * outerpaned;
    GtkWidget         * innerpaned;
    GtkWidget         * insScrolledWindow;
    GtkWidget         * insTreeView;
    GtkListStore      * insListStore;
    GtkWidget         * funcScrolledWindow;
    GtkWidget         * funcTreeView;
    GtkListStore      * funcListStore;
    GtkWidget         * imageScrolledWindow;
    GtkWidget         * imageEventBox;
    GtkWidget         * image;
    struct _graph     * graph;
    struct _tree      * function_tree;
    struct _rdg_graph * rdg_graph;

    struct _graph * currently_displayed_graph;

    double image_drag_x;
    double image_drag_y;
    double image_zoom;
    int    image_drawing;
};


struct _inswindow * inswindow_create (struct _graph * graph,
                                      struct _tree  * function_tree,
                                      uint64_t        entry);
void                inswindow_delete (struct _inswindow * inswindow);

GtkWidget *         inswindow_window  (struct _inswindow * inswindow);

void inswindow_image_update (struct _inswindow * inswindow);
void inswindow_graph_update (struct _inswindow * inswindow, uint64_t index);

void inswindow_insTreeView_select_index (struct _inswindow * inswindow,
                                         uint64_t index);

void inswindow_insTreeView_row_activated (GtkTreeView * treeView,
                                          GtkTreePath * treePath,
                                          GtkTreeViewColumn * treeViewColumn,
                                          struct _inswindow * inswindow);

void inswindow_funcTreeView_row_activated (GtkTreeView * treeView,
                                           GtkTreePath * treePath,
                                           GtkTreeViewColumn * treeViewColumn,
                                           struct _inswindow * inswindow);

gboolean inswindow_image_motion_notify_event (GtkWidget * widget,
                                              GdkEventMotion * event,
                                              struct _inswindow * inswindow);

gboolean inswindow_image_button_press_event  (GtkWidget * widget,
                                              GdkEventButton * event,
                                              struct _inswindow * inswindow);

gboolean inswindow_image_scroll_event (GtkWidget * widget,
                                       GdkEventScroll * event,
                                       struct _inswindow * inswindow);

#endif
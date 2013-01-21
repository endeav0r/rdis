#ifndef rdgwindow_HEADER
#define rdgwindow_HEADER

#include <gtk/gtk.h>
#include <glib.h>

#include "graph.h"
#include "gui.h"
#include "rdg.h"
#include "tree.h"

#define RDGWINDOW_MAX_DEFAULT_WIDTH  880
#define RDGWINDOW_MAX_DEFAULT_HEIGHT 600

#define RDGWINDOW_NODE_COLOR_SELECT 1.0, 0.9, 0.9
#define RDGWINDOW_NODE_COLOR_PRE    0.9, 0.9, 1.0

enum {
    RDGWINDOW_OTHER,
    RDGWINDOW_INS_GRAPH,
    RDGWINDOW_CALL_GRAPH
};


struct _rdgwindow {
    GtkWidget         * window;
    GtkWidget         * scrolledWindow;
    GtkWidget         * imageEventBox;
    GtkWidget         * image;
    GtkWidget         * menu_popup;
    GtkWidget         * tooltip;

    struct _gui       * gui;
    uint64_t            gui_identifier;
    uint64_t            callback_identifier;

    uint64_t            top_index;
    int                 type;
    
    struct _rdg       * rdg;

    struct _graph     * graph;

    double image_drag_x;
    double image_drag_y;
    int    image_dragging;

    int scrolledWindow_width;
    int scrolledWindow_height;

    uint64_t       selected_node;
    uint64_t       selected_ins;
    struct _list * node_colors;

    int editing;
};



struct _rdgwindow * rdgwindow_create (struct _gui * gui,
                                      struct _graph * graph,
                                      int type,
                                      uint64_t top_index);
void                rdgwindow_delete (struct _rdgwindow * rdgwindow);
GtkWidget *         rdgwindow_window (struct _rdgwindow * rdgwindow);

// redraws the rdg_graph
void rdgwindow_image_update (struct _rdgwindow * rdgwindow);

void     rdgwindow_destroy_event (GtkWidget * widget,
                                  struct _rdgwindow * rdgwindow);

gboolean rdgwindow_image_motion_notify_event  (GtkWidget * widget,
                                               GdkEventMotion * event,
                                               struct _rdgwindow * rdgwindow);
gboolean rdgwindow_image_button_press_event   (GtkWidget * widget,
                                               GdkEventButton * event,
                                               struct _rdgwindow * rdgwindow);
gboolean rdgwindow_image_button_release_event (GtkWidget * widget,
                                               GdkEventButton * event,
                                               struct _rdgwindow * rdgwindow);
gboolean rdgwindow_image_key_press_event      (GtkWidget * widget,
                                               GdkEventKey * event,
                                               struct _rdgwindow * rdgwindow);
void     rdgwindow_size_allocate_event        (GtkWidget * widget,
                                               GdkRectangle * allocation,
                                               struct _rdgwindow * rdgwindow);

void rdgwindow_center_on_node (struct _rdgwindow * rdgwindow, uint64_t index);

void rdgwindow_reset_node_colors (struct _rdgwindow * rdgwindow);
void rdgwindow_color_node        (struct _rdgwindow * rdgwindow);
void rdgwindow_color_node_predecessors (struct _rdgwindow * rdgwindow);

void rdgwindow_rdis_callback (struct _rdgwindow * rdgwindow);

void rdgwindow_menu_popup    (struct _rdgwindow * rdgwindow);
void rdgwindow_user_function (GtkMenuItem * menuItem,
                              struct _rdgwindow * rdgwindow);
void rdgwindow_remove_all_after (GtkMenuItem * menuItem,
                                 struct _rdgwindow * rdgwindow);

#endif
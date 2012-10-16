#include "inswindow.h"

// for debugging
#include <cairo.h>

#include <gdk/gdk.h>
#include <stdio.h>
#include <string.h>

#include "instruction.h"
#include "object.h"
#include "rdgraph.h"
#include "util.h"

enum {
    COL_INDEX,
    COL_ADDR,
    COL_BYTES,
    COL_DESCRIPTION,
    COL_COMMENT,
    COL_N
};


enum {
    COL_FUNC_INDEX,
    COL_FUNC_ADDR,
    COL_FUNC_NAME,
    COL_FUNC_N
};


struct _inswindow * inswindow_create (struct _graph * graph,
                                      struct _tree  * function_tree,
                                      uint64_t        entry)
{
    struct _inswindow * inswindow;

    inswindow = (struct _inswindow *) malloc(sizeof(struct _inswindow));
    inswindow->window            = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    inswindow->insScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    inswindow->insListStore      = gtk_list_store_new(COL_N, 
                                                      G_TYPE_UINT64,
                                                      G_TYPE_STRING,
                                                      G_TYPE_STRING,
                                                      G_TYPE_STRING,
                                                      G_TYPE_STRING);
    inswindow->insTreeView       = gtk_tree_view_new_with_model(
                                         GTK_TREE_MODEL(inswindow->insListStore));
    inswindow->funcScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    inswindow->funcListStore      = gtk_list_store_new(COL_FUNC_N, 
                                                       G_TYPE_UINT64,
                                                       G_TYPE_STRING,
                                                       G_TYPE_STRING);
    inswindow->funcTreeView       = gtk_tree_view_new_with_model(
                                         GTK_TREE_MODEL(inswindow->funcListStore));
    inswindow->outerpaned     = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    inswindow->innerpaned     = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    inswindow->image          = gtk_image_new_from_file("logo.png");
    inswindow->imageEventBox  = gtk_event_box_new();
    inswindow->imageScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    inswindow->graph          = object_copy(graph);
    inswindow->function_tree  = object_copy(function_tree);
    inswindow->rdg_graph      = NULL;
    inswindow->currently_displayed_graph = NULL;
    inswindow->image_drawing  = 0;

    GtkTreeIter treeIter;
    GtkTreePath * treePath = NULL;


    /*
    * Instruction Tree View Init
    */

    // add all the instructions to the insListStore
    char   addrText[64];
    char   tmp[8];
    char   bytesText[64];
    size_t i;
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(inswindow->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {

        struct _list_it * list_it;
        for (list_it = list_iterator(graph_it_data(graph_it));
             list_it != NULL;
             list_it = list_it->next) {

            struct _ins * ins = list_it->data;

            // prepare text based off instruction
            snprintf(addrText, 64, "%04llx", (unsigned long long) ins->address);
            memset(bytesText, 0, 64);
            for (i = 0; i < (ins->size < 8 ? ins->size : 8); i++) {
                snprintf(tmp, 8, "%02x", ins->bytes[i]);
                strncat(bytesText, tmp, 64);
            }
            bytesText[63] = '\0';

            // add text to tree model
            gtk_list_store_append(inswindow->insListStore, &treeIter);
            gtk_list_store_set(inswindow->insListStore, &treeIter,
                               COL_INDEX,       ins->address,
                               COL_ADDR,        addrText,
                               COL_BYTES,       bytesText,
                               COL_DESCRIPTION, ins->description,
                               COL_COMMENT,     ins->comment,
                               -1);
            // if this is the instruction we want to select later, IE its
            // address is entry, we will set the treePath now.
            if (ins->address == entry)
                treePath = gtk_tree_model_get_path(
                           GTK_TREE_MODEL(inswindow->insListStore), &treeIter);
        }
    }

    // create the columns for the insTreeView
    GtkCellRenderer   * renderer;
    GtkTreeViewColumn * column;
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("address",
                                                        renderer,
                                                        "text", COL_ADDR,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->insTreeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("bytes",
                                                        renderer,
                                                        "text", COL_BYTES,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->insTreeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("description",
                                                        renderer,
                                                        "text", COL_DESCRIPTION,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->insTreeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("comment",
                                                        renderer,
                                                        "text", COL_COMMENT,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->insTreeView), column);

    // select entry row
    // according to GTK docs, GTK will handle gracefully invalid treePath
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(inswindow->insTreeView), treePath, NULL, 0);



    /*
    * Function Tree View Init
    */
    struct _tree_it * tree_it;
    for (tree_it = tree_iterator(inswindow->function_tree);
         tree_it != NULL;
         tree_it = tree_it_next(tree_it)) {
        struct _function * function = tree_it_data(tree_it);

        snprintf(addrText, 64, "%04llx", (unsigned long long) function->address);

        gtk_list_store_append(inswindow->funcListStore, &treeIter);
        gtk_list_store_set(inswindow->funcListStore, &treeIter,
                           COL_FUNC_INDEX, function->address,
                           COL_FUNC_ADDR,  addrText,
                           COL_FUNC_NAME,  function->name,
                           -1);
    }

    // create the columns for the funcTreeView
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("address",
                                                        renderer,
                                                        "text", COL_FUNC_ADDR,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->funcTreeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("name",
                                                        renderer,
                                                        "text", COL_FUNC_NAME,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->funcTreeView), column);


    /*
    * Other Init
    */ 

    // add things inside containers
    gtk_container_add(GTK_CONTAINER(inswindow->insScrolledWindow),
                      inswindow->insTreeView);

    gtk_container_add(GTK_CONTAINER(inswindow->imageEventBox),
                      inswindow->imageScrolledWindow);

    gtk_scrolled_window_add_with_viewport(
        GTK_SCROLLED_WINDOW(inswindow->imageScrolledWindow), inswindow->image);

    gtk_paned_add1(GTK_PANED(inswindow->innerpaned), inswindow->insScrolledWindow);
    gtk_paned_add2(GTK_PANED(inswindow->innerpaned), inswindow->imageEventBox);

    gtk_container_add(GTK_CONTAINER(inswindow->funcScrolledWindow),
                      inswindow->funcTreeView);

    gtk_paned_add1(GTK_PANED(inswindow->outerpaned), inswindow->funcScrolledWindow);
    gtk_paned_add2(GTK_PANED(inswindow->outerpaned), inswindow->innerpaned);

    gtk_container_add(GTK_CONTAINER(inswindow->window), inswindow->outerpaned);

    // signal some stuff
    g_signal_connect(inswindow->insTreeView,
                     "row-activated",
                     G_CALLBACK(inswindow_insTreeView_row_activated),
                     inswindow);

    g_signal_connect(inswindow->funcTreeView,
                     "row-activated",
                     G_CALLBACK(inswindow_funcTreeView_row_activated),
                     inswindow);

    g_signal_connect(inswindow->imageEventBox,
                     "motion-notify-event",
                     G_CALLBACK(inswindow_image_motion_notify_event),
                     inswindow);

    g_signal_connect(inswindow->imageEventBox,
                     "button-press-event",
                     G_CALLBACK(inswindow_image_button_press_event),
                     inswindow);

    g_signal_connect(inswindow->imageScrolledWindow,
                     "scroll-event",
                     G_CALLBACK(inswindow_image_scroll_event),
                     inswindow);

    // set more options
    gtk_window_set_default_size(GTK_WINDOW(inswindow->window), 800, 600);
    //g_object_set(inswindow->funcScrolledWindow, "min-content-width", 240, NULL);
    //g_object_set(inswindow->insScrolledWindow, "min-content-width",  320, NULL);

    // show ALL THE WIDGETS
    gtk_widget_show(inswindow->insScrolledWindow);
    gtk_widget_show(inswindow->insTreeView);
    gtk_widget_show(inswindow->funcScrolledWindow);
    gtk_widget_show(inswindow->funcTreeView);
    gtk_widget_show(inswindow->outerpaned);
    gtk_widget_show(inswindow->innerpaned);
    gtk_widget_show(inswindow->image);
    gtk_widget_show(inswindow->imageEventBox);
    gtk_widget_show(inswindow->imageScrolledWindow);

    return inswindow;
}



void inswindow_delete (struct _inswindow * inswindow)
{
    gtk_widget_destroy(inswindow->window);
    object_delete(inswindow->graph);
    free(inswindow);
}



GtkWidget * inswindow_window (struct _inswindow * inswindow)
{
    return inswindow->window;
}



void inswindow_image_update (struct _inswindow * inswindow)
{
    cairo_surface_write_to_png(inswindow->rdg_graph->surface, "cairo.png");
    GdkPixbuf * pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(inswindow->rdg_graph->surface,
                                         0,
                                         0,
                                         rdg_graph_width(inswindow->rdg_graph),
                                         rdg_graph_height(inswindow->rdg_graph));
    gtk_image_set_from_pixbuf(GTK_IMAGE(inswindow->image), pixbuf);
    /*
    inswindow->image_drawing = 1;
    printf("drawing\n");
    if (inswindow->currently_displayed_graph == NULL)
        return;

    char params_string[256];

    snprintf(params_string, 256, " size=\"%f, %f\"; ",
             DEFAULT_IMAGE_WIDTH * inswindow->image_zoom,
             DEFAULT_IMAGE_HEIGHT * inswindow->image_zoom);
    rdgraph_graph_to_png_file_params(inswindow->currently_displayed_graph,
                                     "/tmp/rdistmp.png",
                                     params_string);
    gtk_image_set_from_file(GTK_IMAGE(inswindow->image), "/tmp/rdistmp.png");
    inswindow->image_drawing = 0;
    printf("done drawing\n");
    */
}



void inswindow_graph_update (struct _inswindow * inswindow, uint64_t index)
{
    // find graph node this index refers to
    struct _graph_node * node = graph_fetch_node_max(inswindow->graph, index);

    // update currently_displayed_graph
    if (inswindow->currently_displayed_graph != NULL)
        object_delete(inswindow->currently_displayed_graph);
    inswindow->currently_displayed_graph = graph_family(inswindow->graph, node->index);

    // pseudo-find top node
    struct _graph_it * it = graph_iterator(inswindow->currently_displayed_graph);
    uint64_t top_node_index = graph_it_index(it);
    graph_it_delete(it);

    printf("inswindow_graph_update %llx %llx\n",
           (unsigned long long) node->index,
           (unsigned long long) top_node_index);

    graph_debug(inswindow->currently_displayed_graph);

    // update rdg_graph
    if (inswindow->rdg_graph != NULL)
        object_delete(inswindow->rdg_graph);
    inswindow->rdg_graph = rdg_graph_create(top_node_index,
                                         inswindow->currently_displayed_graph);

    // graph and display graph family
    inswindow->image_zoom = 1.0;
    inswindow_image_update(inswindow);
}



void inswindow_insTreeView_select_index (struct _inswindow * inswindow,
                                         uint64_t index)
{
    GtkTreeIter treeIter;
    uint64_t store_index;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(inswindow->insListStore),
                                      &treeIter) == FALSE)
        return;

    while (1) {
        gtk_tree_model_get(GTK_TREE_MODEL(inswindow->insListStore),
                           &treeIter,
                           COL_INDEX,
                           &store_index, -1);
        if (store_index == index) {
            gtk_tree_view_set_cursor(
                                     GTK_TREE_VIEW(inswindow->insTreeView),
                                     gtk_tree_model_get_path(
                                       GTK_TREE_MODEL(inswindow->insListStore),
                                                             &treeIter),
                                     NULL,
                                     0);
            break;
        }
        if (gtk_tree_model_iter_next(GTK_TREE_MODEL(inswindow->insListStore),
                                     &treeIter) == FALSE)
            break;
    }
}



void inswindow_insTreeView_row_activated (GtkTreeView * treeView,
                                       GtkTreePath * treePath,
                                       GtkTreeViewColumn * treeViewColumn,
                                       struct _inswindow * inswindow)
{
    uint64_t index;

    GtkTreeIter treeIter;

    // get the index
    gtk_tree_model_get_iter(GTK_TREE_MODEL(inswindow->insListStore),
                            &treeIter,
                            treePath);
    gtk_tree_model_get(GTK_TREE_MODEL(inswindow->insListStore),
                       &treeIter,
                       COL_INDEX,
                       &index, -1);

    inswindow_graph_update(inswindow, index);
}



void inswindow_funcTreeView_row_activated (GtkTreeView * treeView,
                                           GtkTreePath * treePath,
                                           GtkTreeViewColumn * treeViewColumn,
                                           struct _inswindow * inswindow)
{
    uint64_t index;

    GtkTreeIter treeIter;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(inswindow->funcListStore),
                            &treeIter,
                            treePath);
    gtk_tree_model_get(GTK_TREE_MODEL(inswindow->funcListStore),
                       &treeIter,
                       COL_FUNC_INDEX,
                       &index, -1);

    inswindow_graph_update(inswindow, index);
    inswindow_insTreeView_select_index(inswindow, index);
}



gboolean inswindow_image_motion_notify_event (GtkWidget * widget,
                                              GdkEventMotion * event,
                                              struct _inswindow * inswindow)
{
    GtkAdjustment * hadjust = gtk_scrolled_window_get_hadjustment(
                          GTK_SCROLLED_WINDOW(inswindow->imageScrolledWindow));
    GtkAdjustment * vadjust = gtk_scrolled_window_get_vadjustment(
                          GTK_SCROLLED_WINDOW(inswindow->imageScrolledWindow));

    double diff_x = inswindow->image_drag_x - event->x;
    double diff_y = inswindow->image_drag_y - event->y;

    gtk_adjustment_set_value(hadjust, gtk_adjustment_get_value(hadjust) + diff_x);
    gtk_adjustment_set_value(vadjust, gtk_adjustment_get_value(vadjust) + diff_y);

    gtk_scrolled_window_set_hadjustment(
                           GTK_SCROLLED_WINDOW(inswindow->imageScrolledWindow),
                           hadjust);

    gtk_scrolled_window_set_vadjustment(
                           GTK_SCROLLED_WINDOW(inswindow->imageScrolledWindow),
                           vadjust);

    inswindow->image_drag_x = event->x;
    inswindow->image_drag_y = event->y;

    return 0;
}


gboolean inswindow_image_button_press_event  (GtkWidget * widget,
                                              GdkEventButton * event,
                                              struct _inswindow * inswindow)
{
    if (inswindow->image_drawing)
        return 0;
    int x = (int) event->x;
    int y = (int) event->y;

    inswindow->image_drag_x = x;
    inswindow->image_drag_y = y;
    return 0;
}


gboolean inswindow_image_scroll_event (GtkWidget * widget,
                                       GdkEventScroll * event,
                                       struct _inswindow * inswindow)
{
    if (inswindow->image_drawing)
        return 1;

    int direction = -1;
    if (event->direction == GDK_SCROLL_SMOOTH) {
        double delta_x;
        double delta_y;
        gdk_event_get_scroll_deltas((GdkEvent *) event, &delta_x, &delta_y);
        if (delta_y < 0)
            direction = GDK_SCROLL_DOWN;
        else if (delta_y > 0)
            direction = GDK_SCROLL_UP;
    }
    else if (event->direction == GDK_SCROLL_UP)
        direction = GDK_SCROLL_UP;
    else if (event->direction == GDK_SCROLL_DOWN)
        direction = GDK_SCROLL_DOWN;


    if (direction == GDK_SCROLL_DOWN) {
        printf("scroll up\n");
        inswindow->image_zoom *= 1.1;
        if (inswindow->image_zoom > 10.0)
            inswindow->image_zoom = 10.0;
    }
    else if (direction == GDK_SCROLL_UP) {
        printf("scroll down\n");
        inswindow->image_zoom *= 0.9;
        if (inswindow->image_zoom < 0.25)
            inswindow->image_zoom = 0.25;
    }
    else
        return 1;

    //printf("image_zoom: %f\n", inswindow->image_zoom);
    printf("reduce and draw\n");
    rdg_graph_reduce_and_draw(inswindow->rdg_graph);

    inswindow_image_update(inswindow);

    return 1;
}
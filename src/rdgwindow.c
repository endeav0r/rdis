#include "rdgwindow.h"


struct _rdgwindow * rdgwindow_create (struct _gui * gui, uint64_t top_index)
{
    struct _rdgwindow * rdgwindow;

    rdgwindow = (struct _rdgwindow *) malloc(sizeof(struct _rdgwindow));
    rdgwindow->window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    rdgwindow->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    rdgwindow->imageEventBox  = gtk_event_box_new();
    rdgwindow->image          = gtk_image_new_from_file("logo.png");

    rdgwindow->gui            = gui;

    rdgwindow->top_index      = top_index;
    rdgwindow->rdg_graph      = NULL;
    rdgwindow->currently_displayed_graph = NULL;

    rdgwindow->image_drag_x   = 0;
    rdgwindow->image_drag_y   = 0;

    rdgwindow->selected_node  = -1;
    rdgwindow->node_colors    = NULL;


    gtk_container_add(GTK_CONTAINER(rdgwindow->imageEventBox),
                      rdgwindow->scrolledWindow);

    gtk_scrolled_window_add_with_viewport(
        GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow), rdgwindow->image);

    gtk_container_add(GTK_CONTAINER(rdgwindow->window), rdgwindow->imageEventBox);


    g_signal_connect(rdgwindow->imageEventBox,
                     "motion-notify-event",
                     G_CALLBACK(rdgwindow_image_motion_notify_event),
                     rdgwindow);

    g_signal_connect(rdgwindow->imageEventBox,
                     "button-press-event",
                     G_CALLBACK(rdgwindow_image_button_press_event),
                     rdgwindow);


    gtk_widget_show(rdgwindow->image);
    gtk_widget_show(rdgwindow->scrolledWindow);
    gtk_widget_show(rdgwindow->imageEventBox);


    rdgwindow_graph_update(rdgwindow);

    int width  = rdg_graph_width(rdgwindow->rdg_graph);
    int height = rdg_graph_width(rdgwindow->rdg_graph);

    if (width > RDGWINDOW_MAX_DEFAULT_WIDTH)
        width = RDGWINDOW_MAX_DEFAULT_WIDTH;
    if (height > RDGWINDOW_MAX_DEFAULT_HEIGHT)
        height = RDGWINDOW_MAX_DEFAULT_HEIGHT;

    gtk_window_set_default_size(GTK_WINDOW(rdgwindow->window), width, height);

    return rdgwindow;
}


void rdgwindow_delete (struct _rdgwindow * rdgwindow)
{
    if (rdgwindow->rdg_graph != NULL)
        object_delete(rdgwindow->rdg_graph);

    if (rdgwindow->currently_displayed_graph != NULL)
        object_delete(rdgwindow->currently_displayed_graph);

    if (rdgwindow->node_colors != NULL)
        object_delete(rdgwindow->node_colors);

    gtk_widget_destroy(rdgwindow->window);

    free(rdgwindow);
}


GtkWidget * rdgwindow_window (struct _rdgwindow * rdgwindow)
{
    return rdgwindow->window;
}


void rdgwindow_image_update (struct _rdgwindow * rdgwindow)
{
    rdg_draw(rdgwindow->rdg_graph);
    GdkPixbuf * pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(rdgwindow->rdg_graph->surface,
                                         0,
                                         0,
                                         rdg_graph_width(rdgwindow->rdg_graph),
                                         rdg_graph_height(rdgwindow->rdg_graph));
    gtk_image_set_from_pixbuf(GTK_IMAGE(rdgwindow->image), pixbuf);
}


void rdgwindow_graph_update (struct _rdgwindow * rdgwindow)
{

    // set currently_displayed_graph to top_index's node's family
    if (rdgwindow->currently_displayed_graph != NULL)
        object_delete(rdgwindow->currently_displayed_graph);
    rdgwindow->currently_displayed_graph = graph_family(rdgwindow->gui->graph,
                                                        rdgwindow->top_index);

    if (rdgwindow->rdg_graph != NULL)
        object_delete(rdgwindow->rdg_graph);

    rdgwindow->rdg_graph = rdg_graph_create(rdgwindow->top_index,
                                            rdgwindow->currently_displayed_graph,
                                            rdgwindow->gui->labels);

    rdgwindow_image_update(rdgwindow);
}



gboolean rdgwindow_image_motion_notify_event (GtkWidget * widget,
                                              GdkEventMotion * event,
                                              struct _rdgwindow * rdgwindow)
{
    GtkAdjustment * hadjust = gtk_scrolled_window_get_hadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));
    GtkAdjustment * vadjust = gtk_scrolled_window_get_vadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));

    double diff_x = rdgwindow->image_drag_x - event->x;
    double diff_y = rdgwindow->image_drag_y - event->y;

    gtk_adjustment_set_value(hadjust, gtk_adjustment_get_value(hadjust) + diff_x);
    gtk_adjustment_set_value(vadjust, gtk_adjustment_get_value(vadjust) + diff_y);

    gtk_scrolled_window_set_hadjustment(
                           GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow),
                           hadjust);

    gtk_scrolled_window_set_vadjustment(
                           GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow),
                           vadjust);

    rdgwindow->image_drag_x = event->x;
    rdgwindow->image_drag_y = event->y;

    return 0;
}


gboolean rdgwindow_image_button_press_event  (GtkWidget * widget,
                                              GdkEventButton * event,
                                              struct _rdgwindow * rdgwindow)
{
    int x = (int) event->x;
    int y = (int) event->y;

    rdgwindow->image_drag_x = x;
    rdgwindow->image_drag_y = y;

    GtkAdjustment * hadjust = gtk_scrolled_window_get_hadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));
    GtkAdjustment * vadjust = gtk_scrolled_window_get_vadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));

    // these are the x,y coords inside the image
    int image_x = x + (int) gtk_adjustment_get_value(hadjust);
    int image_y = y + (int) gtk_adjustment_get_value(vadjust);

    rdgwindow->selected_node = rdg_get_node_by_coords(rdgwindow->rdg_graph,
                                                      image_x, image_y);

    printf("image_x: %d, image_y: %d\n", image_x, image_y);

    printf("selected node: %llx\n", (unsigned long long) rdgwindow->selected_node);
    rdgwindow_color_node(rdgwindow);

    return 0;
}


void rdgwindow_color_node (struct _rdgwindow * rdgwindow)
{
    struct _list * node_colors = list_create();

    if (rdgwindow->node_colors != NULL) {
        struct _list_it * it;
        for (it = list_iterator(rdgwindow->node_colors); it != NULL; it = it->next) {
            struct _rdg_node_color * rdg_node_color = it->data;

            struct _rdg_node_color * new;
            new = rdg_node_color_create(rdg_node_color->index, RDG_NODE_BG_COLOR);
            list_append(node_colors, new);
            object_delete(new);
        }
        object_delete(rdgwindow->node_colors);
    }

    struct _rdg_node_color * rdg_node_color;
    rdg_node_color = rdg_node_color_create(rdgwindow->selected_node,
                                                  RDGWINDOW_NODE_COLOR_SELECT);
    list_append(node_colors, rdg_node_color);

    rdgwindow->node_colors = node_colors;
    rdg_color_nodes(rdgwindow->rdg_graph,
                    rdgwindow->currently_displayed_graph,
                    rdgwindow->gui->labels,
                    node_colors);
    rdgwindow_image_update(rdgwindow);
}


/*
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
*/
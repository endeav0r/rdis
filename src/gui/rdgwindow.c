#include "rdgwindow.h"

#include "instruction.h"
#include "queue.h"
#include "util.h"

#include <string.h>
#include <gdk/gdk.h>


GtkWidget * tipwindow (const char * text)
{
    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget * label  = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);

    // set up containers
    gtk_container_add(GTK_CONTAINER(window), label);

    GdkDisplay * gd = gdk_display_get_default();
    GdkScreen * gs = gtk_widget_get_screen(window);
    GdkWindow * rw = gdk_screen_get_root_window(gs);

    GdkDeviceManager * gdm = gdk_display_get_device_manager(gd);
    GdkDevice * pointer = gdk_device_manager_get_client_pointer(gdm);

    gint x, y;
    gdk_window_get_device_position(rw, pointer, &x, &y, NULL);

    gtk_window_move(GTK_WINDOW(window), x + 1, y - 1);

    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

    GdkColor bg = {0x0, 0xffff, 0xffff, 0xffff};
    gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &bg);

    gtk_widget_show(label);
    gtk_widget_show(GTK_WIDGET(window));
    
    return window;
}



struct _rdgwindow * rdgwindow_create (struct _gui * gui,
                                      struct _graph * graph,
                                      int type,
                                      uint64_t top_index)
{
    struct _rdgwindow * rdgwindow;

    rdgwindow = (struct _rdgwindow *) malloc(sizeof(struct _rdgwindow));
    rdgwindow->window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    rdgwindow->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    rdgwindow->imageEventBox  = gtk_event_box_new();
    rdgwindow->image          = gtk_image_new_from_file("logo.png");
    rdgwindow->menu_popup     = gtk_menu_new();

    rdgwindow->gui            = gui;
    rdgwindow->gui_identifier = gui_add_window(rdgwindow->gui, rdgwindow->window);

    rdgwindow->graph          = object_copy(graph);

    rdgwindow->image_drag_x   = 0;
    rdgwindow->image_drag_y   = 0;
    rdgwindow->image_dragging = 0;

    rdgwindow->selected_node  = -1;
    rdgwindow->selected_ins   = -1;
    rdgwindow->node_colors    = NULL;

    rdgwindow->editing        = 0;
    rdgwindow->tooltip        = NULL;

    rdgwindow->type           = type;
    rdgwindow->top_index      = top_index;

    rdgwindow->rdg = rdg_create(rdgwindow->top_index,
                                rdgwindow->graph,
                                rdgwindow->gui->rdis->labels);

    // popup menu stuff
    GtkWidget * menuItem = gtk_menu_item_new_with_label(LANG_USERFUNCTION);
    g_signal_connect(menuItem,
                     "activate",
                     G_CALLBACK(rdgwindow_user_function),
                     rdgwindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(rdgwindow->menu_popup), menuItem);
    menuItem = gtk_menu_item_new_with_label(LANG_REMOVEALLAFTER);
    g_signal_connect(menuItem,
                     "activate",
                     G_CALLBACK(rdgwindow_remove_all_after),
                     rdgwindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(rdgwindow->menu_popup), menuItem);

    gtk_widget_show_all(rdgwindow->menu_popup);




    gtk_container_add(GTK_CONTAINER(rdgwindow->imageEventBox),
                      rdgwindow->scrolledWindow);

    gtk_scrolled_window_add_with_viewport(
        GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow), rdgwindow->image);

    gtk_container_add(GTK_CONTAINER(rdgwindow->window), rdgwindow->imageEventBox);


    g_signal_connect(rdgwindow->window,
                     "destroy",
                     G_CALLBACK(rdgwindow_destroy_event),
                     rdgwindow);

    g_signal_connect(rdgwindow->imageEventBox,
                     "motion-notify-event",
                     G_CALLBACK(rdgwindow_image_motion_notify_event),
                     rdgwindow);

    g_signal_connect(rdgwindow->imageEventBox,
                     "button-press-event",
                     G_CALLBACK(rdgwindow_image_button_press_event),
                     rdgwindow);

    g_signal_connect(rdgwindow->imageEventBox,
                     "button-release-event",
                     G_CALLBACK(rdgwindow_image_button_release_event),
                     rdgwindow);

    g_signal_connect(rdgwindow->imageEventBox,
                     "key-press-event",
                     G_CALLBACK(rdgwindow_image_key_press_event),
                     rdgwindow);

    g_signal_connect(rdgwindow->scrolledWindow,
                     "size-allocate",
                     G_CALLBACK(rdgwindow_size_allocate_event),
                     rdgwindow);


    gtk_widget_show(rdgwindow->image);
    gtk_widget_show(rdgwindow->scrolledWindow);
    gtk_widget_show(rdgwindow->imageEventBox);

    gtk_widget_set_events(rdgwindow->imageEventBox,
                          gtk_widget_get_events(rdgwindow->imageEventBox)
                          | GDK_POINTER_MOTION_MASK);


    rdgwindow_image_update(rdgwindow);

    int width  = rdg_width(rdgwindow->rdg);
    int height = rdg_height(rdgwindow->rdg);

    if (width > RDGWINDOW_MAX_DEFAULT_WIDTH)
        width = RDGWINDOW_MAX_DEFAULT_WIDTH;
    if (height > RDGWINDOW_MAX_DEFAULT_HEIGHT)
        height = RDGWINDOW_MAX_DEFAULT_HEIGHT;

    gtk_window_set_default_size(GTK_WINDOW(rdgwindow->window), width, height);

    rdgwindow->callback_identifier = rdis_add_callback(rdgwindow->gui->rdis,
                                        RDIS_CALLBACK(rdgwindow_rdis_callback),
                                        rdgwindow,
                                        RDIS_CALLBACK_GRAPH_NODE
                                        | RDIS_CALLBACK_LABEL);

    gtk_widget_show_all(rdgwindow->window);
    rdgwindow_center_on_node(rdgwindow, top_index);

    return rdgwindow;
}


void rdgwindow_delete (struct _rdgwindow * rdgwindow)
{
    if (rdgwindow->rdg != NULL)
        object_delete(rdgwindow->rdg);

    if (rdgwindow->graph != NULL)
        object_delete(rdgwindow->graph);

    if (rdgwindow->node_colors != NULL)
        object_delete(rdgwindow->node_colors);

    if (rdgwindow->tooltip != NULL)
        gtk_widget_destroy(rdgwindow->tooltip);

    gui_remove_window(rdgwindow->gui, rdgwindow->gui_identifier);
    rdis_remove_callback(rdgwindow->gui->rdis, rdgwindow->callback_identifier);

    gtk_widget_destroy(rdgwindow->menu_popup);

    //gtk_widget_destroy(rdgwindow->window);

    free(rdgwindow);
}


GtkWidget * rdgwindow_window (struct _rdgwindow * rdgwindow)
{
    return rdgwindow->window;
}


void rdgwindow_image_update (struct _rdgwindow * rdgwindow)
{
    rdg_draw(rdgwindow->rdg);
    GdkPixbuf * pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(rdgwindow->rdg->surface,
                                         0,
                                         0,
                                         rdg_width(rdgwindow->rdg),
                                         rdg_height(rdgwindow->rdg));
    gtk_image_set_from_pixbuf(GTK_IMAGE(rdgwindow->image), pixbuf);
    g_object_unref(pixbuf);
    while (gtk_events_pending())
        gtk_main_iteration();
}



void rdgwindow_destroy_event (GtkWidget * widget, struct _rdgwindow * rdgwindow)
{
    rdgwindow_delete(rdgwindow);
}



gboolean rdgwindow_image_motion_notify_event (GtkWidget * widget,
                                              GdkEventMotion * event,
                                              struct _rdgwindow * rdgwindow)
{
    if (rdgwindow->tooltip != NULL) {
        gtk_widget_destroy(rdgwindow->tooltip);
        rdgwindow->tooltip = NULL;
    }

    GtkAdjustment * hadjust = gtk_scrolled_window_get_hadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));
    GtkAdjustment * vadjust = gtk_scrolled_window_get_vadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));

    // set hover instruction
    int image_x = (int) event->x + (int) gtk_adjustment_get_value(hadjust);
    int image_y = (int) event->y + (int) gtk_adjustment_get_value(vadjust);

    if (rdgwindow->scrolledWindow_width > rdg_width(rdgwindow->rdg))
        image_x -= (rdgwindow->scrolledWindow_width
                    - rdg_width(rdgwindow->rdg)) / 2;

    if (rdgwindow->scrolledWindow_height > rdg_height(rdgwindow->rdg))
        image_y -= (rdgwindow->scrolledWindow_height
                    - rdg_height(rdgwindow->rdg)) / 2;

    uint64_t hover_ins = rdg_get_ins_by_coords(rdgwindow->rdg,
                                               rdgwindow->gui->rdis->graph,
                                               rdgwindow->gui->rdis->labels,
                                               image_x, image_y);

    if (hover_ins != -1) {
        struct _ins * ins = graph_fetch_ins(rdgwindow->graph, hover_ins);
        if (ins == NULL) {
            printf("could not get hover_ins %llx\n",
            (unsigned long long) hover_ins);
            return FALSE;
        }
        if (ins->references->size > 0) {
            char references_text[512];
            strcpy(references_text, "<span background=\"#ffffff\" font_family=\"monospace\" font=\"9.0\">");
            struct _list_it * it;
            for (it = list_iterator(ins->references); it != NULL; it = it->next) {
                struct _reference * reference = it->data;
                char reference_text[128];

                snprintf(reference_text, 128,
                         "<span foreground=\"#000099\">%04llx</span> ",
                         (unsigned long long) reference->address);

                size_t i;
                for (i = 0; i < 16; i++) {
                    int c = mem_map_byte(rdgwindow->gui->rdis->memory,
                                         reference->address + i);
                    if (c == -1) {
                        break;
                    }

                    char hex[16];
                    if ((i + 1) % 4 == 0)
                        snprintf(hex, 16, "%02x ", c);
                    else
                        snprintf(hex, 16, "%02x", c);
                    rdstrcat(reference_text, hex, 128);
                }
                rdstrcat(references_text, reference_text, 512);

                char ascii[65];
                int ascii_valid = 1;
                int ai = 0;
                for (i = 0; i < 64; i++) {
                    int c = mem_map_byte(rdgwindow->gui->rdis->memory,
                                         reference->address + i);
                    if (c == -1) {
                        ascii_valid = 0;
                        break;
                    }

                    if (c == 0) {
                        if (i > 4) {
                            ascii[ai] = 0;
                            break;
                        }
                        else {
                            ascii_valid = 0;
                            break;
                        }
                    }

                    if ((c != '\n') && ((c < 0x20) || (c >= 0x7f))) {
                        ascii_valid = 0;
                        break;
                    }

                    if (c == '<') {
                        ascii[ai++] = '&';
                        ascii[ai++] = 'l';
                        ascii[ai++] = 't';
                        ascii[ai++] = ';';
                    }
                    else if (c == '>') {
                        ascii[ai++] = '&';
                        ascii[ai++] = 'g';
                        ascii[ai++] = 't';
                        ascii[ai++] = ';';
                    }
                    else if (c == '&') {
                        ascii[ai++] = '&';
                        ascii[ai++] = 'a';
                        ascii[ai++] = 'm';
                        ascii[ai++] = 'p';
                        ascii[ai++] = ';';
                    }
                    else
                        ascii[ai++] = c;
                }
                ascii[ai] = 0;
                if (ascii_valid) {
                    rdstrcat(references_text, "\n    <span foreground=\"#009900\">", 512);
                    rdstrcat(references_text, ascii, 512);
                    rdstrcat(references_text, "</span>", 512);
                }

                if (it->next != NULL)
                    rdstrcat(references_text, "\n", 512);
            }
            rdstrcat(references_text, "</span>", 512);
            rdgwindow->tooltip = tipwindow(references_text);
        }
    }
    else
        gtk_widget_set_has_tooltip(rdgwindow->imageEventBox, FALSE);

    if (rdgwindow->image_dragging == 0)
        return TRUE;

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

    return FALSE;
}


gboolean rdgwindow_image_button_press_event  (GtkWidget * widget,
                                              GdkEventButton * event,
                                              struct _rdgwindow * rdgwindow)
{
    int x = (int) event->x;
    int y = (int) event->y;

    if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1))
        rdgwindow->image_dragging = 1;

    rdgwindow->image_drag_x = x;
    rdgwindow->image_drag_y = y;

    GtkAdjustment * hadjust = gtk_scrolled_window_get_hadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));
    GtkAdjustment * vadjust = gtk_scrolled_window_get_vadjustment(
                          GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));

    // these are the x,y coords inside the image
    int image_x = x + (int) gtk_adjustment_get_value(hadjust);
    int image_y = y + (int) gtk_adjustment_get_value(vadjust);

    if (rdgwindow->scrolledWindow_width > rdg_width(rdgwindow->rdg))
        image_x -= (rdgwindow->scrolledWindow_width
                    - rdg_width(rdgwindow->rdg)) / 2;

    if (rdgwindow->scrolledWindow_height > rdg_height(rdgwindow->rdg))
        image_y -= (rdgwindow->scrolledWindow_height
                    - rdg_height(rdgwindow->rdg)) / 2;

    printf("image_x: %d, image_y: %d\n",
           image_x, image_y);

    uint64_t selected_node = rdg_get_node_by_coords(rdgwindow->rdg,
                                                    image_x, image_y);

    uint64_t selected_ins = rdg_get_ins_by_coords(rdgwindow->rdg,
                                                  rdgwindow->gui->rdis->graph,
                                                  rdgwindow->gui->rdis->labels,
                                                  image_x, image_y);

    if (selected_node == -1)
        return FALSE;
    else if (selected_ins == -1)
        rdgwindow->selected_ins = -1;
    else
        rdgwindow->selected_ins = selected_ins;

    rdgwindow->selected_node = selected_node;

    rdgwindow_color_node(rdgwindow);

    printf("selected_ins: %llx, event->button: %d\n",
           (unsigned long long) selected_ins,
           event->button);

    // right click for popup menu
    if ((selected_ins != -1) && (event->button == 3)) {
        rdgwindow_menu_popup(rdgwindow);
    }

    // double click on an instruction of type CALL spawns an rdgwindow for
    // that instruction
    if ((rdgwindow->selected_ins != -1) && (event->type == GDK_2BUTTON_PRESS)) {
        struct _graph_node * node = graph_fetch_node(rdgwindow->gui->rdis->graph,
                                                     rdgwindow->selected_node);
        if (node == NULL) {
            printf("double-click node %llx didn't find node\n",
                   (unsigned long long) rdgwindow->selected_node);
            return FALSE;
        }
        struct _ins * ins = graph_node_ins(node, rdgwindow->selected_ins);
        if (ins == NULL) {
            printf("double-click ins %llx didn't find ins\n",
                   (unsigned long long) rdgwindow->selected_ins);
            return FALSE;
        }

        // if this ins is a call and the target is set
        if ((ins->flags & INS_CALL) && (ins->flags & INS_TARGET_SET)) {
            struct _graph * family = graph_family(rdgwindow->gui->rdis->graph,
                                                  ins->target);
            if (family == NULL) {
                printf("could not find graph family for call target (call at %llx\n",
                       (unsigned long long) rdgwindow->selected_ins);
                return FALSE;
            }

            gui_rdgwindow(rdgwindow->gui,
                          family,
                          RDGWINDOW_INS_GRAPH,
                          ins->target);
            object_delete(family);
        }
    }

    return FALSE;
}


gboolean rdgwindow_image_button_release_event (GtkWidget * widget,
                                               GdkEventButton * event,
                                               struct _rdgwindow * rdgwindow)
{
    rdgwindow->image_dragging = 0;

    return FALSE;
}


gboolean rdgwindow_image_key_press_event (GtkWidget * widget,
                                          GdkEventKey * event,
                                          struct _rdgwindow * rdgwindow)
{
    // key 'p'
    if ((event->keyval == ';') && (rdgwindow->selected_ins != -1)) {
        rdgwindow->editing = 1;
        return FALSE;
    }

    else if (event->keyval == GDK_KEY_Return) {
        rdgwindow->editing = 0;
        return FALSE;
    }

    if (rdgwindow->editing) {
        if ((event->keyval >= 0x20) && (event->keyval < 0x7f)) {
            struct _graph_node * node = graph_fetch_node(rdgwindow->gui->rdis->graph,
                                                         rdgwindow->selected_node);
            // find the selected instruction
            struct _list_it * it;
            struct _list * ins_list = node->data;
            for (it = list_iterator(ins_list); it != NULL; it = it->next) {
                struct _ins * ins = it->data;
                if (ins->address != rdgwindow->selected_ins)
                    continue;

                char tmpc[4];
                sprintf(tmpc, "%c", event->keyval);
                if (ins->comment == NULL) {
                    ins_s_comment(ins, tmpc);
                }
                else {
                    char * tmp = (char *) malloc(strlen(ins->comment) + 2);
                    strcpy(tmp, ins->comment);
                    strcat(tmp, tmpc);
                    ins_s_comment(ins, tmp);
                    free(tmp);
                }
                break;
            }
        }
        else if (event->keyval == GDK_KEY_BackSpace) {
            struct _graph_node * node = graph_fetch_node(rdgwindow->gui->rdis->graph,
                                                         rdgwindow->selected_node);
            struct _list_it * it;
            struct _list * ins_list = node->data;
            for (it = list_iterator(ins_list); it != NULL; it = it->next) {
                struct _ins * ins = it->data;
                if (ins->address != rdgwindow->selected_ins)
                    continue;

                if (ins->comment == NULL)
                    break;

                if (strlen(ins->comment) == 0)
                    break;

                char * tmp = strdup(ins->comment);
                tmp[strlen(tmp) - 1] = '\0';
                ins_s_comment(ins, tmp);
                free(tmp);
            }
        }
        // this callback will cause us to redraw the graph
        rdis_callback(rdgwindow->gui->rdis, RDIS_CALLBACK_GRAPH_NODE);
        return FALSE;
    }

    if (event->keyval == 0x70) {
        rdgwindow_color_node_predecessors(rdgwindow);
    }

    return FALSE;
}


void rdgwindow_size_allocate_event (GtkWidget * widget,
                                    GdkRectangle * allocation,
                                    struct _rdgwindow * rdgwindow)
{
    rdgwindow->scrolledWindow_width  = allocation->width;
    rdgwindow->scrolledWindow_height = allocation->height;
}


void rdgwindow_center_on_node (struct _rdgwindow * rdgwindow, uint64_t index)
{
    GtkAdjustment * ha = gtk_scrolled_window_get_hadjustment(
                             GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));
    GtkAdjustment * va = gtk_scrolled_window_get_vadjustment(
                             GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow));

    double width  = rdg_width(rdgwindow->rdg);
    double height = rdg_height(rdgwindow->rdg);
    int    xx     = rdg_node_x(rdgwindow->rdg, index);
    int    yy     = rdg_node_y(rdgwindow->rdg, index);

    printf("xx=%d, yy=%d\n", xx, yy);

    if ((xx == -1) || (yy == -1))
        return;

    double x = xx;
    double y = yy;

    gtk_adjustment_set_value(ha, gtk_adjustment_get_upper(ha) * (x / width));
    gtk_adjustment_set_value(va, gtk_adjustment_get_upper(va) * (y / height));

    gtk_scrolled_window_set_hadjustment(
                             GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow), ha);
    gtk_scrolled_window_set_vadjustment(
                             GTK_SCROLLED_WINDOW(rdgwindow->scrolledWindow), va);
}


// sets rdgwindow->node_colors to a list that resets all nodes to the neutral color
void rdgwindow_reset_node_colors (struct _rdgwindow * rdgwindow)
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

    rdg_custom_nodes(rdgwindow->rdg,
                     rdgwindow->graph,
                     rdgwindow->gui->rdis->labels,
                     node_colors,
                     rdgwindow->selected_ins);

    object_delete(node_colors);
    rdgwindow->node_colors = list_create();
}


void rdgwindow_color_node (struct _rdgwindow * rdgwindow)
{
    rdgwindow_reset_node_colors(rdgwindow);

    struct _rdg_node_color * rdg_node_color;
    rdg_node_color = rdg_node_color_create(rdgwindow->selected_node,
                                           RDGWINDOW_NODE_COLOR_SELECT);
    list_append(rdgwindow->node_colors, rdg_node_color);

    rdg_custom_nodes(rdgwindow->rdg,
                     rdgwindow->graph,
                     rdgwindow->gui->rdis->labels,
                     rdgwindow->node_colors,
                     rdgwindow->selected_ins);
    rdgwindow_image_update(rdgwindow);
}


void rdgwindow_color_node_predecessors (struct _rdgwindow * rdgwindow)
{
    if (rdgwindow->selected_node == -1)
        return;

    struct _tree  * pre_tree = tree_create();
    struct _queue * queue    = queue_create();

    // add currently selected node to queue
    struct _index * index = index_create(rdgwindow->selected_node);
    queue_push(queue, index);
    object_delete(index);

    // add predecessors to the queue
    while (queue->size > 0) {
        index = queue_peek(queue);
        
        // if we've already added this node, skip it
        if (tree_fetch(pre_tree, index) != NULL) {
            queue_pop(queue);
            continue;
        }

        tree_insert(pre_tree, index);

        struct _graph_node * node = graph_fetch_node(rdgwindow->gui->rdis->graph,
                                                     index->index);
        struct _list * predecessors = graph_node_predecessors(node);
        struct _list_it * it;
        for (it = list_iterator(predecessors); it != NULL; it = it->next) {
            struct _graph_edge * edge = it->data;
            index = index_create(edge->head);
            queue_push(queue, index);
            index_delete(index);
        }

        queue_pop(queue);
    }

    object_delete(queue);

    rdgwindow_reset_node_colors(rdgwindow);

    // add predecessors to node_colors
    struct _tree_it * it;
    for (it = tree_iterator(pre_tree); it != NULL; it = tree_it_next(it)) {
        index = tree_it_data(it);

        struct _rdg_node_color * rdg_node_color;
        rdg_node_color = rdg_node_color_create(index->index,
                                               RDGWINDOW_NODE_COLOR_PRE);
        list_append(rdgwindow->node_colors, rdg_node_color);
        object_delete(rdg_node_color);
    }

    object_delete(pre_tree);

    rdg_color_nodes(rdgwindow->rdg,
                    rdgwindow->graph,
                    rdgwindow->gui->rdis->labels,
                    rdgwindow->node_colors);
    rdgwindow_image_update(rdgwindow);
}


void rdgwindow_rdis_callback (struct _rdgwindow * rdgwindow)
{
    printf("rdgwindow_rdis_callback\n");
    // reconstruct graph family from top index
    if (rdgwindow->type == RDGWINDOW_INS_GRAPH) {
        struct _graph * graph = graph_family(rdgwindow->gui->rdis->graph,
                                             rdgwindow->top_index);
        if (graph == NULL)
            graph = graph_create();
        object_delete(rdgwindow->graph);
        rdgwindow->graph = graph;
        object_delete(rdgwindow->rdg);
        rdgwindow->rdg = rdg_create(rdgwindow->top_index,
                                    rdgwindow->graph,
                                    rdgwindow->gui->rdis->labels);
        rdgwindow_image_update(rdgwindow);
    }
    else
        rdgwindow_image_update(rdgwindow);
}



void rdgwindow_menu_popup (struct _rdgwindow * rdgwindow)
{
    gtk_menu_popup(GTK_MENU(rdgwindow->menu_popup),
                   NULL, NULL, NULL,
                   rdgwindow,
                   0,
                   gtk_get_current_event_time());
}



void rdgwindow_user_function (GtkMenuItem * menuItem,
                              struct _rdgwindow * rdgwindow)
{
    if (rdgwindow->selected_ins != -1)
    rdis_user_function(rdgwindow->gui->rdis, rdgwindow->selected_ins);

    printf("user function click on %llx\n",
           (unsigned long long) rdgwindow->selected_ins);
}



void rdgwindow_remove_all_after (GtkMenuItem * menuItem,
                                 struct _rdgwindow * rdgwindow)
{
    if (rdgwindow->selected_ins != -1) {
        struct _graph_node * node;
        node = graph_fetch_node(rdgwindow->gui->rdis->graph, rdgwindow->selected_node);
        if (node != NULL) {
            remove_all_after(node, rdgwindow->selected_ins);
            rdis_callback(rdgwindow->gui->rdis, RDIS_CALLBACK_GRAPH | RDIS_CALLBACK_GRAPH_NODE);
        }
    }

    printf("remove all after click on %llx\n",
           (unsigned long long) rdgwindow->selected_ins);
}
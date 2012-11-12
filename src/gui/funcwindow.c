#include "funcwindow.h"

#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>

#include "gui.h"
#include "index.h"
#include "label.h"
#include "tree.h"
#include "util.h"

enum {
    COL_INDEX,
    COL_ADDR,
    COL_LABEL,
    COL_N
};


struct _funcwindow * funcwindow_create (struct _gui * gui)
{
    struct _funcwindow * funcwindow;

    funcwindow = (struct _funcwindow *) malloc(sizeof(struct _funcwindow));
    funcwindow->window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    funcwindow->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    funcwindow->listStore      = gtk_list_store_new(COL_N,
                                                    G_TYPE_UINT64,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING);
    funcwindow->treeView       = gtk_tree_view_new_with_model(
                                        GTK_TREE_MODEL(funcwindow->listStore));
    funcwindow->gui            = gui;
    funcwindow->menu_popup      = gtk_menu_new();


    // popup menu stuff
    GtkWidget * menuItem = gtk_menu_item_new_with_label("Call Graph");
    g_signal_connect(menuItem,
                     "activate",
                     G_CALLBACK(funcwindow_call_graph),
                     funcwindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(funcwindow->menu_popup), menuItem);

    gtk_widget_show_all(funcwindow->menu_popup);


    // add this window to the gui window list so gui can close it
    funcwindow->gui_identifier = gui_add_window(gui, funcwindow->window);

    // add data to tree model
    struct _tree_it * tree_it;
    for (tree_it = tree_iterator(funcwindow->gui->rdis->function_tree);
         tree_it != NULL;
         tree_it = tree_it_next(tree_it)) {
        struct _index * index = tree_it_data(tree_it);

        funcwindow_append_row(funcwindow, index->index);
    }

    // add columns to tree view
    GtkCellRenderer   * renderer;
    GtkTreeViewColumn * column;
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("address",
                                                        renderer,
                                                        "text", COL_ADDR,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(funcwindow->treeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("name",
                                                        renderer,
                                                        "text", COL_LABEL,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    g_object_set(renderer, "editable", TRUE, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(funcwindow->treeView), column);
    g_signal_connect(renderer,
                     "edited",
                     G_CALLBACK(funcwindow_edited),
                     funcwindow);


    // set up containers
    gtk_container_add(GTK_CONTAINER(funcwindow->scrolledWindow),
                      funcwindow->treeView);
    gtk_container_add(GTK_CONTAINER(funcwindow->window),
                      funcwindow->scrolledWindow);

    // signals
    g_signal_connect(funcwindow->window,
                     "destroy",
                     G_CALLBACK(funcwindow_destroy_event),
                     funcwindow);

    g_signal_connect(funcwindow->treeView,
                     "button-press-event",
                     G_CALLBACK(funcwindow_button_press_event),
                     funcwindow);

    g_signal_connect(funcwindow->treeView,
                     "row-activated",
                     G_CALLBACK(funcwindow_row_activated),
                     funcwindow);

    gtk_window_set_default_size(GTK_WINDOW(funcwindow->window), 280, 480);

    gtk_widget_show(funcwindow->treeView);
    gtk_widget_show(funcwindow->scrolledWindow);

    funcwindow->callback_identifier = rdis_add_callback(funcwindow->gui->rdis,
                                       RDIS_CALLBACK(funcwindow_rdis_callback),
                                                        funcwindow);
    
    return funcwindow;
}



void funcwindow_delete (struct _funcwindow * funcwindow)
{
    //gtk_widget_destroy(funcwindow->window);
    rdis_remove_callback(funcwindow->gui->rdis, funcwindow->callback_identifier);
    gui_remove_window(funcwindow->gui, funcwindow->gui_identifier);
    free(funcwindow);
}



GtkWidget * funcwindow_window (struct _funcwindow * funcwindow)
{
    return funcwindow->window;
}



void funcwindow_append_row (struct _funcwindow * funcwindow, uint64_t index)
{

    GtkTreeIter treeIter;
    struct _label * label = map_fetch(funcwindow->gui->rdis->labels, index);


    if (label == NULL) {
        printf("funcwindow null label for %llx\n",
               (unsigned long long) index);
        return;
    }

    char addrText[64];
    snprintf(addrText, 64, "%04llx", (unsigned long long) index);

    gtk_list_store_append(funcwindow->listStore, &treeIter);
    gtk_list_store_set(funcwindow->listStore, &treeIter,
                       COL_INDEX,  index,
                       COL_ADDR,   addrText,
                       COL_LABEL,  label->text,
                       -1);
}



void funcwindow_destroy_event (GtkWidget * widget,
                               struct _funcwindow * funcwindow)
{
    funcwindow_delete(funcwindow);
}



void funcwindow_row_activated (GtkTreeView * treeView,
                                           GtkTreePath * treePath,
                                           GtkTreeViewColumn * treeViewColumn,
                                           struct _funcwindow * funcwindow)
{
    uint64_t index;

    GtkTreeIter treeIter;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(funcwindow->listStore),
                            &treeIter,
                            treePath);
    gtk_tree_model_get(GTK_TREE_MODEL(funcwindow->listStore),
                       &treeIter,
                       COL_INDEX, &index,
                       -1);

    printf("funcwindow_row_activated %llx\n", (unsigned long long) index);

    struct _graph * graph = graph_family(funcwindow->gui->rdis->graph, index);
    if (graph != NULL) {
        gui_rdgwindow(funcwindow->gui, graph);
        object_delete(graph);
    }
    else {
        char tmp[128];
        snprintf(tmp, 128, "Could not create graph family for %llx\n",
                 (unsigned long long) index);
        gui_console(funcwindow->gui, tmp);
    }
}



void funcwindow_edited (GtkCellRendererText * renderer,
                        char * path,
                        char * new_text,
                        struct _funcwindow * funcwindow)
{
    // get the index that was updated
    GtkTreeIter treeIter;

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(funcwindow->listStore),
                                        &treeIter,
                                        path);

    uint64_t index;
    gtk_tree_model_get(GTK_TREE_MODEL(funcwindow->listStore),
                       &treeIter,
                       COL_INDEX, &index,
                       -1);

    // update this label
    struct _label * label = map_fetch(funcwindow->gui->rdis->labels, index);
    if (label == NULL) {
        printf("failed to find label for %llx\n",
               (unsigned long long) index);
        return;
    }

    label_set_text(label, new_text);

    rdis_callback(funcwindow->gui->rdis);
}



void funcwindow_rdis_callback (struct _funcwindow * funcwindow)
{
    // remove all rows in the funcwindow
    gtk_list_store_clear(funcwindow->listStore);

    // add in all the functions
    struct _tree_it * it;
    for (it  = tree_iterator(funcwindow->gui->rdis->function_tree);
         it != NULL;
         it  = tree_it_next(it)) {
        struct _index * index = tree_it_data(it);
        funcwindow_append_row(funcwindow, index->index);
    }
}


gboolean funcwindow_button_press_event  (GtkWidget * widget,
                                               GdkEventButton * event,
                                               struct _funcwindow * funcwindow)
{
    GtkTreeSelection * selection;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(funcwindow->treeView));

    if (    (gtk_tree_selection_count_selected_rows(selection) > 0)
         && (event->button == 3))
        gtk_menu_popup(GTK_MENU(funcwindow->menu_popup),
               NULL, NULL, NULL,
               funcwindow,
               0,
               gtk_get_current_event_time());

    return FALSE;
}


void funcwindow_call_graph (GtkMenuItem * menuItem,
                            struct _funcwindow * funcwindow)
{
    printf("funcwindow call graph\n");

    GtkTreeSelection * selection;
    GtkTreeIter treeIter;
    GtkTreeModel * model = GTK_TREE_MODEL(funcwindow->listStore);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(funcwindow->treeView));

    if (gtk_tree_selection_get_selected(selection,
                                        &model,
                                        &treeIter)) {
        uint64_t index;
        gtk_tree_model_get(GTK_TREE_MODEL(funcwindow->listStore),
                           &treeIter,
                           COL_INDEX, &index,
                           -1);
        printf("selected %llx\n", (unsigned long long) index);

        if (graph_fetch_node(funcwindow->gui->rdis->graph, index) == NULL) {
            printf("we are in trouble, not found %llx\n", (unsigned long long) index);
        }
        else
            printf("found %llx\n", (unsigned long long) index);

        struct _graph * graph = create_call_graph(funcwindow->gui->rdis->graph, index);

        graph_debug(graph);

        gui_rdgwindow(funcwindow->gui, graph);

        object_delete(graph);
    }
}
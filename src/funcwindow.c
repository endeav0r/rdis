#include "funcwindow.h"

#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>

#include "index.h"
#include "label.h"
#include "tree.h"

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

    // add data to tree model
    GtkTreeIter treeIter;
    struct _tree_it * tree_it;
    for (tree_it = tree_iterator(funcwindow->gui->function_tree);
         tree_it != NULL;
         tree_it = tree_it_next(tree_it)) {
        struct _index * index = tree_it_data(tree_it);

        struct _label * label = map_fetch(funcwindow->gui->labels, index->index);

        char addrText[64];
        snprintf(addrText, 64, "%04llx", (unsigned long long) index->index);

        gtk_list_store_append(funcwindow->listStore, &treeIter);
        gtk_list_store_set(funcwindow->listStore, &treeIter,
                           COL_INDEX,  index->index,
                           COL_ADDR,   addrText,
                           COL_LABEL,  label->text,
                           -1);
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
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(funcwindow->treeView), column);


    // set up containers
    gtk_container_add(GTK_CONTAINER(funcwindow->scrolledWindow),
                      funcwindow->treeView);
    gtk_container_add(GTK_CONTAINER(funcwindow->window),
                      funcwindow->scrolledWindow);

    // signals

    g_signal_connect(funcwindow->treeView,
                     "row-activated",
                     G_CALLBACK(funcwindow_row_activated),
                     funcwindow);

    gtk_window_set_default_size(GTK_WINDOW(funcwindow->window), 280, 480);

    gtk_widget_show(funcwindow->treeView);
    gtk_widget_show(funcwindow->scrolledWindow);
    
    return funcwindow;
}



void funcwindow_delete (struct _funcwindow * funcwindow)
{
    gtk_widget_destroy(funcwindow->window);
    free(funcwindow);
}



GtkWidget * funcwindow_window (struct _funcwindow * funcwindow)
{
    return funcwindow->window;
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

    gui_rdgwindow(funcwindow->gui, index);
}
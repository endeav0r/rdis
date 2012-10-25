#include "inswindow.h"

// for debugging
#include <cairo.h>

#include <gdk/gdk.h>
#include <stdio.h>
#include <string.h>

#include "instruction.h"
#include "object.h"
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


struct _inswindow * inswindow_create (struct _gui * gui)
{
    struct _inswindow * inswindow;

    inswindow = (struct _inswindow *) malloc(sizeof(struct _inswindow));
    inswindow->window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    inswindow->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    inswindow->listStore      = gtk_list_store_new(COL_N, 
                                                   G_TYPE_UINT64,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING);
    inswindow->treeView       = gtk_tree_view_new_with_model(
                                         GTK_TREE_MODEL(inswindow->listStore));

    inswindow->gui            = gui;

    GtkTreeIter treeIter;

    // add all the instructions to the listStore
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(inswindow->gui->rdis->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {

        struct _list_it * list_it;
        for (list_it = list_iterator(graph_it_data(graph_it));
             list_it != NULL;
             list_it = list_it->next) {

            struct _ins * ins = list_it->data;

            // prepare text based off instruction
            char addrText[64];
            snprintf(addrText, 64, "%04llx", (unsigned long long) ins->address);


            char   bytesText[64];
            char   tmp[8];
            size_t i;
            memset(bytesText, 0, 64);
            for (i = 0; i < (ins->size < 8 ? ins->size : 8); i++) {
                snprintf(tmp, 8, "%02x", ins->bytes[i]);
                strncat(bytesText, tmp, 63);
            }
            bytesText[63] = '\0';

            // add text to tree model
            gtk_list_store_append(inswindow->listStore, &treeIter);
            gtk_list_store_set(inswindow->listStore, &treeIter,
                               COL_INDEX,       ins->address,
                               COL_ADDR,        addrText,
                               COL_BYTES,       bytesText,
                               COL_DESCRIPTION, ins->description,
                               COL_COMMENT,     ins->comment,
                               -1);
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
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->treeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("bytes",
                                                        renderer,
                                                        "text", COL_BYTES,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->treeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("description",
                                                        renderer,
                                                        "text", COL_DESCRIPTION,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->treeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("comment",
                                                        renderer,
                                                        "text", COL_COMMENT,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inswindow->treeView), column);


    /*
    * Other Init
    */ 

    // add things inside containers
    gtk_container_add(GTK_CONTAINER(inswindow->scrolledWindow),
                      inswindow->treeView);

    gtk_container_add(GTK_CONTAINER(inswindow->window),
                      inswindow->scrolledWindow);

    // signal some stuff
    g_signal_connect(inswindow->treeView,
                     "row-activated",
                     G_CALLBACK(inswindow_row_activated),
                     inswindow);

    // set more options
    gtk_window_set_default_size(GTK_WINDOW(inswindow->window), 800, 600);

    // show ALL THE WIDGETS
    gtk_widget_show(inswindow->scrolledWindow);
    gtk_widget_show(inswindow->treeView);

    return inswindow;
}



void inswindow_delete (struct _inswindow * inswindow)
{
    gtk_widget_destroy(inswindow->window);
    free(inswindow);
}



GtkWidget * inswindow_window (struct _inswindow * inswindow)
{
    return inswindow->window;
}



void inswindow_insTreeView_select_index (struct _inswindow * inswindow,
                                         uint64_t index)
{
    GtkTreeIter treeIter;
    uint64_t store_index;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(inswindow->listStore),
                                      &treeIter) == FALSE)
        return;

    while (1) {
        gtk_tree_model_get(GTK_TREE_MODEL(inswindow->listStore),
                           &treeIter,
                           COL_INDEX,
                           &store_index, -1);
        if (store_index == index) {
            gtk_tree_view_set_cursor(
                                     GTK_TREE_VIEW(inswindow->treeView),
                                     gtk_tree_model_get_path(
                                       GTK_TREE_MODEL(inswindow->listStore),
                                                             &treeIter),
                                     NULL,
                                     0);
            break;
        }
        if (gtk_tree_model_iter_next(GTK_TREE_MODEL(inswindow->listStore),
                                     &treeIter) == FALSE)
            break;
    }
}



void inswindow_row_activated (GtkTreeView * treeView,
                                       GtkTreePath * treePath,
                                       GtkTreeViewColumn * treeViewColumn,
                                       struct _inswindow * inswindow)
{
    uint64_t index;

    GtkTreeIter treeIter;

    // get the index
    gtk_tree_model_get_iter(GTK_TREE_MODEL(inswindow->listStore),
                            &treeIter,
                            treePath);
    gtk_tree_model_get(GTK_TREE_MODEL(inswindow->listStore),
                       &treeIter,
                       COL_INDEX,
                       &index, -1);
}
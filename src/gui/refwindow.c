#include "refwindow.h"

#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "index.h"
#include "label.h"
#include "tree.h"
#include "util.h"

enum {
    COL_ADDR_UINT64,
    COL_ADDRESS,
    COL_REFERENCERS,
    COL_N
};


struct _refwindow * refwindow_create (struct _gui * gui)
{
    struct _refwindow * refwindow;

    refwindow = (struct _refwindow *) malloc(sizeof(struct _refwindow));
    refwindow->window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    refwindow->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    refwindow->listStore      = gtk_list_store_new(COL_N,
                                                   G_TYPE_UINT64,
                                                   G_TYPE_STRING,
                                                   G_TYPE_STRING);
    refwindow->treeView       = gtk_tree_view_new_with_model(
                                         GTK_TREE_MODEL(refwindow->listStore));
    refwindow->gui            = gui;

    // add this window to the gui window list so gui can close it
    refwindow->gui_identifier = gui_add_window(gui, refwindow->window);

    // add data to tree model
    refwindow_refresh (refwindow);

    // add columns to tree view
    GtkCellRenderer   * renderer;
    GtkTreeViewColumn * column;

    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("address",
                                                        renderer,
                                                        "text", COL_ADDRESS,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(refwindow->treeView), column);
    
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes("referencers",
                                                        renderer,
                                                        "text", COL_REFERENCERS,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(refwindow->treeView), column);


    // set up containers
    gtk_container_add(GTK_CONTAINER(refwindow->scrolledWindow),
                      refwindow->treeView);
    gtk_container_add(GTK_CONTAINER(refwindow->window),
                      refwindow->scrolledWindow);

    // signals
    g_signal_connect(refwindow->window,
                     "destroy",
                     G_CALLBACK(refwindow_destroy_event),
                     refwindow);

    gtk_window_set_default_size(GTK_WINDOW(refwindow->window), 280, 480);

    gtk_widget_show(refwindow->treeView);
    gtk_widget_show(refwindow->scrolledWindow);

    refwindow->callback_identifier = rdis_add_callback(refwindow->gui->rdis,
                                        RDIS_CALLBACK(refwindow_rdis_callback),
                                                       refwindow);
    
    return refwindow;
}



void refwindow_delete (struct _refwindow * refwindow)
{
    //gtk_widget_destroy(refwindow->window);
    rdis_remove_callback(refwindow->gui->rdis, refwindow->callback_identifier);
    gui_remove_window(refwindow->gui, refwindow->gui_identifier);
    free(refwindow);
}



GtkWidget * refwindow_window (struct _refwindow * refwindow)
{
    return refwindow->window;
}



void refwindow_destroy_event (GtkWidget * widget,
                               struct _refwindow * refwindow)
{
    refwindow_delete(refwindow);
}


void refwindow_refresh (struct _refwindow * refwindow)
{
    gtk_list_store_clear(refwindow->listStore);

    struct _map * references = rdis_g_references(refwindow->gui->rdis);
    struct _map_it * it;
    for (it = map_iterator(references); it != NULL; it = map_it_next(it)) {
        uint64_t address = map_it_key(it);
        char address_str[32];
        snprintf(address_str, 32, "%04llx", (unsigned long long) address);

        struct _list * references = map_it_data(it);
        char referencers_str[512];
        memset(referencers_str, 0, 512);
        
        struct _list_it * rit;
        for (rit = list_iterator(references); rit != NULL; rit = rit->next) {
            struct _reference * reference = rit->data;

            const char * type_str = "";
            switch (reference->type) {
                case REFERENCE_LOAD                 : type_str = "l:"; break;
                case REFERENCE_STORE                : type_str = "s:"; break;
                case REFERENCE_CONSTANT_ADDRESSABLE : type_str = "c:"; break;
                default: break;
            }

            char referencer[32];
            snprintf(referencer, 32, "%s%04llx ",
                     type_str,
                     (unsigned long long) reference->referencer);
            if (rdstrcat(referencers_str, referencer, 512) >= 512)
                break;
        }

        if (strlen(referencers_str) > 512) {
            printf("ERROR! %d\n", (int) strlen(referencers_str));
        }

        GtkTreeIter treeIter;
        gtk_list_store_append(refwindow->listStore, &treeIter);
        gtk_list_store_set(refwindow->listStore, &treeIter,
                           COL_ADDR_UINT64, address,
                           COL_ADDRESS, address_str,
                           COL_REFERENCERS, referencers_str,
                           -1);
    }

    object_delete(references);
}




void refwindow_rdis_callback (struct _refwindow * refwindow)
{
    refwindow_refresh(refwindow);
}
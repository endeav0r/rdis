#include "rdiswindow.h"

#include <stdlib.h>
#include <string.h>

struct _rdiswindow * rdiswindow_create (struct _gui * gui)
{
    struct _rdiswindow * rdiswindow;

    rdiswindow = (struct _rdiswindow *) malloc(sizeof(struct _rdiswindow));

    rdiswindow->window              = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    rdiswindow->buttonsBox          = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    rdiswindow->vbox                = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    rdiswindow->hexButton           = gtk_button_new_with_label("hex");
    rdiswindow->functionsButton     = gtk_button_new_with_label("functions");
    rdiswindow->consoleTagTable     = gtk_text_tag_table_new();
    rdiswindow->consoleBuffer       =
                              gtk_text_buffer_new(rdiswindow->consoleTagTable);
    rdiswindow->consoleView         =
                      gtk_text_view_new_with_buffer(rdiswindow->consoleBuffer);
    rdiswindow->inputEntry          = gtk_entry_new();
    rdiswindow->menu                = gtk_menu_bar_new();
    rdiswindow->gui                 = gui;


    GtkWidget * menuItemFile = gtk_menu_item_new_with_label("File");
    GtkWidget * menuItemLoad = gtk_menu_item_new_with_label("Load .rdis");
    GtkWidget * menuItemSave = gtk_menu_item_new_with_label("Save .rdis");
    GtkWidget * menuFile         = gtk_menu_new();

    gtk_container_add(GTK_CONTAINER(menuFile), menuItemLoad);
    gtk_container_add(GTK_CONTAINER(menuFile), menuItemSave);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItemFile), menuFile);
    gtk_container_add(GTK_CONTAINER(rdiswindow->menu), menuItemFile);

    gtk_widget_show(menuItemFile);
    gtk_widget_show(menuItemSave);
    gtk_widget_show(menuItemLoad);
    gtk_widget_show(menuFile);


    g_signal_connect(rdiswindow->functionsButton,
                     "clicked",
                     G_CALLBACK(rdiswindow_functions_activate),
                     rdiswindow);

    g_signal_connect(rdiswindow->hexButton,
                     "clicked",
                     G_CALLBACK(rdiswindow_hex_activate),
                     rdiswindow);

    g_signal_connect(rdiswindow->window,
                     "destroy",
                     G_CALLBACK(rdiswindow_destroy_event),
                     rdiswindow);

    gtk_box_pack_start(GTK_BOX(rdiswindow->buttonsBox),
                       rdiswindow->functionsButton,
                       FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(rdiswindow->buttonsBox),
                       rdiswindow->hexButton,
                       FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(rdiswindow->vbox),
                       rdiswindow->menu,
                       FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rdiswindow->vbox),
                       rdiswindow->buttonsBox,
                       FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rdiswindow->vbox),
                       rdiswindow->consoleView,
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rdiswindow->vbox),
                       rdiswindow->inputEntry,
                       FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(rdiswindow->window), rdiswindow->vbox);

    gtk_window_set_default_size(GTK_WINDOW(rdiswindow->window), 600, 400);

    gtk_widget_show(rdiswindow->menu);
    gtk_widget_show(rdiswindow->functionsButton);
    gtk_widget_show(rdiswindow->hexButton);
    gtk_widget_show(rdiswindow->consoleView);
    gtk_widget_show(rdiswindow->inputEntry);
    gtk_widget_show(rdiswindow->buttonsBox);
    gtk_widget_show(rdiswindow->vbox);

    return rdiswindow;
}



void rdiswindow_delete (struct _rdiswindow * rdiswindow)
{
    free(rdiswindow);
}


GtkWidget * rdiswindow_window (struct _rdiswindow * rdiswindow)
{
    return rdiswindow->window;
}


void rdiswindow_console (struct _rdiswindow * rdiswindow, const char * line)
{
    GtkTextIter textIter;

    gtk_text_buffer_get_end_iter(rdiswindow->consoleBuffer, &textIter);
    gtk_text_buffer_insert(rdiswindow->consoleBuffer, &textIter, line, strlen(line));
}


void rdiswindow_functions_activate (GtkButton * button,
                                    struct _rdiswindow * rdiswindow)
{
    printf("rdiswindow_functions_activate\n");
    gui_funcwindow(rdiswindow->gui);
}


void rdiswindow_hex_activate (GtkButton * button,
                              struct _rdiswindow * rdiswindow)
{
    printf("rdiswindow_hex_activate\n");
    gui_hexwindow(rdiswindow->gui);
}


void rdiswindow_destroy_event (GtkWidget * widget,
                               struct _rdiswindow * rdiswindow)
{
    rdiswindow_delete(rdiswindow);
    gtk_main_quit();
}
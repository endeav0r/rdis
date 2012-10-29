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
    rdiswindow->scrolledWindow      = gtk_scrolled_window_new(NULL, NULL);
    rdiswindow->consoleTagTable     = gtk_text_tag_table_new();
    rdiswindow->consoleBuffer       =
                              gtk_text_buffer_new(rdiswindow->consoleTagTable);
    rdiswindow->consoleView         =
                      gtk_text_view_new_with_buffer(rdiswindow->consoleBuffer);
    rdiswindow->inputEntry          = gtk_entry_new();
    rdiswindow->menu                = gtk_menu_bar_new();
    rdiswindow->gui                 = gui;


    gtk_text_buffer_create_tag(rdiswindow->consoleBuffer, "console",
                               "font", "monospace",
                               "size-points", 9.0,
                               NULL);


    GtkWidget * menuItemFile = gtk_menu_item_new_with_label("File");
    GtkWidget * menuItemOpen = gtk_menu_item_new_with_label("Open .rdis");
    GtkWidget * menuItemSave = gtk_menu_item_new_with_label("Save .rdis");
    GtkWidget * menuItemLoad = gtk_menu_item_new_with_label("Load Executable");
    GtkWidget * menuFile     = gtk_menu_new();

    gtk_container_add(GTK_CONTAINER(menuFile), menuItemOpen);
    gtk_container_add(GTK_CONTAINER(menuFile), menuItemSave);
    gtk_container_add(GTK_CONTAINER(menuFile), menuItemLoad);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItemFile), menuFile);
    gtk_container_add(GTK_CONTAINER(rdiswindow->menu), menuItemFile);

    gtk_widget_show(menuItemFile);
    gtk_widget_show(menuItemOpen);
    gtk_widget_show(menuItemSave);
    gtk_widget_show(menuItemLoad);
    gtk_widget_show(menuFile);

    g_signal_connect(menuItemSave,
                     "activate",
                     G_CALLBACK(rdiswindow_save),
                     rdiswindow);

    g_signal_connect(menuItemOpen,
                     "activate",
                     G_CALLBACK(rdiswindow_open),
                     rdiswindow);

    g_signal_connect(menuItemLoad,
                     "activate",
                     G_CALLBACK(rdiswindow_load),
                     rdiswindow);

    g_signal_connect(rdiswindow->functionsButton,
                     "clicked",
                     G_CALLBACK(rdiswindow_functions_activate),
                     rdiswindow);

    g_signal_connect(rdiswindow->hexButton,
                     "clicked",
                     G_CALLBACK(rdiswindow_hex_activate),
                     rdiswindow);

    g_signal_connect(rdiswindow->inputEntry,
                     "activate",
                     G_CALLBACK(rdiswindow_input_activate),
                     rdiswindow);

    g_signal_connect(rdiswindow->scrolledWindow,
                     "size-allocate",
                     G_CALLBACK(rdiswindow_size_allocate),
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
                       rdiswindow->scrolledWindow,
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rdiswindow->vbox),
                       rdiswindow->inputEntry,
                       FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(rdiswindow->scrolledWindow),
                      rdiswindow->consoleView);
    gtk_container_add(GTK_CONTAINER(rdiswindow->window), rdiswindow->vbox);

    gtk_window_set_default_size(GTK_WINDOW(rdiswindow->window), 600, 400);

    gtk_widget_show(rdiswindow->menu);
    gtk_widget_show(rdiswindow->functionsButton);
    gtk_widget_show(rdiswindow->hexButton);
    gtk_widget_show(rdiswindow->scrolledWindow);
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
    gtk_text_buffer_insert_with_tags_by_name(rdiswindow->consoleBuffer,
                                             &textIter,
                                             line,
                                             strlen(line),
                                             "console",
                                             NULL);
    gtk_text_buffer_insert_with_tags_by_name(rdiswindow->consoleBuffer,
                                             &textIter,
                                             "\n",
                                             1,
                                             "console",
                                             NULL);
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


void rdiswindow_open (GtkMenuItem * menuItem, struct _rdiswindow * rdiswindow)
{
    GtkWidget * dialog;

    dialog = gtk_file_chooser_dialog_new("Open Rdis File",
                                         GTK_WINDOW(rdiswindow->window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char tmp[256];

        json_error_t json_error;
        json_t * json = json_load_file(filename, 0, &json_error);
        if (json == NULL) {
            snprintf(tmp, 256, "json error: %s", json_error.text);
            gui_console(rdiswindow->gui, tmp);
            json_decref(json);
        }
        else {
            struct _rdis * rdis = rdis_deserialize(json);
            if (rdis == NULL) {
                snprintf(tmp, 256, "rdis deserialize error # %d\n", serialize_error);
                gui_console(rdiswindow->gui, tmp);
            }
            else {
                gui_close_windows(rdiswindow->gui);
                gui_set_rdis(rdiswindow->gui, rdis);
                snprintf(tmp, 256, "opened file %s", filename);
                gui_console(rdiswindow->gui, tmp);
            }
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


void rdiswindow_save (GtkMenuItem * menuItem, struct _rdiswindow * rdiswindow)
{
    GtkWidget * dialog;
    char tmp[256];

    if (rdiswindow->gui->rdis == NULL) {
        gui_console(rdiswindow->gui, "no rdis loaded");
        return;
    }

    dialog = gtk_file_chooser_dialog_new("Save Rdis File",
                                         GTK_WINDOW(rdiswindow->window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        json_t * json = object_serialize(rdiswindow->gui->rdis);
        if (json_dump_file(json, filename, JSON_COMPACT))
            snprintf(tmp, 256, "error (json) saving file %s\n", filename);
        else
            snprintf(tmp, 256, "saved file %s\n", filename);

        gui_console(rdiswindow->gui, tmp);

        json_decref(json);

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


void rdiswindow_load (GtkMenuItem * menuItem, struct _rdiswindow * rdiswindow)
{
    GtkWidget * dialog;

    dialog = gtk_file_chooser_dialog_new("Load Executable File",
                                         GTK_WINDOW(rdiswindow->window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char tmp[256];

        _loader * loader = loader_create(filename);

        if (loader == NULL)
            snprintf(tmp, 256, "failed to load executable %s", filename);
        else {
            struct _rdis * rdis = rdis_create(loader);
            gui_close_windows(rdiswindow->gui);
            gui_set_rdis(rdiswindow->gui, rdis);
            snprintf(tmp, 256, "loaded executable %s", filename);
        }

        gui_console(rdiswindow->gui, tmp);

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


void rdiswindow_input_activate (GtkEntry * entry,
                                struct _rdiswindow * rdiswindow)
{
    if (rdiswindow->gui->rdis == NULL) {
        gui_console(rdiswindow->gui, "must load rdis first");
        return;
    }

    rdis_lua_execute(rdiswindow->gui->rdis->rdis_lua,
                     gtk_entry_get_text(entry));
    gtk_entry_set_text(entry, "");
}


void rdiswindow_size_allocate (GtkWidget * widget,
                               GdkRectangle * allocation,
                               struct _rdiswindow * rdiswindow)
{
    GtkAdjustment * adj;
    adj = gtk_scrolled_window_get_vadjustment(
                              GTK_SCROLLED_WINDOW(rdiswindow->scrolledWindow));
    gtk_adjustment_set_value(adj, 
            gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
}
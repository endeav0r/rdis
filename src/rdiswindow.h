#ifndef rdiswindow_HEADER
#define rdiswindow_HEADER

#include <gtk/gtk.h>
#include <glib.h>

#include "gui.h"

struct _rdiswindow {
    GtkWidget       * window;
    GtkWidget       * buttonsBox;
    GtkWidget       * vbox;
    GtkWidget       * hexButton;
    GtkWidget       * functionsButton;
    GtkTextTagTable * consoleTagTable;
    GtkTextBuffer   * consoleBuffer;
    GtkWidget       * consoleView;
    GtkWidget       * inputEntry;
    GtkWidget       * menu;

    struct _gui * gui;
};


struct _rdiswindow * rdiswindow_create (struct _gui * gui);
void                 rdiswindow_delete (struct _rdiswindow * rdiswindow);

GtkWidget *          rdiswindow_window  (struct _rdiswindow * rdiswindow);
void                 rdiswindow_console (struct _rdiswindow * rdiswindow,
                                         const char * line);

void rdiswindow_functions_activate (GtkButton * button,
                                    struct _rdiswindow * rdiswindow);
void rdiswindow_hex_activate       (GtkButton * button,
                                    struct _rdiswindow * rdiswindow);
void rdiswindow_destroy_event      (GtkWidget * widget,
                                    struct _rdiswindow * rdiswindow);

#endif
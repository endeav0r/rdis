#ifndef hexwindow_HEADER
#define hexwindow_HEADER

#include <gtk/gtk.h>

#include "gui.h"

struct _hexwindow {
    GtkWidget         * window;
    GtkWidget         * scrolledWindow;
    GtkTextTagTable   * textTagTable;
    GtkTextBuffer     * textBuffer;
    GtkWidget         * textView;
    GtkWidget         * menu_popup;

    struct _gui * gui;
    uint64_t      gui_identifier;

    int      resetting_mark;
    uint64_t selected_address;
};


struct _hexwindow * hexwindow_create (struct _gui * gui);
void                hexwindow_delete (struct _hexwindow * hexwindow);
GtkWidget *         hexwindow_window (struct _hexwindow * hexwindow);

void hexwindow_draw_memmap (struct _hexwindow * hexwindow);

void hexwindow_mark_set     (GtkTextBuffer     * textBuffer,
                             GtkTextIter       * iter,
                             GtkTextMark       * mark,
                             struct _hexwindow * hexwindow);

void hexwindow_populate_popup (GtkTextView * text_view,
                               GtkMenu     * menu,
                               struct _hexwindow * hexwindow);

void hexwindow_user_function (GtkMenuItem * menuItem,
                              struct _hexwindow * hexwindow);

void hexwindow_destroy_event (GtkWidget         * widget,
                              struct _hexwindow * hexwindow);
#endif
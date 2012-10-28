#ifndef gui_HEADER
#define gui_HEADER

#include <gtk/gtk.h>

#include "rdis.h"

struct _gui_window {
    const struct _object * object;
    GtkWidget * window;
};

struct _gui {
    struct _rdis       * rdis;
    struct _rdiswindow * rdiswindow;
    struct _map        * windows;
    uint64_t next_window_index;
};


struct _gui * gui_create ();
void          gui_delete (struct _gui * gui);

void gui_set_rdis (struct _gui * gui, struct _rdis * rdis);

uint64_t gui_add_window    (struct _gui * gui, GtkWidget * window);
void     gui_remove_window (struct _gui * gui, uint64_t identifier);
void     gui_close_windows (struct _gui * gui);

void gui_rdgwindow  (struct _gui * gui, uint64_t top_index);
void gui_funcwindow (struct _gui * gui);
void gui_hexwindow  (struct _gui * gui);

void gui_console (struct _gui * gui, const char * line);

struct _gui_window * gui_window_create (GtkWidget * window);
void                 gui_window_delete (struct _gui_window * gui_window);
struct _gui_window * gui_window_copy   (struct _gui_window * gui_window);

#endif
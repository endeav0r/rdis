#include <gtk/gtk.h>

#include "gui.h"


int main (int argc, char * argv[])
{
    gtk_init(&argc, &argv);

    struct _gui * gui = gui_create();

    gtk_main();

    gui_delete(gui);

    return 0;
}
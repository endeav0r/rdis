#include "hexwindow.h"

#include <string.h>

GtkWidget * hexwindow (struct _byte_table * byte_table)
{
    GtkWidget *     window;
    GtkWidget *     scrolledWindow;
    GtkWidget *     textView;
    GtkTextBuffer * textBuffer;
    GtkTextIter     textIter;
    size_t i, j;
    int c;

    char asciiHexLine[128];
    char asciiHex[8];
    char ascii[32];

    // create textView and textBuffer
    textView   = gtk_text_view_new();
    textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));

    gtk_text_buffer_create_tag(textBuffer,
                               "monospace_font",
                               "font",
                               "monospace",
                               NULL);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), 0);

    // fill textBuffer
    i = 0;
    while (i < byte_table->size) {
        snprintf(asciiHexLine, 128, "%llx  ",
                 (unsigned long long) byte_table->table[i].address);

        for (j = 0; j < 16; j++) {
            if (j > byte_table->size)
                break;

            c = byte_table->table[i + j].byte;
            snprintf(asciiHex, 4, "%02x ", c);
            strncat(asciiHexLine, asciiHex, 127);
            asciiHexLine[127] = '\0';

            if ((c >= 32) && (c < 127))
                ascii[j] = c;
            else
                ascii[j] = '.';
        }
        i += j;
        ascii[j] = '\0';

        strncat(asciiHexLine, ascii, 17);
        strncat(asciiHexLine, "\n", 128);
        gtk_text_buffer_get_end_iter(textBuffer, &textIter);
        gtk_text_buffer_insert_with_tags_by_name(textBuffer,
                                                 &textIter,
                                                 asciiHexLine,
                                                 strlen(asciiHexLine),
                                                 "monospace_font",
                                                 NULL
                                                );
    }

    // textView goes inside scrolledWindow
    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), textView);

    // scrolledWindow goes inside hex_window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_add(GTK_CONTAINER(window), scrolledWindow);

    gtk_widget_show(scrolledWindow);
    gtk_widget_show(textView);

    return window;
}
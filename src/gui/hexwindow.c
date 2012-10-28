#include "hexwindow.h"

#include <string.h>

#include "map.h"
#include "buffer.h"


struct _hexwindow * hexwindow_create (struct _gui * gui)
{
    struct _hexwindow * hexwindow = malloc(sizeof(struct _hexwindow));
    
    hexwindow->window         = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    hexwindow->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    hexwindow->textTagTable   = gtk_text_tag_table_new();
    hexwindow->textBuffer     = gtk_text_buffer_new(hexwindow->textTagTable);
    hexwindow->textView       =
                          gtk_text_view_new_with_buffer(hexwindow->textBuffer);
    hexwindow->menu_popup     = gtk_menu_new();
    hexwindow->gui            = gui;
    hexwindow->gui_identifier = gui_add_window(gui, hexwindow->window);

    hexwindow->selected_address = -1;
    hexwindow->resetting_mark   = 0;


    gtk_text_buffer_create_tag(hexwindow->textBuffer, "hex",
                               "font", "monospace",
                               "size-points", 10.0,
                               NULL);

    g_signal_connect(hexwindow->window,
                     "destroy",
                     G_CALLBACK(hexwindow_destroy_event),
                     hexwindow);

    g_signal_connect(hexwindow->textView,
                     "populate-popup",
                     G_CALLBACK(hexwindow_populate_popup),
                     hexwindow);

    g_signal_connect(hexwindow->textBuffer,
                     "mark-set",
                     G_CALLBACK(hexwindow_mark_set),
                     hexwindow);


    gtk_container_add(GTK_CONTAINER(hexwindow->scrolledWindow),
                      hexwindow->textView);
    gtk_container_add(GTK_CONTAINER(hexwindow->window),
                      hexwindow->scrolledWindow);

    gtk_widget_show(hexwindow->scrolledWindow);
    gtk_widget_show(hexwindow->textView);

    g_object_set(hexwindow->textView, "editable", FALSE, NULL);

    gtk_window_set_default_size(GTK_WINDOW(hexwindow->window), 640, 480);

    hexwindow_draw_memmap(hexwindow);

    return hexwindow;
}



void hexwindow_delete (struct _hexwindow * hexwindow)
{
    gui_remove_window(hexwindow->gui, hexwindow->gui_identifier);
    free(hexwindow);
}



GtkWidget * hexwindow_window (struct _hexwindow * hexwindow)
{
    return hexwindow->window;
}



void hexwindow_draw_memmap (struct _hexwindow * hexwindow)
{
    GtkTextIter iterStart, iterEnd;
    GtkTextIter iter;

    gtk_text_buffer_get_start_iter(hexwindow->textBuffer, &iterStart);
    gtk_text_buffer_get_end_iter  (hexwindow->textBuffer, &iterEnd);
    gtk_text_buffer_delete(hexwindow->textBuffer, &iterStart, &iterEnd);

    gtk_text_buffer_get_start_iter(hexwindow->textBuffer, &iter);

    struct _map_it * it;
    for (it = map_iterator(hexwindow->gui->rdis->memory_map);
         it != NULL;
         it = map_it_next(it)) {
        struct _buffer * buffer = map_it_data(it);
        uint64_t base           = map_it_key(it);
        uint64_t offset         = 0;

        char line[256];
        char ascii[32];
        char tmp[32];
        while (offset < buffer->size) {
            if (offset % 16 == 0) {
                line[0] = '\0';
                snprintf(tmp, 32, "%04llx  ", 
                         (unsigned long long) base + offset);
                strncat(line, tmp, 255);
            }

            int i;
            for (i = 0; i < 16; i++) {
                if (offset < buffer->size) {
                    unsigned int c = buffer->bytes[offset++];
                    snprintf(tmp, 32, "%02x ", c);
                    strncat(line, tmp, 255);
                    if ((c >= 0x20) && (c < 0x7f))
                        ascii[i] = c;
                    else
                        ascii[i] = '.';
                }
                else {
                    strncat(line, "   ", 256);
                    ascii[i] = ' ';
                }
            }
            ascii[i] = '\0';

            strncat(line, " ", 255);
            strncat(line, ascii, 255);
            strncat(line, "\n", 255);
            line[255] = '\0';

            gtk_text_buffer_insert(hexwindow->textBuffer,
                                   &iter,
                                   line,
                                   strlen(line));
        }
    }

    gtk_text_buffer_get_start_iter(hexwindow->textBuffer, &iterStart);
    gtk_text_buffer_get_end_iter  (hexwindow->textBuffer, &iterEnd);
    gtk_text_buffer_apply_tag_by_name(hexwindow->textBuffer, "hex", &iterStart, &iterEnd);
}


void hexwindow_mark_set     (GtkTextBuffer     * textBuffer,
                             GtkTextIter       * iter,
                             GtkTextMark       * mark,
                             struct _hexwindow * hexwindow)
{

    if (hexwindow->resetting_mark)
        return;

    hexwindow->resetting_mark   = 1;
    hexwindow->selected_address = -1;

    // unselect any selected text
    gtk_text_buffer_select_range(textBuffer, iter, iter);

    // iterator to the first character on this line
    int lineno = gtk_text_iter_get_line(iter);
    GtkTextIter firstCharIter;
    GtkTextIter lastCharIter;
    gtk_text_buffer_get_iter_at_line(hexwindow->textBuffer,
                                     &firstCharIter,
                                     lineno);
    gtk_text_buffer_get_iter_at_line(hexwindow->textBuffer,
                                     &lastCharIter,
                                     lineno);

    // the first space past this character will mark the end of our address
    // text
    while (gtk_text_iter_get_char(&lastCharIter) != 0x20) {
        gtk_text_iter_forward_char(&lastCharIter);
    }

    // extract base address text
    char * address_text = gtk_text_iter_get_text(&firstCharIter, &lastCharIter);
    uint64_t address_base = strtoull(address_text, NULL, 16);

    // calculate offset based on position relative to address
    // lastCharIter points to first ' ' after address
    int bytes_offset  = gtk_text_iter_get_line_offset(&lastCharIter) + 2;
    int select_offset = gtk_text_iter_get_line_offset(iter) - bytes_offset;

    if (select_offset < 0) {
        hexwindow->resetting_mark = 0;
        return;
    }

    uint64_t address = 0;
    if (select_offset / 3 > 16) {
        hexwindow->resetting_mark = 0;
        return;
    }
    else
        address = address_base + (select_offset / 3);

    printf("address: %llx\n", (unsigned long long) address);

    // let's select this address
    int offset = address - address_base;
    offset *= 3;
    offset += bytes_offset;

    gtk_text_buffer_get_iter_at_line_offset(hexwindow->textBuffer,
                                            &firstCharIter,
                                            lineno,
                                            offset);
    gtk_text_buffer_get_iter_at_line_offset(hexwindow->textBuffer,
                                            &lastCharIter,
                                            lineno,
                                            offset + 2);
    gtk_text_buffer_select_range(textBuffer, &firstCharIter, &lastCharIter);

    g_free(address_text);

    hexwindow->selected_address = address;
    hexwindow->resetting_mark = 0;
}


void hexwindow_populate_popup (GtkTextView * text_view,
                               GtkMenu     * menu,
                               struct _hexwindow * hexwindow)
{
    printf("populate popup\n");
    // popup menu stuff
    GtkWidget * menuItem = gtk_menu_item_new_with_label("User Function");
    g_signal_connect(menuItem,
                     "activate",
                     G_CALLBACK(hexwindow_user_function),
                     hexwindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);

    gtk_widget_show(menuItem);
}


void hexwindow_user_function (GtkMenuItem * menuItem,
                              struct _hexwindow * hexwindow)
{
    if (hexwindow->selected_address != -1)
        rdis_user_function(hexwindow->gui->rdis, hexwindow->selected_address);
}


void hexwindow_destroy_event (GtkWidget * widget,
                              struct _hexwindow * hexwindow)
{
    hexwindow_delete(hexwindow);
}
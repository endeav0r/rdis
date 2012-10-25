#include "rdg_node.h"

#include <stdlib.h>
#include <stdio.h>

#include "instruction.h"
#include "label.h"
#include "list.h"


static const struct _object rdg_node_object = {
    (void   (*) (void *))         rdg_node_delete, 
    (void * (*) (void *))         rdg_node_copy,
    (int    (*) (void *, void *)) rdg_node_cmp,
    NULL
};


struct _rdg_node * rdg_node_create (uint64_t index, cairo_surface_t * surface)
{
    struct _rdg_node * node;

    node = (struct _rdg_node *) malloc(sizeof(struct _rdg_node));
    node->object  = &rdg_node_object;
    node->index   = index;
    if (surface == NULL)
        node->surface = NULL;
    else
        node->surface = cairo_surface_copy(surface);
    node->x        = 0;
    node->y        = 0;
    node->position = 0.0;
    node->level    = 0;
    node->flags    = 0;

    return node;
}


void rdg_node_delete (struct _rdg_node * node)
{
    if (node->surface != NULL)
        cairo_surface_destroy(node->surface);
    free(node);
}


struct _rdg_node * rdg_node_copy (struct _rdg_node * node)
{
    struct _rdg_node * new_node = rdg_node_create(node->index, node->surface);
    new_node->x        = node->x;
    new_node->y        = node->y;
    new_node->position = node->position;
    new_node->level    = node->level;
    new_node->flags    = node->flags;
    return new_node;
}


int rdg_node_cmp (struct _rdg_node * lhs, struct _rdg_node * rhs)
{
    if (lhs->index < rhs->index)
        return -1;
    if (lhs->index > rhs->index);
        return 1;
    return 0;
}


int rdg_node_width (struct _rdg_node * rdg_node)
{
    if (rdg_node->surface == NULL)
        return 0;
    return cairo_image_surface_get_width(rdg_node->surface);
}


int rdg_node_height (struct _rdg_node * rdg_node)
{
    if (rdg_node->surface == NULL)
        return 0;
    return cairo_image_surface_get_height(rdg_node->surface);
}


int rdg_node_center_x (struct _rdg_node * rdg_node)
{
    int width = rdg_node_width(rdg_node);
    return rdg_node->x + (width / 2);
}


int rdg_node_center_y (struct _rdg_node * rdg_node)
{
    int height = rdg_node_height(rdg_node);
    return rdg_node->y + (height / 2);
}


cairo_surface_t * rdg_node_draw_full (struct _graph_node * node,
                                      struct _map        * labels,
                                      double               bg_red,
                                      double               bg_green,
                                      double               bg_blue,
                                      uint64_t             highlight_ins) {
    cairo_surface_t    * surface;
    cairo_t            * ctx;
    cairo_text_extents_t te;
    cairo_font_extents_t fe;

    // create a rough approximation of the graph height
    int number_of_instructions = ((struct _list *) node->data)->size;
    int height = ((int) RDG_NODE_FONT_SIZE + 6) * number_of_instructions;
    height += (int) RDG_NODE_PADDING * 2;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 800, height);
    ctx     = cairo_create(surface);

    // draw contents
    cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);

    cairo_select_font_face(ctx, 
                           RDG_NODE_FONT_FACE,
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);

    cairo_set_font_size(ctx, RDG_NODE_FONT_SIZE);

    cairo_font_extents(ctx, &fe);
    double top = RDG_NODE_PADDING + fe.height;
    double max_width = 0.0;

    struct _list_it * list_it;
    for (list_it = list_iterator(node->data);
         list_it != NULL;
         list_it = list_it->next) {

        struct _ins * ins = list_it->data;

        if (ins->address == highlight_ins)
            cairo_select_font_face(ctx,
                                   RDG_NODE_FONT_FACE,
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
        else
            cairo_select_font_face(ctx,
                                   RDG_NODE_FONT_FACE,
                                   CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);

        int line_x = RDG_NODE_PADDING;
        char tmp[128];

        // print the address
        snprintf(tmp, 128, "%llx", (unsigned long long) ins->address);
        cairo_set_source_rgb(ctx, RDG_NODE_ADDR_COLOR);
        cairo_move_to(ctx, line_x, top);
        cairo_show_text(ctx, tmp);
        cairo_text_extents(ctx, tmp, &te);
        line_x += te.width + RDG_NODE_FONT_SIZE;

        // print the bytes
        // get the length of two characters for padding
        /*
        cairo_set_source_rgb(ctx, RDG_NODE_BYTE_COLOR);
        cairo_text_extents(ctx, "00", &te);
        int byte_padding = te.width;
        int i;
        for (i = 0; i < 8; i++) {
            if (i >= ins->size) {
                snprintf(tmp, 128, "..");
                cairo_move_to(ctx, line_x, top);
                cairo_show_text(ctx, tmp);
            }
            else {
                snprintf(tmp, 128, "%02x", (int) ins->bytes[i]);
                cairo_move_to(ctx, line_x, top);
                cairo_show_text(ctx, tmp);
            }
            line_x += byte_padding;
        }
        line_x += byte_padding;
        */

        snprintf(tmp, 128, "%s", ins->description);
        cairo_move_to(ctx, line_x, top);
        cairo_set_source_rgb(ctx, RDG_NODE_DESC_COLOR);
        cairo_show_text(ctx, tmp);
        cairo_text_extents(ctx, tmp, &te);
        line_x += te.width;

        if (ins->target != -1) {
            struct _label * label = map_fetch(labels, ins->target);
            if (label != NULL) {
                snprintf(tmp, 128, " (%s) ", label->text);
                cairo_move_to(ctx, line_x, top);
                cairo_set_source_rgb(ctx, RDG_NODE_LABEL_COLOR);
                cairo_show_text(ctx, tmp);
                cairo_text_extents(ctx, tmp, &te);
                line_x += te.width;
            }
        }

        if (ins->comment != NULL) {
            snprintf(tmp, 128, " ; %s", ins->comment);
            cairo_move_to(ctx, line_x, top);
            cairo_set_source_rgb(ctx, RDG_NODE_COMMENT_COLOR);
            cairo_show_text(ctx, tmp);
            cairo_text_extents(ctx, tmp, &te);
            line_x += te.width;
        }


        top += fe.height + 2.0;
        if (line_x > max_width)
            max_width = line_x;
    }

    top -= fe.height + 2.0;

    // draw edges
    cairo_set_line_width(ctx, 1);
    cairo_move_to(ctx, 0.5, 0.5);
    cairo_line_to(ctx, (RDG_NODE_PADDING * 2) + max_width + 0.5, 0.5);
    cairo_line_to(ctx,
                  (RDG_NODE_PADDING * 2) + max_width + 0.5,
                  top + (RDG_NODE_PADDING * 2) + 0.5);
    cairo_line_to(ctx, 0.5, top + (RDG_NODE_PADDING * 2) + 0.5);
    cairo_line_to(ctx, 0.5, 0.5);
    cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);
    cairo_stroke(ctx);
    cairo_destroy(ctx);
    
    // copy this surface to our final, cropped surface
    cairo_surface_t * dest;
    dest = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                      (RDG_NODE_PADDING * 2) + max_width + 2,
                                      (RDG_NODE_PADDING * 2) + top + 2);
    ctx = cairo_create(dest);

    // draw background
    cairo_rectangle(ctx,
                    0,
                    0,
                    (RDG_NODE_PADDING * 2) + max_width + 1,
                    (RDG_NODE_PADDING * 2) + top + 1);
    cairo_set_source_rgb(ctx, bg_red, bg_green, bg_blue);
    cairo_fill(ctx);

    cairo_set_source_surface(ctx, surface, 0, 0);
    cairo_paint(ctx);

    // cairo has a memory leak with cairo_select_font_face (1.12.2-3) in
    // cairo_select_font_face. it will be triggered here by not freeing
    // the font resource
    cairo_destroy(ctx);

    cairo_surface_destroy(surface);

    return dest;
}


cairo_surface_t * rdg_node_draw (struct _graph_node * node, struct _map * labels)
{
    return rdg_node_draw_full(node, labels, RDG_NODE_BG_COLOR, -1);
}

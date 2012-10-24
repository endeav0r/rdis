#ifndef rdg_node_HEADER
#define rdg_node_HEADER

#include <cairo.h>

struct _rdg_node;

#include "rdg.h"

struct _rdg_node {
    const struct _object * object;
    uint64_t               index;
    cairo_surface_t      * surface;
    int    level;
    double position;
    int    x;
    int    y;
    int    flags;
};

struct _rdg_node * rdg_node_create (uint64_t index, cairo_surface_t * surface);
void               rdg_node_delete (struct _rdg_node * node);
struct _rdg_node * rdg_node_copy   (struct _rdg_node * node);
int                rdg_node_cmp    (struct _rdg_node * lhs,
                                    struct _rdg_node * rhs);

int rdg_node_width    (struct _rdg_node * rdg_node);
int rdg_node_height   (struct _rdg_node * rdg_node);
int rdg_node_center_x (struct _rdg_node * rdg_node);
int rdg_node_center_y (struct _rdg_node * rdg_node);



cairo_surface_t * rdg_node_draw_full (struct _graph_node * node,
                                      struct _map * labels,
                                      double bg_red,
                                      double bg_green,
                                      double bg_blue,
                                      uint64_t highlight_ins);
cairo_surface_t * rdg_node_draw (struct _graph_node * node, struct _map * labels);

#endif
#ifndef rdgraph_HEADER
#define rdgraph_HEADER

#include <cairo.h>
#include <inttypes.h>

#include "graph.h"
#include "index.h"
#include "list.h"
#include "map.h"
#include "object.h"
#include "rdg_node.h"

enum {
    RDG_ARROW_TYPE_NONE,
    RDG_ARROW_TYPE_OPEN,
    RDG_ARROW_TYPE_FILL
};

#define RDG_NODE_FONT_SIZE 12.0
#define RDG_NODE_FONT_FACE "monospace"
#define RDG_NODE_PADDING   8.0

#define RDG_NODE_BG_COLOR        1,   1,   1
#define RDG_NODE_ADDR_COLOR      0,   0.2, 0.5
#define RDG_NODE_BYTE_COLOR      0.5, 0.2, 0
#define RDG_NODE_DESC_COLOR      0,   0,   0
#define RDG_NODE_LABEL_COLOR     0,   0,   1
#define RDG_NODE_COMMENT_COLOR   0,   0.5, 0
#define RDG_NODE_REFERENCE_COLOR 0.5, 0.0, 0.2

#define RDG_EDGE_NORMAL_COLOR    0,   0,   0
#define RDG_EDGE_JUMP_COLOR      0,   0,   0.5
#define RDG_EDGE_JCC_TRUE_COLOR  0,   0.5, 0
#define RDG_EDGE_JCC_FALSE_COLOR 0.5, 0,   0

#define RDG_ARROW_LENGTH  12.0
#define RDG_ARROW_DEGREES 0.5

#define RDG_NODE_VIRTUAL      (1 << 0)
#define RDG_NODE_ACYCLIC      (1 << 1)
#define RDG_NODE_LEVEL_SET    (1 << 2)
#define RDG_NODE_POSITIONED   (1 << 4)

#define RDG_NODE_X_SPACING  16
//#define RDG_NODE_X_MAX_SPACING 200
#define RDG_NODE_Y_SPACING  48
#define RDG_SURFACE_PADDING 16

enum {
    RDG_NO_INTERSECT,
    RDG_TOP_LEFT,
    RDG_TOP_RIGHT,
    RDG_BOTTOM_LEFT,
    RDG_BOTTOM_RIGHT
};

/*
* assign levels
* create virtual nodes
* assign level map
* assign position (creates level map)
* assign initial x posiiton values
* reduce edge crossings
* assign final x posiiton values and set graph width
* assign y position values
* reduce edge lengths
*/

/*
* rdg->levels
* this is a map of maps of struct _index, where the map at position N contains
* the index of all nodes at level N.
* rdg->levels is NULL until instantiated and filled
*/
struct _rdg {
    const struct _object * object;
    cairo_surface_t      * surface;
    uint64_t               top_index;
    struct _graph        * graph;
    struct _map          * levels;
    int width;
    int height;
};

struct _rdg_node_color {
    const struct _object * object;
    uint64_t index;
    double red;
    double green;
    double blue;
};

void rdg_debug (struct _rdg * rdg);

/*
* rdg functions
*/
// top_index = is the index of the top node in this graph
// graph     = an instruction graph from a loader
// labels    = a label map which has labels for instruction targets
struct _rdg * rdg_create (uint64_t        top_index,
                          struct _graph * graph,
                          struct _map   * labels);
void          rdg_delete (struct _rdg * rdg);
struct _rdg * rdg_copy   (struct _rdg * rdg);

void          rdg_reduce_and_draw (struct _rdg * rdg);

int           rdg_width  (struct _rdg * rdg);
int           rdg_height (struct _rdg * rdg);

// these functions return the x/y coords of a node in the
// rdg_graph, after they have been laid out
int rdg_node_x (struct _rdg * rdg, uint64_t index);
int rdg_node_y (struct _rdg * rdg, uint64_t index);

// rdg       = this graph we want to color
// ins_graph = a graph, as returned by the loader, which contains the nodes
//             we want to color
// labels    = a label map which includes labels for instruction targets
// node_color_lost = a list of rdg_node_color objects
void                rdg_color_nodes  (struct _rdg * rdg,
                                      struct _graph     * ins_graph,
                                      struct _map       * labels,
                                      struct _list * node_color_list);

// rdg       = this graph we want to color
// ins_graph = a graph, as returned by the loader, which contains the nodes
//             we want to color
// labels    = a label map which includes labels for instruction targets
// node_color_lost = a list of rdg_node_color objects
// highlight_ins   = an instruction index to highlight
void                rdg_custom_nodes  (struct _rdg * rdg,
                                       struct _graph     * ins_graph,
                                       struct _map       * labels,
                                       struct _list * node_color_list,
                                       uint64_t highlight_ins);
// redraws the entire graph. call this after rdg_color_nodes
void                rdg_draw      (struct _rdg * rdg);


/*
* rdg_node_color functions
*/
struct _rdg_node_color * rdg_node_color_create (uint64_t index,
                                                double red,
                                                double green,
                                                double blue);
void                     rdg_node_color_delete (struct _rdg_node_color *);
struct _rdg_node_color * rdg_node_color_copy   (struct _rdg_node_color *);
int                      rdg_node_color_cmp    (struct _rdg_node_color *,
                                                struct _rdg_node_color *);
/*
* rdg_node functions
*/
uint64_t rdg_get_node_by_coords (struct _rdg * rdg, int x, int y);

// rdg       = the rdg
// ins_graph = an instruction graph returned by the loader
uint64_t rdg_get_ins_by_coords (struct _rdg   * rdg,
                                struct _graph * ins_graph,
                                struct _map   * labels,
                                int x, int y);

int rdg_node_source_x (struct _rdg * rdg,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node);
int rdg_node_source_y (struct _rdg * rdg,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node);
int rdg_node_sink_x   (struct _rdg * rdg,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node);
int rdg_node_sink_y   (struct _rdg * rdg,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node);

// utility functions
void rdg_acyclicize     (struct _graph * graph, uint64_t top_index);
void rdg_acyclicize_pre (struct _graph * graph, uint64_t index);


void rdg_node_level_zero         (struct _graph_node * node);
void rdg_assign_levels           (struct _graph * graph, uint64_t top_index);

void rdg_assign_initial_position (struct _rdg * rdg);

void rdg_create_virtual_nodes  (struct _rdg * rdg);
void rdg_remove_virtual_nodes (struct _rdg * rdg);

void rdg_assign_level_map     (struct _rdg * rdg);
void rdg_assign_x             (struct _rdg * rdg);
void rdg_assign_y             (struct _rdg * rdg);
void rdg_set_graph_width      (struct _rdg * rdg);

int  rdg_level_top            (struct _rdg * rdg, int level);

inline void rdg_swap_node_positions (struct _rdg * rdg,
                                     int level,
                                     int left,
                                     int right);

void rdg_reduce_edge_crossings (struct _rdg * rdg);

cairo_surface_t * rdg_draw_node_full (struct _graph_node * node,
                                      struct _map * labels,
                                      double bg_red,
                                      double bg_green,
                                      double bg_blue,
                                      uint64_t highlight_ins);
cairo_surface_t * rdg_draw_node (struct _graph_node * node, struct _map * labels);

cairo_surface_t * cairo_surface_copy (cairo_surface_t * src);

void              rdgraph_draw_node_test (const char * filename,
                                          struct _graph_node * node);
#endif
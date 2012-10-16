#ifndef rdgraph_HEADER
#define rdgraph_HEADER

#include <cairo.h>
#include <inttypes.h>

#include "graph.h"
#include "index.h"
#include "map.h"
#include "object.h"
#include "tree.h"

char * rdgraph_graphviz_string (struct _graph * graph);
char * rdgraph_graphviz_string_params (struct _graph * graph,
                                       const char * params);

int rdgraph_graph_to_png_file (struct _graph * graph,
                               const char * filename);
int rdgraph_graph_to_png_file_params (struct _graph * graph,
                                      const char * filename,
                                      const char * params);

enum {
    RDG_ARROW_TYPE_NONE,
    RDG_ARROW_TYPE_OPEN,
    RDG_ARROW_TYPE_FILL
};

#define RDG_NODE_FONT_SIZE  14.0
#define RDG_NODE_PADDING    8.0

#define RDG_NODE_BG_COLOR   1, 1,   1
#define RDG_NODE_ADDR_COLOR 0,   0.2, 0.5
#define RDG_NODE_BYTE_COLOR 0.5, 0.2, 0
#define RDG_NODE_DESC_COLOR 0,   0,   0

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

#define RDG_NODE_X_SPACING  40
#define RDG_NODE_X_MAX_SPACING 200
#define RDG_NODE_Y_SPACING  80
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
* rdg_graph->levels
* this is a map of maps of struct _index, where the map at position N contains
* the index of all nodes at level N.
* rdg_graph->levels is NULL until instantiated and filled
*/
struct _rdg_graph {
    const struct _object * object;
    cairo_surface_t      * surface;
    uint64_t               top_index;
    struct _graph        * graph;
    struct _map          * levels;
    int width;
    int height;
};

struct _rdg_node {
    const struct _object * object;
    uint64_t               index;
    cairo_surface_t      * surface;
    int level;
    int position;
    int x;
    int y;
    int flags;
};

void rdg_debug (struct _rdg_graph * rdg_graph);

struct _rdg_graph * rdg_graph_create (uint64_t top_index, struct _graph * graph);
void                rdg_graph_delete (struct _rdg_graph * rdg_graph);
struct _rdg_graph * rdg_graph_copy   (struct _rdg_graph * rdg_graph);

void       rdg_graph_reduce_and_draw (struct _rdg_graph * rdg_graph);

int rdg_graph_width  (struct _rdg_graph * rdg_graph);
int rdg_graph_height (struct _rdg_graph * rdg_graph);

struct _rdg_node * rdg_node_create (uint64_t index, cairo_surface_t * surface);
void               rdg_node_delete (struct _rdg_node * node);
struct _rdg_node * rdg_node_copy   (struct _rdg_node * node);
int                rdg_node_cmp    (struct _rdg_node * lhs,
                                    struct _rdg_node * rhs);

int rdg_node_width    (struct _rdg_node * rdg_node);
int rdg_node_height   (struct _rdg_node * rdg_node);
int rdg_node_center_x (struct _rdg_node * rdg_node);
int rdg_node_center_y (struct _rdg_node * rdg_node);

int rdg_node_source_x (struct _rdg_graph * rdg_graph,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node);
int rdg_node_source_y (struct _rdg_graph * rdg_graph,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node);
int rdg_node_sink_x (struct _rdg_graph * rdg_graph,
                     struct _rdg_node * src_node,
                     struct _rdg_node * dst_node);
int rdg_node_sink_y (struct _rdg_graph * rdg_graph,
                     struct _rdg_node * src_node,
                     struct _rdg_node * dst_node);


void rdg_node_level_zero      (struct _graph_node * node);
void rdg_node_assign_level    (struct _graph * graph, struct _graph_node * node);
void rdg_assign_levels        (struct _graph * graph, uint64_t top_index);
void rdg_create_virtual_nodes (struct _rdg_graph * rdg_graph);
void rdg_assign_level_map     (struct _rdg_graph * rdg_graph);
void rdg_assign_position      (struct _rdg_graph * rdg_graph);
void rdg_assign_x     (struct _rdg_graph * rdg_graph);
void rdg_assign_y             (struct _rdg_graph * rdg_graph);
void rdg_acyclicize           (struct _graph * graph, uint64_t top_index);
void rdg_acyclicize_pre       (struct _graph * graph, uint64_t index);
void rdg_set_graph_width      (struct _rdg_graph * rdg_graph);

int  rdg_level_top            (struct _rdg_graph * rdg_graph, int level);


int  rdg_level_count_edge_crossings  (struct _rdg_graph * rdg_graph, int level);
int  rdg_count_edge_crossings  (struct _rdg_graph * rdg_graph);
void rdg_reduce_edge_crossings (struct _rdg_graph * rdg_graph);

void rdg_draw                 (struct _rdg_graph * rdg_graph);

cairo_surface_t * cairo_surface_copy (cairo_surface_t * src);

cairo_surface_t * rdgraph_draw_graph_node (struct _graph_node * node);
void              rdgraph_draw_node_test (const char * filename,
                                          struct _graph_node * node);
#endif
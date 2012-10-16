#include "rdgraph.h"

#include "graph.h"
#include "instruction.h"
#include "list.h"
#include "rdstring.h"
#include "queue.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static const struct _object rdg_node_object = {
    (void   (*) (void *))         rdg_node_delete, 
    (void * (*) (void *))         rdg_node_copy,
    (int    (*) (void *, void *)) rdg_node_cmp,
    NULL
};


static const struct _object rdg_graph_object = {
    (void   (*) (void *))         rdg_graph_delete, 
    (void * (*) (void *))         rdg_graph_copy,
    NULL,
    NULL
};


void rdg_debug (struct _rdg_graph * rdg_graph) {
    struct _graph_it * graph_it;

    for (graph_it = graph_iterator(rdg_graph->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_node = graph_it_data(graph_it);
        printf("index: %llx, level: %d, position: %x, flags: %d, x: %d, y: %d\n",
               (unsigned long long) rdg_node->index,
               rdg_node->level,
               rdg_node->position,
               rdg_node->flags,
               rdg_node->x,
               rdg_node->y);
    }
}


struct _rdg_graph * rdg_graph_create (uint64_t top_index, struct _graph * graph)
{
    struct _rdg_graph * rdg_graph;

    rdg_graph = (struct _rdg_graph *) malloc(sizeof(struct _rdg_graph));
    rdg_graph->object    = &rdg_graph_object;
    rdg_graph->surface   = NULL;
    rdg_graph->top_index = top_index;
    rdg_graph->graph     = graph_create();
    rdg_graph->levels    = NULL;
    rdg_graph->width     = 0;
    rdg_graph->height    = 0;

    // add nodes to rdg_graph->graph and acyclic graph
    struct _graph * acyclic_graph = graph_create();
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _graph_node * node = graph_it_node(graph_it);

        cairo_surface_t * surface;
        surface = rdgraph_draw_graph_node(node);
        struct _rdg_node * rdg_node = rdg_node_create(node->index, surface);
        graph_add_node(rdg_graph->graph, node->index, rdg_node);
        object_delete(rdg_node);

        rdg_node = rdg_node_create(node->index, NULL);
        graph_add_node(acyclic_graph, node->index, rdg_node);
        object_delete(rdg_node);

        cairo_surface_destroy(surface);
    }

    // add edges
    for (graph_it = graph_iterator(graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _graph_node * node = graph_it_node(graph_it);

        struct _list_it * list_it;
        for (list_it = list_iterator(node->edges);
             list_it != NULL;
             list_it = list_it->next) {
            struct _graph_edge * edge = list_it->data;

            printf("rdg_graph adding edge: %llx -> %llx\n",
                   (unsigned long long) edge->head,
                   (unsigned long long) edge->tail);
            graph_add_edge(rdg_graph->graph, edge->head, edge->tail, edge->data);
            graph_add_edge(acyclic_graph, edge->head, edge->tail, edge->data);
        }
    }

    // acyclicize and assign levels
    rdg_acyclicize(acyclic_graph, top_index);
    rdg_acyclicize_pre(acyclic_graph, top_index);
    rdg_assign_levels(acyclic_graph, top_index);

    // copy over levels
    for (graph_it = graph_iterator(acyclic_graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_acyclic_node = graph_it_data(graph_it);
        struct _rdg_node * rdg_node = graph_fetch_data(rdg_graph->graph, rdg_acyclic_node->index);
        rdg_node->level = rdg_acyclic_node->level;
    }

    object_delete(acyclic_graph);
    rdg_create_virtual_nodes(rdg_graph);
    rdg_assign_level_map(rdg_graph);
    rdg_assign_position(rdg_graph);
    rdg_assign_y(rdg_graph);
    rdg_assign_x(rdg_graph);

    rdg_graph_reduce_and_draw (rdg_graph);

    return rdg_graph;
}


void rdg_graph_reduce_and_draw (struct _rdg_graph * rdg_graph)
{

    rdg_reduce_edge_crossings(rdg_graph);
    rdg_assign_x(rdg_graph);
    rdg_set_graph_width(rdg_graph);
    //rdg_debug(rdg_graph);
    rdg_draw(rdg_graph);
}


int rdg_graph_width (struct _rdg_graph * rdg_graph)
{
    return rdg_graph->width + RDG_SURFACE_PADDING * 2;
}


int rdg_graph_height (struct _rdg_graph * rdg_graph)
{
    return rdg_graph->height + RDG_SURFACE_PADDING * 2;
}


void rdg_graph_delete (struct _rdg_graph * rdg_graph)
{
    if (rdg_graph->surface != NULL)
        cairo_surface_destroy(rdg_graph->surface);
    if (rdg_graph->levels != NULL)
        object_delete(rdg_graph->levels);
    object_delete(rdg_graph->graph);
    free(rdg_graph);
}


struct _rdg_graph * rdg_graph_copy (struct _rdg_graph * rdg_graph)
{
    struct _rdg_graph * new_rdg;

    new_rdg = (struct _rdg_graph *) malloc(sizeof(struct _rdg_graph));
    new_rdg->object    = &rdg_graph_object;
    if (rdg_graph->surface == NULL)
        new_rdg->surface = NULL;
    else
        new_rdg->surface   = cairo_surface_copy(rdg_graph->surface);
    new_rdg->top_index = rdg_graph->top_index;
    new_rdg->graph     = object_copy(rdg_graph->graph);
    if (rdg_graph->levels != NULL)
        new_rdg->levels = object_copy(rdg_graph->levels);
    new_rdg->width  = rdg_graph->width;
    new_rdg->height = rdg_graph->height;

    return new_rdg;
}


cairo_surface_t * cairo_surface_copy (cairo_surface_t * src)
{
    cairo_surface_t * dst;
    dst = cairo_image_surface_create(cairo_image_surface_get_format(src),
                                     cairo_image_surface_get_width(src),
                                     cairo_image_surface_get_height(src));
    cairo_t * ctx = cairo_create(dst);
    cairo_set_source_surface(ctx, src, 0, 0);
    cairo_paint(ctx);
    cairo_destroy(ctx);
    return dst;
}


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
    node->position = 0;
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


int rdg_node_source_x (struct _rdg_graph * rdg_graph,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node)
{
    // translate destination's center x in image to position on x axis of src
    if (src_node->level == dst_node->level) {
        if (src_node->position < dst_node->position)
            return src_node->x + rdg_node_width(src_node);
        else
            return src_node->x;
    }

    double position = rdg_node_center_x(dst_node);
    position /= rdg_graph_width(rdg_graph);
    return src_node->x + ((double) rdg_node_width(src_node) * position);
}


int rdg_node_source_y (struct _rdg_graph * rdg_graph,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node)
{
    if (src_node->level == dst_node->level)
        return src_node->y + (rdg_node_height(src_node) / 2);

    if (src_node->y < dst_node->y)
        return src_node->y + rdg_node_height(src_node);
    return src_node->y;
}


int rdg_node_sink_x (struct _rdg_graph * rdg_graph,
                     struct _rdg_node * src_node,
                     struct _rdg_node * dst_node)
{
    // translate destination's center x in image to position on x axis of src
    if (src_node->level == dst_node->level) {
        if (src_node->position > dst_node->position)
            return dst_node->x + rdg_node_width(dst_node);
        else
            return dst_node->x;
    }

    double position = rdg_node_center_x(src_node);
    position /= rdg_graph_width(rdg_graph);
    return dst_node->x + ((double) rdg_node_width(dst_node) * position);
}


int rdg_node_sink_y (struct _rdg_graph * rdg_graph,
                     struct _rdg_node * src_node,
                     struct _rdg_node * dst_node)
{
    if (src_node->level == dst_node->level)
        return src_node->y + (rdg_node_height(src_node) / 2);

    if (src_node->y < dst_node->y)
        return dst_node->y;
    return dst_node->y + rdg_node_height(dst_node);
}



void rdg_acyclicize (struct _graph * graph, uint64_t index)
{
    struct _graph_node * node = graph_fetch_node(graph, index);
    if (node == NULL) {
        printf("acyclicize NULL node error, index: %llx\n",
               (unsigned long long) index);
        exit(-1);
    }
    struct _rdg_node * rdg_node = node->data;
    struct _queue * queue = queue_create();

    rdg_node->flags |= RDG_NODE_ACYCLIC;

    struct _list_it * it;
    struct _list * successors = graph_node_successors(node);
    for (it = list_iterator(successors); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;

        struct _rdg_node * rdg_suc_node;
        rdg_suc_node = graph_fetch_data(graph, edge->tail);
        if (rdg_suc_node->flags & RDG_NODE_ACYCLIC)
            queue_push(queue, edge);
        else
            rdg_acyclicize(graph, edge->tail);
    }
    object_delete(successors);

    while (queue->size > 0) {
        struct _graph_edge * edge = queue_peek(queue);
        printf("acyclicize removing edge %llx -> %llx\n",
               (unsigned long long) edge->head,
               (unsigned long long) edge->tail);
        graph_remove_edge(graph, edge->head, edge->tail);
        queue_pop(queue);
    }

    object_delete(queue);
}



void rdg_acyclicize_pre (struct _graph * graph, uint64_t index)
{
    struct _graph_node * node = graph_fetch_node(graph, index);
    if (node == NULL) {
        printf("acyclicize NULL node error, index: %llx\n",
               (unsigned long long) index);
        exit(-1);
    }
    struct _rdg_node * rdg_node = node->data;
    struct _queue * queue = queue_create();

    // mark this node with acyclic flag
    rdg_node->flags |= RDG_NODE_ACYCLIC;

    struct _list_it * it;
    struct _list * predecessors = graph_node_predecessors(node);
    for (it = list_iterator(predecessors); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;

        struct _rdg_node * rdg_suc_node;
        // get predecessor node
        rdg_suc_node = graph_fetch_data(graph, edge->head);
        // if this node is acyclic
        if (rdg_suc_node->flags & RDG_NODE_ACYCLIC)
            // mark this edge for deletion
            queue_push(queue, edge);
        else
            // otherwise, perform recursive call on this node
            rdg_acyclicize_pre(graph, edge->head);
    }
    object_delete(predecessors);

    while (queue->size > 0) {
        struct _graph_edge * edge = queue_peek(queue);
        printf("acyclicize pre removing edge %llx -> %llx\n",
               (unsigned long long) edge->head,
               (unsigned long long) edge->tail);
        graph_remove_edge(graph, edge->head, edge->tail);
        queue_pop(queue);
    }

    object_delete(queue);
}


cairo_surface_t * rdgraph_draw_graph_node (struct _graph_node * node) {
    cairo_surface_t    * surface;
    cairo_t            * ctx;
    cairo_text_extents_t te;
    cairo_font_extents_t fe;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 800, 800);
    ctx     = cairo_create(surface);

    // draw contents
    cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);

    cairo_select_font_face(ctx, 
                           "monospace",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);

    cairo_set_font_size(ctx, 12.0);

    cairo_font_extents(ctx, &fe);
    double top = RDG_NODE_PADDING + fe.height;
    double max_width = 0.0;

    struct _list_it * list_it;
    for (list_it = list_iterator(node->data);
         list_it != NULL;
         list_it = list_it->next) {

        struct _ins * ins = list_it->data;

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

        snprintf(tmp, 128, "%s", ins->description);
        cairo_move_to(ctx, line_x, top);
        cairo_set_source_rgb(ctx, RDG_NODE_DESC_COLOR);
        cairo_show_text(ctx, tmp);
        cairo_text_extents(ctx, tmp, &te);
        line_x += te.width;


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
    cairo_set_source_rgb(ctx, RDG_NODE_BG_COLOR);
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


void rdg_node_level_zero (struct _graph_node * node)
{
    struct _rdg_node * rdg_node = node->data;
    rdg_node->flags &= ~RDG_NODE_LEVEL_SET;
    rdg_node->level = 0;
}


void rdg_node_assign_level (struct _graph * graph, struct _graph_node * node)
{
    struct _rdg_node * rdg_node = node->data;
    struct _graph_edge * edge;
    struct _list_it * it;
    int new_level = -9000;

    if (rdg_node->flags & RDG_NODE_LEVEL_SET)
        return;

    printf("rdg_node_assign_level node %p %p %llx\n",
           node, node->graph,
           (unsigned long long) rdg_node->index);

    // get highest level of predecessors
    struct _list * predecessors = graph_node_predecessors(node);
    for (it = list_iterator(predecessors); it != NULL; it = it->next) {
        edge = it->data;
        struct _rdg_node * pre_node = graph_fetch_data(graph, edge->head);

        // loops don't count
        if (edge->head == edge->tail)
            continue;

        printf("%llx -> %llx\n",
               (unsigned long long) edge->head,
               (unsigned long long) edge->tail);

        // if this predecessor has not had its level assigned yet, conduct a
        // recursive call to assign its predecessors and then itself
        if ((pre_node->flags & RDG_NODE_LEVEL_SET) == 0) {
            printf("node %llx says predecessor %llx not set\n",
                   (unsigned long long) edge->tail,
                   (unsigned long long) edge->head);

            rdg_node_assign_level(graph, graph_fetch_node(graph, edge->head));
        }

        if (pre_node->flags & RDG_NODE_LEVEL_SET) {
            if ((rdg_node->flags & RDG_NODE_LEVEL_SET) == 0) {
                new_level = pre_node->level + 1;
                rdg_node->flags |= RDG_NODE_LEVEL_SET;
            }
            else if (pre_node->level + 1 > new_level) {
                new_level = pre_node->level + 1;
            }
        }
        else {
            printf("node %llx says predecessor %llx still not set\n",
                   (unsigned long long) edge->tail,
                   (unsigned long long) edge->head);
        }
    }
    object_delete(predecessors);

    if (rdg_node->flags & RDG_NODE_LEVEL_SET) {
        printf("final level for %llx: %d\n",
               (unsigned long long) rdg_node->index, new_level);
        rdg_node->level = new_level;
    }
}


void rdg_node_assign_level_by_successor (struct _graph_node * node)
{
    struct _rdg_node * rdg_node = node->data;
    struct _graph_edge * edge;
    struct _list_it * it;
    int new_level = -11;

    if (rdg_node->flags & RDG_NODE_LEVEL_SET)
        return;

    printf("rdg_node_assign_level_by_successor node %p %p %llx\n",
           node, node->graph,
           (unsigned long long) rdg_node->index);

    // get lowest level of predecessors
    struct _list * successors = graph_node_successors(node);
    for (it = list_iterator(successors); it != NULL; it = it->next) {
        edge = it->data;
        struct _rdg_node * suc_node = graph_fetch_data(node->graph, edge->tail);

        printf("%llx (%d) -> %llx (%d)\n",
               (unsigned long long) rdg_node->index, rdg_node->flags,
               (unsigned long long) suc_node->index, suc_node->flags);

        // loops don't count
        if (edge->head == edge->tail)
            continue;

        // if this successor has not had its level assigned yet, conduct a
        // recursive call to assign its sucessors and then itself
        if ((suc_node->flags & RDG_NODE_LEVEL_SET) == 0) {
            printf("node %llx says successor %llx not set %d\n",
                   (unsigned long long) rdg_node->index,
                   (unsigned long long) suc_node->index,
                   suc_node->flags);

            rdg_node_assign_level_by_successor(
                                    graph_fetch_node(node->graph, edge->tail));
        }

        if (suc_node->flags & RDG_NODE_LEVEL_SET) {
            if ((rdg_node->flags & RDG_NODE_LEVEL_SET) == 0) {
                new_level = suc_node->level - 1;
                rdg_node->flags |= RDG_NODE_LEVEL_SET;
            }
            else if (suc_node->level - 1 < new_level)
                new_level = suc_node->level - 1;
        }
        else {
            printf("node %llx says successor %llx still not set\n",
                   (unsigned long long) edge->tail,
                   (unsigned long long) edge->head);
        }
    }
    object_delete(successors);
    printf("done\n");

    rdg_node->level = new_level;
}


void rdg_assign_levels (struct _graph * graph, uint64_t top_index)
{
    struct _rdg_node * rdg_node;

    graph_map(graph, rdg_node_level_zero);

    // set level of entry
    rdg_node = graph_fetch_data(graph, top_index);
    rdg_node->flags |= RDG_NODE_LEVEL_SET;

    // set node levels
    graph_bfs(graph, top_index, rdg_node_assign_level);
    graph_map(graph, rdg_node_assign_level_by_successor);

    // adjust for negative levels
    int lowest_level = 0;
    struct _graph_it * it;
    for (it = graph_iterator(graph);
         it != NULL;
         it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);
        if (rdg_node->level < lowest_level)
            lowest_level = rdg_node->level;
    }

    // no negative level, goodbye
    if (lowest_level >= 0)
        return;

    int adjust = 0 - lowest_level;

    for (it = graph_iterator(graph);
         it != NULL;
         it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);
        rdg_node->level += adjust;
    }
}


void rdg_assign_level_map (struct _rdg_graph * rdg_graph)
{
    if (rdg_graph->levels != NULL)
        object_delete(rdg_graph->levels);

    rdg_graph->levels = map_create();

    struct _graph_it * graph_it;
    // for each node in the graph
    for (graph_it = graph_iterator(rdg_graph->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_node = graph_it_data(graph_it);

        // if this level does not exist, create it
        if (map_fetch(rdg_graph->levels, rdg_node->level) == NULL) {
            struct _map * new_level_map = map_create();
            map_insert(rdg_graph->levels, rdg_node->level, new_level_map);
            object_delete(new_level_map);
        }

        // insert this node's index into it's level's map
        struct _map * level_map = map_fetch(rdg_graph->levels, rdg_node->level);
        struct _index * index = index_create(rdg_node->index);
        map_insert(level_map, level_map->size, index);
        object_delete(index);
    }
}


void rdg_create_virtual_nodes (struct _rdg_graph * rdg_graph)
{
    struct _graph_node * node;

    // it is dangerous to modify a graph during iteration. we are going to copy
    // the graph nodes into a queue, then iterate through the queue while we
    // modify the graph
    struct _queue * queue = queue_create();

    // we need indexes for our virtual nodes. they will begin after the highest
    // index in the graph
    uint64_t virtual_index = 0;

    // for each node in the graph
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(rdg_graph->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
         node = graph_it_node(graph_it);
        // add it to the queue
        queue_push(queue, node);

        if (node->index > virtual_index)
            virtual_index = node->index;
    }

    virtual_index++;

    while (queue->size > 0) {
        // get the node out of the queue. don't pop it out of the queue until
        // we're done with it
        node = queue_peek(queue);

        struct _rdg_node * rdg_head = node->data;

        // for each successor
        struct _list * successors = graph_node_successors(node);
        struct _list_it * list_it;
        for (list_it = list_iterator(successors);
             list_it != NULL;
             list_it = list_it->next) {
            struct _graph_edge * edge = list_it->data;
            struct _graph_node * successor;
            successor = graph_fetch_node(rdg_graph->graph, edge->tail);

            struct _rdg_node * rdg_tail = successor->data;

            // if the distance between nodes is 1, ignore, otherwise set
            // direction
            int direction;
            if (rdg_tail->level > rdg_head->level + 1)
                direction = 1;
            else if (rdg_tail->level < rdg_head->level - 1)
                direction = -1;
            else
                continue;

            graph_remove_edge(rdg_graph->graph, node->index, successor->index);

            // add the virtual nodes and their edges
            uint64_t head_index = rdg_head->index;
            int i;
            for (i = rdg_head->level + direction;
                 i != rdg_tail->level;
                 i += direction) {
                // create the virtual node
                struct _rdg_node * virtual;
                virtual = rdg_node_create(virtual_index, NULL);
                virtual->flags |= RDG_NODE_VIRTUAL | RDG_NODE_LEVEL_SET;
                virtual->level = i;
                // add it
                graph_add_node(rdg_graph->graph, virtual_index, virtual);
                // add edge to its parent
                graph_add_edge(rdg_graph->graph,
                               head_index,
                               virtual_index,
                               edge->data);
                head_index = virtual_index;
                virtual_index++;
                // clean up virtual node
                object_delete(virtual);
            }

            // add final edge from last virtual node to successor
            graph_add_edge(rdg_graph->graph,
                           head_index,
                           successor->index,
                           edge->data);
        }

        queue_pop(queue);
        object_delete(successors);
    }

    object_delete(queue);
}


void rdg_set_graph_width (struct _rdg_graph * rdg_graph)
{
    struct _index    * index;
    struct _rdg_node * rdg_node;
    struct _map      * level_map;
    int position, level, x;

    rdg_graph->width = 0;

    // find graph and level widths
    for (level = 0; level < rdg_graph->levels->size; level++) {
        level_map = map_fetch(rdg_graph->levels, level);

        x = 0;
        for (position = 0;
             (index = map_fetch(level_map, position)) != NULL;
             position++) {

            rdg_node = graph_fetch_data(rdg_graph->graph, index->index);

            if (rdg_node_width(rdg_node) + rdg_node->x > x)
                x = rdg_node_width(rdg_node) + rdg_node->x;
        }

        //x -= RDG_NODE_X_SPACING;

        if (x > rdg_graph->width)
            rdg_graph->width = x;
    }
}


int rdg_level_top (struct _rdg_graph * rdg_graph, int level)
{
    if (level > rdg_graph->levels->size)
        return -1;

    struct _map * level_map = map_fetch(rdg_graph->levels, level);
    struct _index * index = map_fetch(level_map, 0);
    if (index == NULL)
        return -1;

    struct _rdg_node * rdg_node;
    rdg_node = graph_fetch_data(rdg_graph->graph, index->index);

    return rdg_node->y;
}


void rdg_assign_x (struct _rdg_graph * rdg_graph)
{
    // we assign an initial x value to all nodes

    struct _map * level_map;
    int level;
    // for each level
    for (level = 0;
         (level_map = map_fetch(rdg_graph->levels, level)) != NULL;
         level++) {

        // assign x value based upon width of previous nodes and spacing
        int position;
        int x = 0;
        struct _index * index;
        for (position = 0;
             (index = map_fetch(level_map, position)) != NULL;
             position++) {
            struct _rdg_node * rdg_node;
            rdg_node = graph_fetch_data(rdg_graph->graph, index->index);

            rdg_node->x = x;

            x += rdg_node_width(rdg_node) + RDG_NODE_X_SPACING;
        }
    }
}


void rdg_assign_y (struct _rdg_graph * rdg_graph)
{
    int level       = 0;
    struct _map      * level_map;
    struct _rdg_node * rdg_node;
    int y = 0;

    rdg_graph->height = 0;

    // for each level
    for (level = 0;
         (level_map = map_fetch(rdg_graph->levels, level)) != NULL;
         level++) {

        // assign current y to all nodes and find tallest node
        struct _index * index;
        size_t position;
        int tallest = 0;
        for (position = 0;
             (index = map_fetch(level_map, position)) != 0;
             position++) {

            rdg_node = graph_fetch_data(rdg_graph->graph, index->index);
            rdg_node->y = y;

            // find tallest node
            if (rdg_node_height(rdg_node) > tallest)
                tallest = rdg_node_height(rdg_node);
        }
        rdg_graph->height = y + tallest;
        y += tallest + RDG_NODE_Y_SPACING;
    }
}


void rdg_assign_position (struct _rdg_graph * rdg_graph)
{
    if (rdg_graph->levels == NULL)
        return;

    // for every node in a level
    // assign it an x value
    int position;
    int level;
    struct _map * level_map;
    // for each level
    for (level = 0;
         (level_map = map_fetch(rdg_graph->levels, level)) != NULL;
         level++) {
        // for each node in this level
        struct _index * index;
        position = 0;
        for (position = 0;
             (index = map_fetch(level_map, position)) != NULL;
             position++) {
            struct _rdg_node * rdg_node;
            printf("rdg_assign_position getting node %llx\n",
                   (unsigned long long) index->index);
            rdg_node = graph_fetch_data(rdg_graph->graph, index->index);
            // assign it's x
            rdg_node->position = position;
        }
    }
}


double rdg_calculate_edge_lengths (struct _rdg_graph * rdg_graph, int level)
{
    double lengths = 0;
    struct _map * level_map = map_fetch(rdg_graph->levels, level);
    int position;
    for (position = 0; position < level_map->size; position++) {
        struct _index * index = map_fetch(level_map, position);
        struct _graph_node * node;
        node = graph_fetch_node(rdg_graph->graph, index->index);
        struct _rdg_node * rdg_this_node = node->data;

        struct _list_it * eit;
        for (eit = list_iterator(node->edges); eit != NULL; eit = eit->next) {
            struct _graph_edge * edge = eit->data;
            struct _rdg_node * rdg_neighbor_node;

            if (edge->head == rdg_this_node->index)
                rdg_neighbor_node = graph_fetch_data(rdg_graph->graph, edge->tail);
            else
                rdg_neighbor_node = graph_fetch_data(rdg_graph->graph, edge->head);

            int src_x = rdg_node_source_x(rdg_graph,
                                          rdg_this_node,
                                          rdg_neighbor_node);
            int src_y = rdg_node_source_y(rdg_graph,
                                          rdg_this_node,
                                          rdg_neighbor_node);
            int dst_x = rdg_node_sink_x(rdg_graph,
                                        rdg_this_node,
                                        rdg_neighbor_node);
            int dst_y = rdg_node_sink_y(rdg_graph,
                                        rdg_this_node,
                                        rdg_neighbor_node);
            double tmp;
            if (src_x > dst_x) tmp  = src_x - dst_x;
            else               tmp  = dst_x - src_x;
            if (src_y > dst_y) tmp += src_y - dst_y;
            else               tmp += dst_y - src_y;
            tmp /= 2.0;
            lengths += tmp;
        }
    }

    return lengths;
}


void rdg_reduce_edge_lengths (struct _rdg_graph * rdg_graph)
{
    int level;

    for (level = 0; level < rdg_graph->levels->size; level++) {
        struct _map * level_map = map_fetch(rdg_graph->levels, level);
        int position;
        for (position = 0; position < level_map->size; level_map++) {
//            struct _index * index = map_fetch(level_map, position);

        }
    }
}


int rdg_level_count_edge_crossings (struct _rdg_graph * rdg_graph, int level)
{
    int position;
    int crossings = 0;
    struct _rdg_node   * rdg_head;
    struct _rdg_node   * rdg_tail;
    struct _map * level_map      = map_fetch(rdg_graph->levels, level);
    struct _map * next_level_map = map_fetch(rdg_graph->levels, level + 1);

    if ((level_map->size == 1) || (next_level_map == NULL))
        return 0;

    struct _map * edges = map_create();

    // for every edge in this level
    struct _index * index;
    for (position = 0;
         (index = map_fetch(level_map, position)) != NULL;
         position++) {
        struct _graph_node * node = graph_fetch_node(rdg_graph->graph,
                                                     index->index);
        rdg_head = node->data;

        struct _list_it * eit;
        for (eit  = list_iterator(node->edges);
             eit != NULL;
             eit  = eit->next) {
            struct _graph_edge * edge = eit->data;

            if (edge->head == rdg_head->index)
                rdg_tail = graph_fetch_data(rdg_graph->graph, edge->tail);
            else
                rdg_tail = graph_fetch_data(rdg_graph->graph, edge->head);

            if (rdg_tail->level < rdg_head->level)
                continue;

            // we shouldn't do this, but we're going to steal _graph_edge
            //if (edge->head == rdg_head->index)
                edge = graph_edge_create(rdg_head->position, rdg_tail->position, NULL);
                /*
                edge = graph_edge_create(rdg_node_source_x(rdg_graph,
                                                           rdg_head,
                                                           rdg_tail),
                                         rdg_node_sink_x(rdg_graph,
                                                         rdg_head,
                                                         rdg_tail),
                                         NULL);
                                         */
            //else
            //    edge = graph_edge_create(rdg_tail->x, rdg_head->x, NULL);
            /*
                edge = graph_edge_create(rdg_node_source_x(rdg_graph,
                                                           rdg_tail,
                                                           rdg_head),
                                         rdg_node_sink_x(rdg_graph,
                                                         rdg_tail,
                                                         rdg_head),
                                         NULL);
                                         */
                //edge = graph_edge_create(rdg_node_center_x(rdg_tail),
                //                         rdg_node_center_x(rdg_head), NULL);
            map_insert(edges, edges->size, edge);
            object_delete(edge);
        }
    }

    int i, j;
    for (i = 0; i < edges->size; i++) {
        for (j = i + 1; j < edges->size; j++) {
            struct _graph_edge * a = map_fetch(edges, i);
            struct _graph_edge * b = map_fetch(edges, j);
            if (    (a->head < b->head)
                 && (a->tail > b->tail)) {
                crossings++;
            }
            else if ((a->head > b->head) && (a->tail < b->tail))
                crossings++;
        }
    }

    object_delete(edges);

    return crossings;
}


int rdg_count_edge_crossings (struct _rdg_graph * rdg_graph)
{
    int crossings = 0;
    int level = 0;

    for (level = 0; level < rdg_graph->levels->size; level++) {
        crossings += rdg_level_count_edge_crossings(rdg_graph, level);
    }

    return crossings;
}


inline void rdg_swap_node_positions (struct _rdg_graph * rdg_graph,
                              int level,
                              int left,
                              int right)
{
    struct _rdg_node * rdg_left_node;
    struct _rdg_node * rdg_right_node;
    struct _index * index;
    
    struct _map * level_map = map_fetch(rdg_graph->levels, level);

    // fetch nodes
    index = map_fetch(level_map, left);
    rdg_left_node = graph_fetch_data(rdg_graph->graph, index->index);
    index = map_fetch(level_map, right);
    rdg_right_node = graph_fetch_data(rdg_graph->graph, index->index);

    // swap positions internally
    rdg_left_node->position  = right;
    rdg_right_node->position = left;

    // remove and readd to map in swapped positions
    map_remove(level_map, left);
    map_remove(level_map, right);

    index = index_create(rdg_right_node->index);
    map_insert(level_map, left, index);
    object_delete(index);

    index = index_create(rdg_left_node->index);
    map_insert(level_map, right, index);
    object_delete(index);
}


int rdg_reduce_end_x (struct _rdg_graph * rdg_graph, struct _rdg_node * rdg_node)
{
    if ((rdg_node->flags & RDG_NODE_VIRTUAL) == 0)
    {
        return rdg_node_center_x(rdg_node);
    }

    // trace this virtual node down to the x coordinate of its end
    struct _graph_node * node;
    struct _rdg_node * rdg_next_node = rdg_node;
    while (1) {
        node = graph_fetch_node(rdg_graph->graph, rdg_next_node->index);
        rdg_next_node = node->data;
        if ((rdg_next_node->flags & RDG_NODE_VIRTUAL) == 0)
            break;

        struct _list_it * it;
        for (it = list_iterator(node->edges); it != NULL; it = it->next) {
            struct _graph_edge * edge = it->data;
            struct _rdg_node * suc_node;
            if (edge->head == rdg_next_node->index) {
                suc_node = graph_fetch_data(rdg_graph->graph, edge->tail);

            }
            else
                suc_node = graph_fetch_data(rdg_graph->graph, edge->head);

            if (suc_node->level < rdg_next_node->level) {
                rdg_next_node = suc_node;
                break;
            }
        }
    }

    return rdg_node_center_x(rdg_next_node);
}


void rdg_reduce_level_heuristic (struct _rdg_graph * rdg_graph, int level)
{
    struct _map * level_map = map_fetch(rdg_graph->levels, level);
    int * positions = malloc(sizeof(int) * level_map->size);
    memset(positions, 0, sizeof(int) * level_map->size);
    int i, j, edges_counted;
    
    for (i = 0; i < level_map->size; i++)
        positions[i] = 0.0;

    // for each node in this level, calculate the average position of the nodes
    // beneath it
    for (i = 0; i < level_map->size; i++) {
        struct _index * index = map_fetch(level_map, i);
        struct _graph_node * node;
        node = graph_fetch_node(rdg_graph->graph, index->index);
        struct _rdg_node * rdg_node = node->data;

        struct _list_it * it;
        edges_counted = 0;
        for (it = list_iterator(node->edges); it != NULL; it = it->next) {
            struct _graph_edge * edge = it->data;
            struct _rdg_node * suc_node;

            if (edge->head == node->index)
                suc_node = graph_fetch_data(rdg_graph->graph, edge->tail);
            else
                suc_node = graph_fetch_data(rdg_graph->graph, edge->head);

            // if this node is the head, then suc node should be below this node
            if ((edge->head == node->index) && (suc_node->level < rdg_node->level)) {
                continue;
            }
            // if this node is the tail, then suc node should be above this node
            else if (    (edge->tail == node->index) 
                      && (suc_node->level < rdg_node->level)) {
                continue;
            }

            edges_counted++;

            // if rdg_node is head, then suc_node is tail below rdg_node
            if (edge->head == node->index)
                positions[i] += rdg_node_center_x(suc_node);//rdg_reduce_end_x(rdg_graph, suc_node);//suc_node->x;//rdg_node_sink_x(rdg_graph, rdg_node, suc_node);
            // else, suc_node is travelling upwards to rdg_node
            else
                positions[i] += rdg_node_center_x(rdg_node);//rdg_reduce_end_x(rdg_graph, rdg_node);//->x;//rdg_node_center_x(rdg_node);//rdg_node_source_x(rdg_graph, rdg_node, suc_node);
        }
        if (edges_counted == 0)
            printf("counted 0 edges for level %d position %d\n", level, i);
        else
            positions[i] /= edges_counted;
        printf("node %llx, edges_counted %d, level %d, positions[%d] %d\n",
               (unsigned long long) node->index,
               edges_counted, level, i, positions[i]);
    }

    // sort nodes in this level by average location of successor position,
    // and do it as inefficiently as possible
    i = 0;
    while (i < level_map->size) {
        for (j = i + 1; j < level_map->size; j++) {
            if (positions[j] < positions[i]) {
                int tmp = positions[j];
                positions[j] = positions[i];
                positions[i] = tmp;
                rdg_swap_node_positions(rdg_graph, level, i, j);
                i = -1;
                break;
            }
        }
        i++;
    }

    free(positions);
}


// RANDOM_SWAP_ITERATIONS higher = better quality, slower generation
void rdg_random_permute_swap (struct _rdg_graph * rdg_graph,
                              int * random_numbers,
                              int   swap_iterations)
{

    // pick 16 random levels and randomly swap a node in each
    int level;
    int node_a, node_b;
    int i;
    int rand_i = 0;

    for (i = 0; i < swap_iterations; i++) {
        level  = random_numbers[rand_i++];
        node_a = random_numbers[rand_i++];
        node_b = random_numbers[rand_i++];

        level %= rdg_graph->levels->size;
        struct _map * level_map = map_fetch(rdg_graph->levels, level);
        node_a %= level_map->size;
        node_b %= level_map->size;

        if (node_a == node_b)
            continue;

        //printf("swap level %d, %d <-> %d\n", level, node_a, node_b);
        rdg_swap_node_positions(rdg_graph, level, node_a, node_b);
    }
}


void rdg_random_permute_unswap (struct _rdg_graph * rdg_graph,
                                int * random_numbers,
                                int   swap_iterations)
{

    // pick 16 random levels and randomly swap a node in each
    int level;
    int node_a, node_b;
    int i;
    int rand_i = swap_iterations * 3;
    rand_i--;

    for (i = swap_iterations - 1; i >= 0; i--) {
        node_b = random_numbers[rand_i--];
        node_a = random_numbers[rand_i--];
        level = random_numbers[rand_i--];

        level %= rdg_graph->levels->size;
        struct _map * level_map = map_fetch(rdg_graph->levels, level);
        node_a %= level_map->size;
        node_b %= level_map->size;

        if (node_a == node_b)
            continue;

        //printf("swap level %d, %d <-> %d\n", level, node_a, node_b);
        rdg_swap_node_positions(rdg_graph, level, node_a, node_b);
    }
}


void rdg_random_permute (struct _rdg_graph * rdg_graph, int swap_iterations)
{
    int * random_numbers = (int *) malloc(sizeof(int) * swap_iterations * 3);

    // we seed it with top_index to get consistent looking graphs each time
    srand(rdg_graph->top_index);
    int i;
    for (i = 0; i < swap_iterations * 3; i++)
        random_numbers[i] = rand();

    int crossings = rdg_count_edge_crossings(rdg_graph);
    rdg_random_permute_swap(rdg_graph, random_numbers, swap_iterations);
    int new_crossings = rdg_count_edge_crossings(rdg_graph);
    if (new_crossings >= crossings)
        rdg_random_permute_unswap(rdg_graph, random_numbers, swap_iterations);

    free(random_numbers);
}


void rdg_reduce_edge_crossings (struct _rdg_graph * rdg_graph)
{
    int level;
    int crossings;
    //int new_crossings;
    int iteration_crossings;


    iteration_crossings = rdg_count_edge_crossings(rdg_graph);
    while (1) {

        // find global solution through random permutation
        for (level = 0; level < rdg_graph->levels->size; level++)
            rdg_random_permute(rdg_graph, 64);

        /*
        // find local solutions
        for (level = 0; level < rdg_graph->levels->size - 1; level++) {

            crossings = rdg_count_edge_crossings(rdg_graph);

            // try swapping this level
            int i, j;
            struct _map * level_map = map_fetch(rdg_graph->levels, level);

            for (i = 0; i < level_map->size; i++) {
                for (j = 0; j < level_map->size; j++) {
                    if (i == j)
                        continue;
                    rdg_swap_node_positions(rdg_graph, level, i, j);

                    new_crossings = rdg_count_edge_crossings(rdg_graph);

                    if (new_crossings < crossings)
                        crossings = new_crossings;
                    else
                        rdg_swap_node_positions(rdg_graph, level, i, j);
                    if (crossings == 0)
                        break;
                }
            }
        }
        */

        crossings = rdg_count_edge_crossings(rdg_graph);
        if (crossings == iteration_crossings)
            break;
        iteration_crossings = crossings;
    }
    printf("edge crossings: %d\n", rdg_count_edge_crossings(rdg_graph));
}


// http://kapo-cpp.blogspot.com/2008/10/drawing-arrows-with-cairo.html
void rdg_draw_arrow (cairo_t * ctx,
                     double src_x,
                     double src_y,
                     double dst_x,
                     double dst_y,
                     int arrow_type) {

    double angle = atan2(dst_y - src_y, dst_x - src_x) + M_PI;

    double x1 = dst_x + RDG_ARROW_LENGTH * cos(angle - RDG_ARROW_DEGREES);
    double y1 = dst_y + RDG_ARROW_LENGTH * sin(angle - RDG_ARROW_DEGREES);
    double x2 = dst_x + RDG_ARROW_LENGTH * cos(angle + RDG_ARROW_DEGREES);
    double y2 = dst_y + RDG_ARROW_LENGTH * sin(angle + RDG_ARROW_DEGREES);

    if (arrow_type == RDG_ARROW_TYPE_OPEN) {
        cairo_move_to(ctx, dst_x, dst_y);
        cairo_line_to(ctx, x1,    y1);
        cairo_stroke(ctx);

        cairo_move_to(ctx, dst_x, dst_y);
        cairo_line_to(ctx, x2,    y2);
        cairo_stroke(ctx);
    }
    else if (arrow_type == RDG_ARROW_TYPE_FILL) {
        cairo_move_to(ctx, dst_x, dst_y);
        cairo_line_to(ctx, x1, y1);
        cairo_line_to(ctx, x2, y2);
        cairo_line_to(ctx, dst_x, dst_y);
        cairo_close_path(ctx);
        cairo_fill(ctx);
    }

}


void rdg_draw_edge (struct _rdg_graph * rdg_graph, struct _graph_edge * edge)
{
    struct _rdg_node * rdg_head = graph_fetch_data(rdg_graph->graph, edge->head);
    struct _rdg_node * rdg_tail = graph_fetch_data(rdg_graph->graph, edge->tail);

    cairo_t * ctx = cairo_create(rdg_graph->surface);
    cairo_set_line_width(ctx, 2.0);

    struct _ins_edge * ins_edge = edge->data;
    if (ins_edge == NULL) {
        fprintf(stderr, "rdg_draw_edge ins_edge was null");
        cairo_set_source_rgb(ctx, RDG_EDGE_NORMAL_COLOR);
    }
    else if (ins_edge->type == INS_EDGE_NORMAL)
        cairo_set_source_rgb(ctx, RDG_EDGE_NORMAL_COLOR);

    else if (ins_edge->type == INS_EDGE_JUMP)
        cairo_set_source_rgb(ctx, RDG_EDGE_JUMP_COLOR);

    else if (ins_edge->type == INS_EDGE_JCC_TRUE)
        cairo_set_source_rgb(ctx, RDG_EDGE_JCC_TRUE_COLOR);

    else if (ins_edge->type == INS_EDGE_JCC_FALSE)
        cairo_set_source_rgb(ctx, RDG_EDGE_JCC_FALSE_COLOR);
    else {
        fprintf(stderr, "rdg_draw_edge ins_edge->type was %d\n", ins_edge->type);
        cairo_set_source_rgb(ctx, RDG_EDGE_NORMAL_COLOR);
    }

    // self referential edge
    if (edge->head == edge->tail) {
        int x1 = rdg_node_center_x(rdg_head) - 10;
        int x2 = x1 + 10;
        int x3 = x2 + 10;
        int y1 = rdg_head->y + rdg_node_height(rdg_head) + 12;
        int y2 = y1 + 40;
        int y3 = y1;

        cairo_curve_to(ctx, x1, y1, x2, y2, x3, y3);
        cairo_stroke(ctx);

        rdg_draw_arrow(ctx, x3, y2, x3, y3, RDG_ARROW_TYPE_FILL);
        return;
    }

    // this edge is going down
    int x1, x2, x3, x4, x5, y1, y2, y3, y4, y5;
    if (rdg_head->level < rdg_tail->level) {
        /*
          1
          |
          2----3----4
                    |
                    5
        */
        x1 = rdg_node_source_x(rdg_graph, rdg_head, rdg_tail);
        y1 = rdg_node_source_y(rdg_graph, rdg_head, rdg_tail);
        x5 = rdg_node_sink_x(rdg_graph, rdg_head, rdg_tail);
        y5 = rdg_node_sink_y(rdg_graph, rdg_head, rdg_tail);

        x1 += RDG_SURFACE_PADDING;
        y1 += RDG_SURFACE_PADDING;
        x5 += RDG_SURFACE_PADDING;
        y5 += RDG_SURFACE_PADDING;

        x3  = x1 + ((x5 - x1) / 2);
        y3  = y5 - (RDG_NODE_Y_SPACING - (RDG_NODE_Y_SPACING / 4));
        y3 += (5 * rdg_head->position);
        x2  = x1;
        y2  = y3;
        x4  = x5;
        y4  = y3;
    }
    // going up to see the big guy in the sky
    else {
        /*
          5
          |
          4----3----2
                    |
                    1
        */
        x1 = rdg_node_source_x(rdg_graph, rdg_head, rdg_tail);
        y1 = rdg_node_source_y(rdg_graph, rdg_head, rdg_tail);
        x5 = rdg_node_sink_x(rdg_graph, rdg_head, rdg_tail);
        y5 = rdg_node_sink_y(rdg_graph, rdg_head, rdg_tail);

        x1 += RDG_SURFACE_PADDING;
        y1 += RDG_SURFACE_PADDING;
        x5 += RDG_SURFACE_PADDING;
        y5 += RDG_SURFACE_PADDING;

        x3  = x5 + ((x1 - x5) / 2);
        y3  = y1 - (RDG_NODE_Y_SPACING / 4);
        y3 += (5 * rdg_head->position);
        x2  = x1;
        y2  = y3;
        x4  = x5;
        y4  = y3;
    }

    // if this edge is going straight down, draw a striaght line
    if ((rdg_head->x == rdg_tail->x)) {
        cairo_move_to(ctx, x1, y1);
        cairo_line_to(ctx, x5, y5);
        cairo_stroke(ctx);
    }
    else {
        cairo_curve_to(ctx, x1, y1, x2, y2, x3, y3);
        cairo_move_to(ctx, x3, y3);
        cairo_curve_to(ctx, x3, y3, x4, y4, x5, y5);
        cairo_stroke(ctx);
    }

    // this edge ends in a non-virtual node
    if ((rdg_tail->flags & RDG_NODE_VIRTUAL) == 0) {
        int arrow_type = RDG_ARROW_TYPE_FILL;
        if (rdg_tail->level == rdg_head->level)
            rdg_draw_arrow(ctx, x1, y1, x5, y5, arrow_type);
        else
            rdg_draw_arrow(ctx, x4, y4, x5, y5, arrow_type);
    }

    cairo_destroy(ctx);
}


void rdg_draw (struct _rdg_graph * rdg_graph)
{

    if (rdg_graph->surface != NULL)
        cairo_surface_destroy(rdg_graph->surface);
    rdg_graph->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                   rdg_graph->width + RDG_SURFACE_PADDING * 2,
                                   rdg_graph->height + RDG_SURFACE_PADDING * 2);

    // for each node, draw the node and its edges
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(rdg_graph->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_node = graph_it_data(graph_it);

        /*
        if (rdg_node->flags & RDG_NODE_VIRTUAL)
            continue;
        */

        // if this node is not virtual, draw it
        if ((rdg_node->flags & RDG_NODE_VIRTUAL) == 0) {
            cairo_t * ctx = cairo_create(rdg_graph->surface);
            /*
            printf("drawing surface %p from %llx to %d %d (%d by %d)\n",
                   rdg_node->surface,
                   (unsigned long long) rdg_node->index,
                   rdg_node->x, rdg_node->y,
                   rdg_node_width(rdg_node), rdg_node_height(rdg_node));
                   */

            cairo_set_source_surface(ctx,
                                     rdg_node->surface,
                                     rdg_node->x + RDG_SURFACE_PADDING,
                                     rdg_node->y + RDG_SURFACE_PADDING);
            cairo_rectangle(ctx,
                            rdg_node->x + RDG_SURFACE_PADDING,
                            rdg_node->y + RDG_SURFACE_PADDING,
                            rdg_node_width(rdg_node),
                            rdg_node_height(rdg_node));
            cairo_fill(ctx);
            cairo_destroy(ctx);
        }

        // draw edges
        struct _list_it * eit;
        struct _list * successors = graph_node_successors(graph_it_node(graph_it));
        for (eit = list_iterator(successors); eit != NULL; eit = eit->next) {
            struct _graph_edge * edge = eit->data;
            rdg_draw_edge(rdg_graph, edge);
        }
        object_delete(successors);
    }
    cairo_surface_write_to_png(rdg_graph->surface, "cairo.png");
}


void rdgraph_draw_node_test (const char * filename, struct _graph_node * node)
{
    cairo_surface_t * surface = rdgraph_draw_graph_node(node);
    cairo_surface_write_to_png(surface, filename);
    cairo_surface_destroy(surface);
}


struct _rdstring * rdgraph_graphviz_string_node (struct _graph * graph,
                                                 uint64_t index)
{
    struct _rdstring * base = rdstring_create("");
    char tmp[128];

    struct _list    * list;
    struct _list_it * it;

    list = graph_fetch_data(graph, index);
    if (list == NULL)
        return NULL;

    // create instructions
    snprintf(tmp, 128, "%lld [label=\"", (unsigned long long) index);
    rdstring_cat(base, tmp);
    for (it = list_iterator(list); it != NULL; it = it->next) {
        struct _ins * ins = it->data;

        snprintf(tmp, 128, "%08llx  ", (unsigned long long) ins->address);
        rdstring_cat(base, tmp);
        snprintf(tmp, 128, "%s", ins->description);
        rdstring_cat(base, tmp);
        rdstring_cat(base, "\\l");
        //rdstring_cat(base, "</td></tr>");
    }
    rdstring_cat(base, "\", fontname=\"monospace\" shape=\"box\"]\n");

    // create edges. only use successors
    struct _graph_node * node = graph_fetch_node(graph, index);
    list = graph_node_successors(node);
    if (list == NULL)
        return NULL;
    for (it = list_iterator(list); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;

        if (edge->head == index) {
            snprintf(tmp, 128, "%lld -> %lld\n",
                     (unsigned long long) edge->head,
                     (unsigned long long) edge->tail);

            rdstring_cat(base, tmp);
        }
    }
    object_delete(list);

    return base;
}


char * rdgraph_graphviz_string_params (struct _graph * graph, const char * params)
{
    struct _graph_it * it;
    struct _rdstring * graph_str = rdstring_create("digraph rdis {\n");
    rdstring_cat(graph_str, params);

    for (it = graph_iterator(graph); it != NULL; it = graph_it_next(it)) {
        struct _rdstring * node_str;
        node_str = rdgraph_graphviz_string_node(graph, graph_it_index(it));
        rdstring_append(graph_str, node_str);
        object_delete(node_str);
    }

    rdstring_cat(graph_str, "\n}");

    char * result = strdup(graph_str->string);

    object_delete(graph_str);

    return result;
}


char * rdgraph_graphviz_string (struct _graph * graph)
{
    return rdgraph_graphviz_string_params(graph, "");
}


int rdgraph_graph_to_png_file_params (struct _graph * graph,
                                      const char * filename,
                                      const char * params)
{
    char * graphviz_string = rdgraph_graphviz_string_params(graph, params);

    FILE * fh;

    fh = fopen("/tmp/rdistmp", "w");
    if (fh == NULL)
        return -1;

    fwrite(graphviz_string, 1, strlen(graphviz_string), fh);
    fclose(fh);
    free(graphviz_string);

    // this is so bad it's sickening
    char tmp[256];
    snprintf(tmp, 256, "dot -Tpng /tmp/rdistmp -o %s", filename);

    return system(tmp);
}


int rdgraph_graph_to_png_file (struct _graph * graph, const char * filename)
{
    return rdgraph_graph_to_png_file_params(graph, filename, "");
}
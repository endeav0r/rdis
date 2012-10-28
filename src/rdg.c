#include "rdg.h"

#include "graph.h"
#include "instruction.h"
#include "label.h"
#include "list.h"
#include "rdstring.h"
#include "queue.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static const struct _object rdg_object = {
    (void   (*) (void *))         rdg_delete, 
    (void * (*) (void *))         rdg_copy,
    NULL,
    NULL
};


static const struct _object rdg_node_color_object = {
    (void   (*) (void *))         rdg_node_color_delete, 
    (void * (*) (void *))         rdg_node_color_copy,
    (int    (*) (void *, void *)) rdg_node_color_cmp,
    NULL
};


void rdg_debug (struct _rdg * rdg) {
    struct _graph_it * graph_it;

    for (graph_it = graph_iterator(rdg->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_node = graph_it_data(graph_it);
        printf("index: %llx, level: %d, position: %f, flags: %d, x: %d, y: %d\n",
               (unsigned long long) rdg_node->index,
               rdg_node->level,
               rdg_node->position,
               rdg_node->flags,
               rdg_node->x,
               rdg_node->y);
    }
}


struct _rdg * rdg_create (uint64_t        top_index,
                          struct _graph * graph,
                          struct _map   * labels)
{
    struct _rdg * rdg;

    rdg = (struct _rdg *) malloc(sizeof(struct _rdg));
    rdg->object      = &rdg_object;
    rdg->surface     = NULL;
    rdg->top_index   = top_index;
    rdg->graph       = graph_create();
    rdg->levels      = NULL;
    rdg->width       = 0;
    rdg->height      = 0;

    // add nodes to rdg->graph and acyclic graph
    struct _graph * acyclic_graph = graph_create();
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _graph_node * node = graph_it_node(graph_it);

        cairo_surface_t * surface;
        surface = rdg_node_draw(node, labels);
        struct _rdg_node * rdg_node = rdg_node_create(node->index, surface);
        graph_add_node(rdg->graph, node->index, rdg_node);
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

            graph_add_edge(rdg->graph, edge->head, edge->tail, edge->data);
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
        struct _rdg_node * rdg_node = graph_fetch_data(rdg->graph, rdg_acyclic_node->index);
        rdg_node->level = rdg_acyclic_node->level;
    }

    object_delete(acyclic_graph);

    rdg_create_virtual_nodes(rdg);
    rdg_assign_level_map(rdg);
    rdg_assign_initial_position(rdg);
    rdg_reduce_edge_crossings(rdg);
    //rdg_remove_virtual_nodes(rdg);

    rdg_reduce_and_draw (rdg);

    return rdg;
}


void rdg_delete (struct _rdg * rdg)
{
    if (rdg->surface != NULL)
        cairo_surface_destroy(rdg->surface);
    
    if (rdg->levels != NULL)
        object_delete(rdg->levels);

    object_delete(rdg->graph);
    free(rdg);
}


struct _rdg * rdg_copy (struct _rdg * rdg)
{
    struct _rdg * new_rdg;

    new_rdg = (struct _rdg *) malloc(sizeof(struct _rdg));

    new_rdg->object    = &rdg_object;
    if (rdg->surface == NULL)
        new_rdg->surface = NULL;
    else
        new_rdg->surface   = cairo_surface_copy(rdg->surface);
    
    new_rdg->top_index = rdg->top_index;
    new_rdg->graph     = object_copy(rdg->graph);
    
    if (rdg->levels != NULL)
        new_rdg->levels = object_copy(rdg->levels);
    else
        new_rdg->levels = NULL;
    
    new_rdg->width  = rdg->width;
    new_rdg->height = rdg->height;

    return new_rdg;
}


void rdg_reduce_and_draw (struct _rdg * rdg)
{
    rdg_assign_y(rdg);
    rdg_assign_x(rdg);
    //rdg_left_adjust_x(rdg);
    //rdg_assign_x(rdg);
    rdg_set_graph_width(rdg);
    //rdg_debug(rdg);
    rdg_draw(rdg);
}


int rdg_width (struct _rdg * rdg)
{
    return rdg->width + RDG_SURFACE_PADDING * 2;
}


void rdg_color_nodes (struct _rdg * rdg,
                      struct _graph     * ins_graph,
                      struct _map       * labels,
                      struct _list * node_color_list)
{
    rdg_custom_nodes(rdg, ins_graph, labels, node_color_list, -1);
}


void rdg_custom_nodes (struct _rdg * rdg,
                       struct _graph     * ins_graph,
                       struct _map       * labels,
                       struct _list * node_color_list,
                       uint64_t highlight_ins)
{
    if (node_color_list == NULL)
        return;

    struct _list_it * it;
    for (it = list_iterator(node_color_list); it != NULL; it = it->next) {
        struct _rdg_node_color * rdg_node_color = it->data;

        struct _rdg_node   * rdg_node;
        struct _graph_node * ins_node;

        ins_node = graph_fetch_node(ins_graph, rdg_node_color->index);
        if (ins_node == NULL)
            continue;

        rdg_node = graph_fetch_data(rdg->graph, rdg_node_color->index);
        if (rdg_node == NULL)
            continue;

        if (rdg_node->flags & RDG_NODE_VIRTUAL)
            continue;

        if (rdg_node->surface != NULL)
            cairo_surface_destroy(rdg_node->surface);

        rdg_node->surface = rdg_node_draw_full(ins_node,
                                               labels,
                                               rdg_node_color->red,
                                               rdg_node_color->blue,
                                               rdg_node_color->green,
                                               highlight_ins);
    }
}


struct _rdg_node_color * rdg_node_color_create (uint64_t index,
                                                double red,
                                                double green,
                                                double blue)
{
    struct _rdg_node_color * rdg_node_color;

    rdg_node_color = (struct _rdg_node_color *)
                                        malloc(sizeof(struct _rdg_node_color));

    rdg_node_color->object = &rdg_node_color_object;
    rdg_node_color->index  = index;
    rdg_node_color->red    = red;
    rdg_node_color->green  = green;
    rdg_node_color->blue   = blue;

    return rdg_node_color;
}


void rdg_node_color_delete (struct _rdg_node_color * rdg_node_color)
{
    free(rdg_node_color);
}


struct _rdg_node_color * rdg_node_color_copy (struct _rdg_node_color * rdg_node_color)
{
    return rdg_node_color_create(rdg_node_color->index,
                                 rdg_node_color->red,
                                 rdg_node_color->green,
                                 rdg_node_color->blue);
}


int rdg_node_color_cmp (struct _rdg_node_color * lhs, struct _rdg_node_color * rhs)
{
    if (lhs->index < rhs->index)
        return -1;
    else if (lhs->index > rhs->index)
        return 1;
    return 0;
}


int rdg_height (struct _rdg * rdg)
{
    return rdg->height + RDG_SURFACE_PADDING * 2;
}


uint64_t rdg_get_node_by_coords (struct _rdg * rdg, int x, int y)
{
    struct _graph_it * it;

    for (it = graph_iterator(rdg->graph);
         it != NULL;
         it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);

        if (    (rdg_node->x + RDG_SURFACE_PADDING < x)
             && (rdg_node->x + RDG_SURFACE_PADDING + rdg_node_width(rdg_node) > x)
             && (rdg_node->y + RDG_SURFACE_PADDING < y)
             && (rdg_node->y + RDG_SURFACE_PADDING + rdg_node_height(rdg_node) > y))
            return rdg_node->index;
    }

    return -1;
}


uint64_t rdg_get_ins_by_coords (struct _rdg * rdg,
                                struct _graph * ins_graph,
                                int x, int y)
{
    uint64_t node_index = rdg_get_node_by_coords(rdg, x, y);
    if (node_index == -1)
        return -1;

    struct _graph_node * node = graph_fetch_node(ins_graph, node_index);
    if (node == NULL)
        return -1;

    struct _rdg_node * rdg_node = graph_fetch_data(rdg->graph, node_index);

    cairo_t            * ctx;
    cairo_font_extents_t fe;
    ctx = cairo_create(rdg_node->surface);
    cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);
    cairo_select_font_face(ctx,
                           RDG_NODE_FONT_FACE,
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(ctx, RDG_NODE_FONT_SIZE);
    cairo_font_extents(ctx, &fe);
    cairo_destroy(ctx);


    //printf("%d %d %f %f\n", rdg_node->y, RDG_SURFACE_PADDING, RDG_NODE_PADDING, fe.height);

    //printf("rdg_node->y: %d\n", rdg_node->y);

    double bottom = rdg_node->y 
                    + (double) RDG_SURFACE_PADDING 
                    + RDG_NODE_PADDING 
                    + fe.height - 4.0;
    struct _list_it * it;
    for (it = list_iterator(node->data); it != NULL; it = it->next) {
        bottom = bottom;//; + 1.0;
        double top = bottom + fe.height + 2.0;
        if (((double) y >= bottom) && ((double) y <= top)) {
            struct _ins * ins = it->data;
            return ins->address;
        }
        bottom = top;
    }

    return -1;
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


int rdg_node_source_x (struct _rdg * rdg,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node)
{
    if (src_node->level == dst_node->level) {
        if (src_node->position < dst_node->position)
            return src_node->x + rdg_node_width(src_node);
        else
            return src_node->x;
    }

    double position = rdg_node_center_x(dst_node);
    position /= rdg_width(rdg);
    return src_node->x + ((double) rdg_node_width(src_node) * position);
}


int rdg_node_source_y (struct _rdg * rdg,
                       struct _rdg_node * src_node,
                       struct _rdg_node * dst_node)
{
    if (src_node->level == dst_node->level)
        return src_node->y + (rdg_node_height(src_node) / 2);

    if (src_node->y < dst_node->y)
        return src_node->y + rdg_node_height(src_node);
    return src_node->y;
}


int rdg_node_sink_x (struct _rdg * rdg,
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
    position /= rdg_width(rdg);
    return dst_node->x + ((double) rdg_node_width(dst_node) * position);
}


int rdg_node_sink_y (struct _rdg * rdg,
                     struct _rdg_node * src_node,
                     struct _rdg_node * dst_node)
{
    if (src_node->level == dst_node->level)
        return src_node->y + (rdg_node_height(src_node) / 2);

    if (src_node->y < dst_node->y)
        return dst_node->y;
    return dst_node->y + rdg_node_height(dst_node);
}



/************************************************
* Code to Acyclicize the graph                  *
************************************************/

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
        graph_remove_edge(graph, edge->head, edge->tail);
        queue_pop(queue);
    }

    object_delete(queue);
}




/************************************************
* Code to set the level of graph nodes          *
************************************************/

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

    // get highest level of predecessors
    struct _list * predecessors = graph_node_predecessors(node);
    for (it = list_iterator(predecessors); it != NULL; it = it->next) {
        edge = it->data;
        struct _rdg_node * pre_node = graph_fetch_data(graph, edge->head);

        // loops don't count
        if (edge->head == edge->tail)
            continue;

        // if this predecessor has not had its level assigned yet, conduct a
        // recursive call to assign its predecessors and then itself
        if ((pre_node->flags & RDG_NODE_LEVEL_SET) == 0) {
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
    }
    object_delete(predecessors);

    if (rdg_node->flags & RDG_NODE_LEVEL_SET) {
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
    }
    object_delete(successors);

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


void rdg_assign_level_map (struct _rdg * rdg)
{
    if (rdg->levels != NULL)
        object_delete(rdg->levels);

    rdg->levels = map_create();

    struct _graph_it * graph_it;
    // for each node in the graph
    for (graph_it = graph_iterator(rdg->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_node = graph_it_data(graph_it);

        // if this level does not exist, create it
        if (map_fetch(rdg->levels, rdg_node->level) == NULL) {
            struct _map * new_level_map = map_create();
            map_insert(rdg->levels, rdg_node->level, new_level_map);
            object_delete(new_level_map);
        }

        // insert this node's index into it's level's map
        struct _map * level_map = map_fetch(rdg->levels, rdg_node->level);
        struct _index * index = index_create(rdg_node->index);
        map_insert(level_map, level_map->size, index);
        object_delete(index);
    }
}


void rdg_create_virtual_nodes (struct _rdg * rdg)
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
    for (graph_it = graph_iterator(rdg->graph);
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
            successor = graph_fetch_node(rdg->graph, edge->tail);

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

            graph_remove_edge(rdg->graph, node->index, successor->index);

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
                graph_add_node(rdg->graph, virtual_index, virtual);
                // add edge to its parent
                graph_add_edge(rdg->graph,
                               head_index,
                               virtual_index,
                               edge->data);
                head_index = virtual_index;
                virtual_index++;
                // clean up virtual node
                object_delete(virtual);
            }

            // add final edge from last virtual node to successor
            graph_add_edge(rdg->graph,
                           head_index,
                           successor->index,
                           edge->data);
        }

        queue_pop(queue);
        object_delete(successors);
    }

    object_delete(queue);
}


void rdg_remove_virtual_nodes (struct _rdg * rdg)
{
    struct _queue * queue = queue_create();

    // for each node
    struct _graph_it * it;
    for (it = graph_iterator(rdg->graph); it != NULL; it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);

        // if this node is virtual, add it to the queue for removal
        if (rdg_node->flags & RDG_NODE_VIRTUAL)
            queue_push(queue, rdg_node);
    }


    while (queue->size > 0) {
        struct _rdg_node * rdg_node = queue_peek(queue);

        printf("removing virtual node: %llx\n",
               (unsigned long long) rdg_node->index);

        // get predecessor and successor for this node
        struct _graph_node * node = graph_fetch_node(rdg->graph, rdg_node->index);
        uint64_t successor_index = -1;
        uint64_t predecessor_index = -1;
        struct _list_it * eit;
        for (eit = list_iterator(node->edges); eit != NULL; eit = eit->next) {
            struct _graph_edge * edge = eit->data;
            if (edge->head == node->index)
                successor_index = edge->tail;
            else
                predecessor_index = edge->head;
        }

        if ((successor_index == -1) || (predecessor_index == -1)) {
            fprintf(stderr, "-1 edge index in rdg_destroy_virtual_nodes\n");
        }

        // remove this node from the graph (which removes its edges)
        //graph_remove_node(rdg->graph, node->index);

        queue_pop(queue);
    }

    object_delete(queue);
}


void rdg_set_graph_width (struct _rdg * rdg)
{
    struct _index    * index;
    struct _rdg_node * rdg_node;
    struct _map      * level_map;
    int position, level, x;

    rdg->width = 0;

    // find graph and level widths
    for (level = 0; level < rdg->levels->size; level++) {
        level_map = map_fetch(rdg->levels, level);

        x = 0;
        for (position = 0;
             (index = map_fetch(level_map, position)) != NULL;
             position++) {

            rdg_node = graph_fetch_data(rdg->graph, index->index);

            if (rdg_node_width(rdg_node) + rdg_node->x > x)
                x = rdg_node_width(rdg_node) + rdg_node->x;
        }

        //x -= RDG_NODE_X_SPACING;

        if (x > rdg->width)
            rdg->width = x;
    }
}


int rdg_level_top (struct _rdg * rdg, int level)
{
    if (level > rdg->levels->size)
        return -1;

    struct _map * level_map = map_fetch(rdg->levels, level);
    struct _index * index = map_fetch(level_map, 0);
    if (index == NULL)
        return -1;

    struct _rdg_node * rdg_node;
    rdg_node = graph_fetch_data(rdg->graph, index->index);

    return rdg_node->y;
}


void rdg_assign_node_x (struct _rdg * rdg, int level, int position, int x)
{
    // get the node
    struct _map       * level_map = map_fetch(rdg->levels, level);
    struct _index     * index     = map_fetch(level_map, position);
    struct _rdg_node  * rdg_node  = graph_fetch_data(rdg->graph, index->index);

    rdg_node->x = x;

    if (position == 0)
        return;

    // get the left node
    index = map_fetch(level_map, position - 1);
    struct _rdg_node  * left_node = graph_fetch_data(rdg->graph, index->index);

    // do these nodes overlap?
    if (rdg_node->x < left_node->x + rdg_node_width(left_node) + RDG_NODE_X_SPACING) {


        /*
        * ______________
        * |      ___c__|____________
        * |      |  c              |
        * |______|  c              |
        *        |__c______________|
        */

        int center;
        center  = left_node->x + rdg_node_width(left_node) - rdg_node->x;
        center /= 2;
        center += x;

        left_node->x = center - rdg_node_width(left_node) - (RDG_NODE_X_SPACING / 2);
        rdg_node->x  = center + (RDG_NODE_X_SPACING / 2);
    }
    else {
        rdg_node-> x = x;
        return;
    }

    // adjust all other nodes, shifting left if needed
    int i;
    for (i = position - 2; i >= 0; i--) {
        struct _rdg_node * left_node;
        struct _index    * left_index;
        struct _rdg_node * right_node;
        struct _index    * right_index;

        left_index  = map_fetch(level_map, i);
        right_index = map_fetch(level_map, i + 1);
        left_node  = graph_fetch_data(rdg->graph, left_index->index);
        right_node = graph_fetch_data(rdg->graph, right_index->index);

        if (right_node->x < 
            left_node->x + rdg_node_width(left_node) + RDG_NODE_X_SPACING) {
            left_node->x = right_node->x
                            - RDG_NODE_X_SPACING
                            - rdg_node_width(left_node);
        }
        else
            break;
    }

}


void rdg_assign_x (struct _rdg * rdg)
{
    // center every node's x value based on this position of its parents
    int level_i;
    for (level_i = 1; level_i < rdg->levels->size; level_i++) {
        struct _map * level_map = map_fetch(rdg->levels, level_i);

        int position_i;
        for (position_i = 0; position_i < level_map->size; position_i++) {
            struct _index      * index = map_fetch(level_map, position_i);
            struct _graph_node * node;
            struct _rdg_node   * rdg_node;
            node = graph_fetch_node(rdg->graph, index->index);
            rdg_node = node->data;

            int above_sum = 0;
            int above_n   = 0;
            struct _list_it * eit;
            for (eit = list_iterator(node->edges); eit != NULL; eit = eit->next) {
                struct _graph_edge * edge = eit->data;
                struct _rdg_node * neighbor;
                if (edge->head == node->index)
                    neighbor = graph_fetch_data(rdg->graph, edge->tail);
                else
                    neighbor = graph_fetch_data(rdg->graph, edge->head);

                if (neighbor->level < rdg_node->level) {
                    above_sum += rdg_node_center_x(neighbor);
                    above_n++;
                }
            }

            int above_avg = 0;
            if (above_n > 0)
                above_avg = above_sum / above_n;

            // where do we WANT to place this node
            int node_x = above_avg - (rdg_node_width(rdg_node) / 2);

            rdg_assign_node_x(rdg, level_i, position_i, node_x);
        }
    }

    int least_x = 1000000;
    struct _graph_it * it;
    for (it = graph_iterator(rdg->graph); it != NULL; it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);
        if (rdg_node->x < least_x)
            least_x = rdg_node->x;
    }

    for (it = graph_iterator(rdg->graph); it != NULL; it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);
        rdg_node->x -= least_x;
    }
}


// adjusts all rdg_node x values to the least value is 0
void rdg_left_adjust_x (struct _rdg * rdg)
{
    int min_x = 1000000;
    struct _graph_it * it;

    // find min_x
    for (it = graph_iterator(rdg->graph); it != NULL; it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);
        if (rdg_node->x < min_x)
            min_x = rdg_node->x;
    }

    printf("min_x: %d\n", min_x);

    if (min_x == 0)
        return;

    min_x *= -1;

    // adjust nodes
    for (it = graph_iterator(rdg->graph); it != NULL; it = graph_it_next(it)) {
        struct _rdg_node * rdg_node = graph_it_data(it);
        rdg_node->x += min_x;
    }
}


void rdg_assign_y (struct _rdg * rdg)
{
    int level       = 0;
    struct _map      * level_map;
    struct _rdg_node * rdg_node;
    int y = 0;

    rdg->height = 0;

    // for each level
    for (level = 0;
         (level_map = map_fetch(rdg->levels, level)) != NULL;
         level++) {

        // assign current y to all nodes and find tallest node
        struct _index * index;
        size_t position;
        int tallest = 0;
        for (position = 0;
             (index = map_fetch(level_map, position)) != 0;
             position++) {

            rdg_node = graph_fetch_data(rdg->graph, index->index);
            rdg_node->y = y;

            // find tallest node
            if (rdg_node_height(rdg_node) > tallest)
                tallest = rdg_node_height(rdg_node);
        }
        rdg->height = y + tallest;
        y += tallest + RDG_NODE_Y_SPACING;
    }
}


void rdg_assign_initial_position (struct _rdg * rdg)
{
    if (rdg->levels == NULL)
        return;

    // for every node in a level
    // assign it an x value
    double position;
    int level;
    struct _map * level_map;
    // for each level
    for (level = 0;
         (level_map = map_fetch(rdg->levels, level)) != NULL;
         level++) {
        // for each node in this level
        struct _index * index;
        for (position = 0.0;
             (index = map_fetch(level_map, position)) != NULL;
             position += 1.0) {
            struct _rdg_node * rdg_node;
            rdg_node = graph_fetch_data(rdg->graph, index->index);
            // assign it's x
            rdg_node->position = position;
        }
    }
}


inline void rdg_swap_node_positions (struct _rdg * rdg,
                              int level,
                              int left,
                              int right)
{
    struct _rdg_node * rdg_left_node;
    struct _rdg_node * rdg_right_node;
    struct _index * index;
    
    struct _map * level_map = map_fetch(rdg->levels, level);

    // fetch nodes
    index = map_fetch(level_map, left);
    rdg_left_node = graph_fetch_data(rdg->graph, index->index);
    index = map_fetch(level_map, right);
    rdg_right_node = graph_fetch_data(rdg->graph, index->index);

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


// find the left x of this node by calculating the x center of adjacent nodes
double rdg_node_adjacent_center (struct _rdg * rdg, struct _rdg_node * rdg_node)
{
    double sum = 0;
    double n   = 0;

    struct _graph_node * node = graph_fetch_node(rdg->graph, rdg_node->index);
    struct _list_it * it;
    for (it = list_iterator(node->edges); it != NULL; it = it->next) {
        struct _graph_edge * edge = it->data;
        struct _rdg_node * rdg_adjacent;
        if (edge->head == rdg_node->index)
            rdg_adjacent = graph_fetch_data(rdg->graph, edge->tail);
        else
            rdg_adjacent = graph_fetch_data(rdg->graph, edge->head);
        n   += 1.0;
        sum += rdg_adjacent->position;
    }

    if (n == 0)
        return 0;

    return sum / n;
}


void rdg_reduce_edge_crossings (struct _rdg * rdg)
{
    int keep_looping = 32;

    while (keep_looping--) {
        // for every node in the graph
        struct _graph_it * it;
        for (it = graph_iterator(rdg->graph);
             it != NULL;
             it = graph_it_next(it)) {
            struct _rdg_node * rdg_node = graph_it_data(it);

            // set the barycenter
            rdg_node->position = rdg_node_adjacent_center(rdg, rdg_node);
        }
    }

    // sort levels by position. warning: inefficient sorting method
    int level_i;
    for (level_i = 0; level_i < rdg->levels->size; level_i++) {
        struct _map * level_map = map_fetch(rdg->levels, level_i);

        // get first node in this level
        int i;
        for (i = 0; i < level_map->size; i++) {
            struct _index    * index_i = map_fetch(level_map, i);
            struct _rdg_node * node_i;
            node_i = graph_fetch_data(rdg->graph, index_i->index);

            // get next node in this level
            int j;
            for (j = i + 1; j < level_map->size; j++) {
                struct _index * index_j = map_fetch(level_map, j);
                struct _rdg_node * node_j;
                node_j = graph_fetch_data(rdg->graph, index_j->index);

                // if next node's position < first node's position, swap
                if (node_j->position < node_i->position) {
                    // swap positions in level_map
                    // we need to copy these because they will be deleted
                    index_j = object_copy(index_j);
                    index_i = object_copy(index_i);

                    map_remove(level_map, i);
                    map_remove(level_map, j);

                    map_insert(level_map, i, index_j);
                    map_insert(level_map, j, index_i);

                    // delete copies and retrieve new index_i
                    object_delete(index_i);
                    object_delete(index_j);
                    index_i = map_fetch(level_map, i);
                    
                    // reset node_i pointer
                    node_i = node_j;
                }
            }
        }
    }
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


void rdg_draw_edge (struct _rdg * rdg, struct _graph_edge * edge)
{
    struct _rdg_node * rdg_head = graph_fetch_data(rdg->graph, edge->head);
    struct _rdg_node * rdg_tail = graph_fetch_data(rdg->graph, edge->tail);

    cairo_t * ctx = cairo_create(rdg->surface);
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
        x1 = rdg_node_source_x(rdg, rdg_head, rdg_tail);
        y1 = rdg_node_source_y(rdg, rdg_head, rdg_tail);
        x5 = rdg_node_sink_x(rdg, rdg_head, rdg_tail);
        y5 = rdg_node_sink_y(rdg, rdg_head, rdg_tail);

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
        x1 = rdg_node_source_x(rdg, rdg_head, rdg_tail);
        y1 = rdg_node_source_y(rdg, rdg_head, rdg_tail);
        x5 = rdg_node_sink_x(rdg, rdg_head, rdg_tail);
        y5 = rdg_node_sink_y(rdg, rdg_head, rdg_tail);

        x1 += RDG_SURFACE_PADDING;
        y1 += RDG_SURFACE_PADDING;
        x5 += RDG_SURFACE_PADDING;
        y5 += RDG_SURFACE_PADDING;

        x3  = x5 + ((x1 - x5) / 2);
        y3  = y1 - (RDG_NODE_Y_SPACING / 4);
        y3 -= (5 * rdg_head->position);
        x2  = x1;
        y2  = y3;
        x4  = x5;
        y4  = y3;
    }

    
    cairo_curve_to(ctx, x1, y1, x2, y2, x3, y3);
    cairo_move_to(ctx, x3, y3);
    cairo_curve_to(ctx, x3, y3, x4, y4, x5, y5);
    /*
    cairo_move_to(ctx, x1, y1);
    cairo_line_to(ctx, x5, y5);
    */
    cairo_stroke(ctx);

    // this edge ends in a non-virtual node
    if ((rdg_tail->flags & RDG_NODE_VIRTUAL) == 0) {
        int arrow_type = RDG_ARROW_TYPE_FILL;
        if (rdg_tail->level == rdg_head->level)
            rdg_draw_arrow(ctx, x1, y1, x5, y5, arrow_type);
        else
            rdg_draw_arrow(ctx, x4, y4, x5, y5, arrow_type);
    }

    x1 = x2 + x3 + x4;
    y1 = y2 + y3 + y4;

    cairo_destroy(ctx);
}


void rdg_draw (struct _rdg * rdg)
{

    if (rdg->surface != NULL)
        cairo_surface_destroy(rdg->surface);
    rdg->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                   rdg->width + RDG_SURFACE_PADDING * 2,
                                   rdg->height + RDG_SURFACE_PADDING * 2);

    // for each node, draw the node and its edges
    struct _graph_it * graph_it;
    for (graph_it = graph_iterator(rdg->graph);
         graph_it != NULL;
         graph_it = graph_it_next(graph_it)) {
        struct _rdg_node * rdg_node = graph_it_data(graph_it);

        /*
        if (rdg_node->flags & RDG_NODE_VIRTUAL)
            continue;
        */

        // if this node is not virtual, draw it
        if ((rdg_node->flags & RDG_NODE_VIRTUAL) == 0) {
            cairo_t * ctx = cairo_create(rdg->surface);
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
            rdg_draw_edge(rdg, edge);
        }
        object_delete(successors);
    }

    //cairo_surface_write_to_png(rdg->surface, "cairo.png");
}
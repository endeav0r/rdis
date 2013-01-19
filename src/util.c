#include "util.h"

#include "buffer.h"
#include "function.h"
#include "graph.h"
#include "index.h"
#include "instruction.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>


void remove_function_predecessors (struct _graph * graph, struct _map * functions)
{
    struct _queue * queue = queue_create();

    struct _map_it * mit;
    for (mit = map_iterator(functions); mit != NULL; mit = map_it_next(mit)) {
        struct _function * function = map_it_data(mit);
        struct _graph_node * node = graph_fetch_node(graph, function->address);

        if (node == NULL) {
            printf("remove_function_predecessors null node %llx\n",
                   (unsigned long long) function->address);
            continue;
        }

        struct _list * predecessors = graph_node_predecessors(node);
        struct _list_it * lit;
        for (lit = list_iterator(predecessors); lit != NULL; lit = lit->next) {
            struct _graph_edge * edge = lit->data;
            queue_push(queue, edge);
        }
    }

    while (queue->size > 0) {
        struct _graph_edge * edge = queue_peek(queue);
        graph_remove_edge(graph, edge->head, edge->tail);
        queue_pop(queue);
    }

    object_delete(queue);
}


void create_call_graph_list (struct _graph_node * node, struct _list * call_list)
{
    struct _list * ins_list = node->data;
    struct _list_it * it;
    for (it = list_iterator(ins_list); it != NULL; it = it->next) {
        struct _ins * ins = it->data;
        if (ins->flags & INS_CALL) {
            list_append(call_list, ins);
        }
    }
}


struct _graph * create_call_graph (struct _graph * graph, uint64_t indx)
{
    struct _graph_node * node = graph_fetch_node(graph, indx);

    if (node == NULL)
        return NULL;

    struct _graph * call_graph     = graph_create();
    struct _queue * function_queue = queue_create();
    struct _queue * edge_queue     = queue_create();

    struct _index * index = index_create(node->index);
    queue_push(function_queue, index);
    object_delete(index);

    while (function_queue->size > 0) {
        struct _index * index = queue_peek(function_queue);

        if (graph_fetch_node(call_graph, index->index) != NULL) {
            queue_pop(function_queue);
            continue;
        }

        struct _list * call_list = list_create();

        if (graph_fetch_node(graph, index->index) == NULL) {
            printf("didn't find node %llx\n", (unsigned long long) index->index);
        }
        
        graph_bfs_data(graph, index->index, call_list,
             (void (*) (struct _graph_node *, void *)) create_call_graph_list);

        // for every call we are making
        struct _list_it * it;
        for (it = list_iterator(call_list); it != NULL; it = it->next) {
            // if it has a target and the target points to a valid node and
            // we haven't already added that function
            struct _ins * ins = it->data;
            if (    (ins->flags & INS_TARGET_SET)
                 && (graph_fetch_node(graph, ins->target) != NULL)
                 && (graph_fetch_node(call_graph, ins->target) == NULL)) {
                // add the target to the function queue
                struct _index * new_index = index_create(ins->target);
                queue_push(function_queue, new_index);
                object_delete(new_index);
                // and add an edge for later
                struct _ins_edge * ins_edge = ins_edge_create(INS_EDGE_NORMAL);
                struct _graph_edge * edge;
                edge = graph_edge_create(index->index, ins->target, ins_edge);
                queue_push(edge_queue, edge);
                object_delete(edge);
                object_delete(ins_edge);
            }
        }

        // create this node in the call graph
        graph_add_node(call_graph, index->index, call_list);

        object_delete(call_list);

        queue_pop(function_queue);
    }
    object_delete(function_queue);

    // add all the edges
    while (edge_queue->size > 0) {
        struct _graph_edge * edge = queue_peek(edge_queue);

        graph_add_edge(call_graph, edge->head, edge->tail, edge->data);

        queue_pop(edge_queue);
    }

    return call_graph;
}


int is_string (uint8_t * data, size_t data_size)
{
    int result = STRING_ASCII;
    size_t i;

    for (i = 0; i < data_size; i++) {
        if (data[i] & 0x80) {
            result = 0;
            break;
        }
    }

    if (i < 4)
        return 0;
    return result;
}


size_t rdstrcat (char * dst, char * src, size_t size)
{
    size_t len = strlen(dst);
    char * s = src;
    char * d = &(dst[len]);

    while (*s != '\0') {
        if (len >= size)
            break;
        *d = *s;
        d++;
        s++;
        len++;
    }

    if (len < size)
        dst[len] = '\0';
    else
        dst[size - 1] = '\0';

    return len;
}


struct _ins * graph_fetch_ins (struct _graph * graph, uint64_t address)
{
    struct _graph_node * node = graph_fetch_node_max(graph, address);

    if (node != NULL) {
        struct _list_it * it;
        for (it = list_iterator(node->data); it != NULL; it = it->next) {
            struct _ins * ins = it->data;
            if (ins->address == address)
                return ins;
        }
    }

    struct _graph_it * git;
    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _graph_node * node = graph_it_node(git);
        struct _list_it * it;
        for (it = list_iterator(node->data); it != NULL; it = it->next) {
            struct _ins * ins = it->data;
            if (ins->address == address)
                return ins;
        }
    }

    return NULL;
}


int mem_map_byte (struct _map * mem_map, uint64_t address)
{
    struct _buffer * buffer = map_fetch_max(mem_map, address);
    if (buffer == NULL)
        return -1;

    uint64_t base_address = map_fetch_max_key(mem_map, address);
    if (address >= base_address + buffer->size)
        return -1;

    int result = buffer->bytes[address - base_address];
    result &= 0x000000ff;

    return result;
}


int mem_map_set (struct _map * mem_map, uint64_t address, struct _buffer * buf)
{
    struct _buffer * buf2 = map_fetch_max(mem_map, address + buf->size);
    uint64_t key          = map_fetch_max_key(mem_map, address + buf->size);

    if (    (buf2 != NULL)
         && (    ((address <= key) && (address + buf->size > key))
              || (    (address <= key + buf2->size)
                   && (address + buf->size > key + buf2->size))
              || ((address >= key) && (address + buf->size < key + buf2->size)))) {

        // if this section fits inside a previous section, modify in place
        if ((address >= key) && (address + buf->size <= key + buf2->size)) {
            memcpy(&(buf2->bytes[address - key]), buf->bytes, buf->size);
        }

        // if this section comes before a previous section
        else if (address <= key) {
            uint64_t new_size;
            if (address + buf->size < key + buf2->size)
                new_size = key + buf2->size;
            else
                new_size = address + buf->size;
            new_size -= address;

            uint8_t * tmp = malloc(new_size);
            memcpy(&(tmp[key - address]), buf2->bytes, buf2->size);
            memcpy(tmp, buf->bytes, buf->size);

            struct _buffer * new_buffer = buffer_create(tmp, new_size);
            free(tmp);
            map_remove(mem_map, key);
            map_insert(mem_map, address, new_buffer);
            object_delete(new_buffer);
        }

        // if this section overlaps previous section but starts after previous
        // section starts
        else {
            uint64_t new_size = address + buf->size - key;
            uint8_t * tmp = malloc(new_size);
            memcpy(tmp, buf2->bytes, buf2->size);
            memcpy(&(tmp[address - key]), buf->bytes, buf->size);

            struct _buffer * new_buffer = buffer_create(tmp, new_size);
            free(tmp);
            map_remove(mem_map, key);
            map_insert(mem_map, key, new_buffer);
            object_delete(new_buffer);
        }
    }
    else {
        map_insert(mem_map, address, buf);
    }

    return 0;
}


struct _ins * graph_node_ins (struct _graph_node * node, uint64_t address)
{
    if (node == NULL)
        return NULL;

    struct _list_it * lit;
    for (lit = list_iterator(node->data); lit != NULL; lit = lit->next) {
        struct _ins * ins = lit->data;
        if (ins->address == address)
            return ins;
    }

    return NULL;
}



int remove_all_after (struct _graph_node * node, uint64_t index)
{
    // start by removing all instructions after the given instruction in this node
    struct _list_it * lit;
    int ins_found = 0;
    for (lit = list_iterator(node->data); lit != NULL; lit = lit->next) {
        struct _ins * ins = lit->data;
        if (ins->address == index) {
            ins_found = 1;
            while (lit != NULL)
                lit = list_remove(node->data, lit);
            break;
        }
    }

    if (ins_found == 0)
        return -1;

    // now remove all successor nodes in the graph
    // start by adding all of this node's successors to the queue
    struct _queue * queue = queue_create();
    struct _graph * graph = node->graph;

    struct _list * successors = graph_node_successors(node);
    for (lit = list_iterator(successors); lit != NULL; lit = lit->next) {
        struct _graph_edge * edge = lit->data;
        queue_push(queue, graph_fetch_node(graph, edge->tail));
    }
    object_delete(successors);

    // we only add this node if it is empty, IE the instruction we removed was
    // the first instruction for this node
    struct _list * ins_list = node->data;
    if (ins_list->size == 0)
        queue_push(queue, node);

    // now we begin removing nodes
    while (queue->size > 0) {
        struct _graph_node * node = queue_peek(queue);
        // does this node still exist in the graph?
        if (graph_fetch_node(graph, node->index) == NULL) {
            queue_pop(queue);
            continue;
        }

        struct _list * successors = graph_node_successors(node);
        struct _list_it * lit;
        for (lit = list_iterator(successors); lit != NULL; lit = lit->next) {
            struct _graph_edge * edge = lit->data;
            struct _graph_node * sucnode = graph_fetch_node(graph, edge->tail);
            queue_push(queue, sucnode);
        }
        object_delete(successors);

        graph_remove_node(graph, node->index);
        queue_pop(queue);
    }

    object_delete(queue);

    return 0;
}
#include "serialize.h"

#include "buffer.h"
#include "elf32.h"
#include "elf64.h"
#include "graph.h"
#include "index.h"
#include "instruction.h"
#include "label.h"
#include "list.h"
#include "map.h"
#include "pe.h"
#include "rdis.h"
#include "reference.h"
#include "tree.h"

#include <stdio.h>
#include <stdlib.h>

int serialize_error;

json_t * json_uint64_t (uint64_t value)
{
    char tmp[64];
    snprintf(tmp, 64, "%llx", (unsigned long long) value);

    json_t * json = json_string(tmp);

    return json;
}


uint64_t json_uint64_t_value (json_t * json)
{
    if (json_is_string(json) == JSON_FALSE)
        return -1;

    uint64_t n = strtoull(json_string_value(json), NULL, 16);

    return n;
}


int json_is_uint64_t (json_t * json)
{
    return json_is_string(json);
}


void * deserialize (json_t * json)
{
    serialize_error = 0;

    printf("deserialize 0\n");

    if (! json_is_object(json)) {
        serialize_error = SERIALIZE_ERROR;
        return NULL;
    }

    json_t * ot = json_object_get(json, "ot");
    if (! json_is_integer(ot)) {
        serialize_error = SERIALIZE_ERROR;
        return NULL;
    }

    switch (json_integer_value(ot)) {
    case SERIALIZE_NULL             : return NULL;
    case SERIALIZE_BUFFER           : return buffer_deserialize(json);
    case SERIALIZE_ELF32            : return elf32_deserialize(json);
    case SERIALIZE_ELF64            : return elf64_deserialize(json);
    case SERIALIZE_FUNCTION         : return function_deserialize(json);
    case SERIALIZE_GRAPH_EDGE       : return graph_edge_deserialize(json);
    case SERIALIZE_GRAPH_NODE       : return graph_node_deserialize(json);
    case SERIALIZE_GRAPH            : return graph_deserialize(json);
    case SERIALIZE_INDEX            : return index_deserialize(json);
    case SERIALIZE_INSTRUCTION      : return ins_deserialize(json);
    case SERIALIZE_INSTRUCTION_EDGE : return ins_edge_deserialize(json);
    case SERIALIZE_LABEL            : return label_deserialize(json);
    case SERIALIZE_LIST             : return list_deserialize(json);
    case SERIALIZE_MAP_NODE         : return map_node_deserialize(json);
    case SERIALIZE_MAP              : return map_deserialize(json);
    case SERIALIZE_PE               : return pe_deserialize(json);
    case SERIALIZE_RDIS             : return rdis_deserialize(json);
    case SERIALIZE_REFERENCE        : return reference_deserialize(json);
    case SERIALIZE_TREE             : return tree_deserialize(json);
    }

    fprintf(stderr, "unknown json ot: %d\n", (int) json_integer_value(ot));

    printf("SERIALIZE_NULL %d\n", SERIALIZE_NULL);
    printf("SERIALIZE_BUFFER %d\n", SERIALIZE_BUFFER);
    printf("SERIALIZE_GRAPH_EDGE %d\n", SERIALIZE_GRAPH_EDGE);
    printf("SERIALIZE_GRAPH_NODE %d\n", SERIALIZE_GRAPH_NODE);
    printf("SERIALIZE_GRAPH %d\n", SERIALIZE_GRAPH);
    printf("SERIALIZE_INDEX %d\n", SERIALIZE_INDEX);
    printf("SERIALIZE_INSTRUCTION %d\n", SERIALIZE_INSTRUCTION);
    printf("SERIALIZE_INSTRUCTION_EDGE %d\n", SERIALIZE_INSTRUCTION_EDGE);
    printf("SERIALIZE_LABEL %d\n", SERIALIZE_LABEL);
    printf("SERIALIZE_LIST %d\n", SERIALIZE_LIST);
    printf("SERIALIZE_MAP_NODE %d\n", SERIALIZE_MAP_NODE);
    printf("SERIALIZE_MAP %d\n", SERIALIZE_MAP);
    printf("SERIALIZE_PE %d\n", SERIALIZE_PE);
    printf("SERIALIZE_QUEUE %d\n", SERIALIZE_QUEUE);
    printf("SERIALIZE_RDIS %d\n", SERIALIZE_RDIS);
    printf("SERIALIZE_TREE %d\n", SERIALIZE_TREE);
    printf("SERIALIZE_ERROR %d\n", SERIALIZE_ERROR);
    return NULL;
}
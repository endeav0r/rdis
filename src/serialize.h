#ifndef serialize_HEADER
#define serialize_HEADER

#include <inttypes.h>
#include <jansson.h>

extern int serialize_error;

enum {
    SERIALIZE_NULL = 1,
    SERIALIZE_BUFFER,           // done+/
    SERIALIZE_ELF32,
    SERIALIZE_ELF64,
    SERIALIZE_GRAPH_EDGE,       // done+/
    SERIALIZE_GRAPH_NODE,       // done+/
    SERIALIZE_GRAPH,            // done+/
    SERIALIZE_INDEX,            // done+/
    SERIALIZE_INSTRUCTION,      // done+/
    SERIALIZE_INSTRUCTION_EDGE, // done+/
    SERIALIZE_LABEL,            // done+/
    SERIALIZE_LIST,             // done+/
    SERIALIZE_MAP_NODE,         // done+/
    SERIALIZE_MAP,              // done+/
    SERIALIZE_PE,
    SERIALIZE_QUEUE,
    SERIALIZE_RDIS,             // done+/
    SERIALIZE_TREE,             // done+/
    SERIALIZE_ERROR // for general serialize errors
};

json_t * json_uint64_t       (uint64_t value);
uint64_t json_uint64_t_value (json_t * json);
int      json_is_uint64_t    (json_t * json);

void * deserialize (json_t * json);

#endif
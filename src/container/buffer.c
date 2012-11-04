#include "buffer.h"

#include <stdio.h>
#include <string.h>

static const struct _object buffer_object = {
    (void     (*) (void *)) buffer_delete, 
    (void *   (*) (void *)) buffer_copy,
    NULL,
    NULL,
    (json_t * (*) (void *)) buffer_serialize
};


struct _buffer * buffer_create (uint8_t * bytes, size_t size)
{
    struct _buffer * buffer = (struct _buffer *) malloc(sizeof(struct _buffer));

    buffer->object      = &buffer_object;
    buffer->bytes       = (uint8_t *) malloc(size);
    buffer->size        = size;
    buffer->permissions = 0;

    memcpy(buffer->bytes, bytes, size);

    return buffer;
}


void buffer_delete (struct _buffer * buffer)
{
    free(buffer->bytes);
    free(buffer);
}


struct _buffer * buffer_copy (struct _buffer * buffer)
{
    return buffer_create(buffer->bytes, buffer->size);
}


json_t * buffer_serialize (struct _buffer * buffer)
{
    json_t * json = json_object();

    json_object_set(json, "ot",   json_integer(SERIALIZE_BUFFER));

    json_t * bytes = json_array();
    size_t i;
    for (i = 0; i < buffer->size; i++) {
        json_array_append(bytes, json_integer(buffer->bytes[i]));
    }

    json_object_set(json, "bytes", bytes);
    json_object_set(json, "permissions", json_integer(buffer->permissions));

    return json;
}


struct _buffer * buffer_deserialize (json_t * json)
{
    json_t * bytes       = json_object_get(json, "bytes");
    json_t * permissions = json_object_get(json, "permissions");

    // some sanity checking
    if (! json_is_array(bytes)) {
        serialize_error = SERIALIZE_BUFFER;
        return NULL;
    }
    if (! json_is_integer(permissions)) {
        serialize_error = SERIALIZE_BUFFER;
        return NULL;
    }

    struct _buffer * buffer = (struct _buffer *) malloc(sizeof(struct _buffer));
    
    buffer->object      = &buffer_object;
    buffer->size        = json_array_size(bytes);
    buffer->bytes       = (uint8_t *) malloc(buffer->size);
    buffer->permissions = json_integer_value(permissions);

    uint64_t i;
    for (i = 0; i < buffer->size; i++) {
        json_t * byte = json_array_get(bytes, i);
        if (! json_is_integer(byte)) {
            serialize_error = SERIALIZE_BUFFER;
            free(buffer);
            return NULL;
        }
        buffer->bytes[i] = json_integer_value(byte);
    }

    return buffer;
}
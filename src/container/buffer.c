#include "buffer.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>

static const struct _object buffer_object = {
    (void     (*) (void *)) buffer_delete, 
    (void *   (*) (void *)) buffer_copy,
    NULL,
    NULL,
    (json_t * (*) (void *)) buffer_serialize
};


struct _buffer * buffer_create (const uint8_t * bytes, size_t size)
{
    struct _buffer * buffer = (struct _buffer *) malloc(sizeof(struct _buffer));

    buffer->object      = &buffer_object;
    buffer->bytes       = (uint8_t *) malloc(size);
    buffer->size        = size;
    buffer->permissions = 0;

    memcpy(buffer->bytes, bytes, size);

    return buffer;
}


struct _buffer * buffer_create_null (size_t size)
{
    struct _buffer * buffer = (struct _buffer *) malloc(sizeof(struct _buffer));

    buffer->object      = &buffer_object;
    buffer->bytes       = (uint8_t *) malloc(size);
    buffer->size        = size;
    buffer->permissions = 0;

    memset(buffer->bytes, 0, buffer->size);

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

    gchar * base64_string = g_base64_encode(buffer->bytes, buffer->size);
    json_t * bytes = json_string(base64_string);
    g_free(base64_string);

    json_object_set(json, "bytes", bytes);
    json_object_set(json, "permissions", json_integer(buffer->permissions));

    return json;
}


struct _buffer * buffer_deserialize (json_t * json)
{
    json_t * bytes       = json_object_get(json, "bytes");
    json_t * permissions = json_object_get(json, "permissions");

    // some sanity checking
    if (! json_is_string(bytes)) {
        serialize_error = SERIALIZE_BUFFER;
        return NULL;
    }
    if (! json_is_integer(permissions)) {
        serialize_error = SERIALIZE_BUFFER;
        return NULL;
    }

    size_t out_len;
    guchar         * bytes_decoded = g_base64_decode(json_string_value(bytes), &out_len);
    struct _buffer * buffer        = buffer_create(bytes_decoded, out_len);

    buffer->permissions |= json_integer_value(permissions);

    return buffer;
}
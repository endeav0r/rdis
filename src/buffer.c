#include "buffer.h"

#include <stdio.h>
#include <string.h>

static const struct _object buffer_object = {
    (void   (*) (void *)) buffer_delete, 
    (void * (*) (void *)) buffer_copy,
    NULL,
    NULL
};


struct _buffer * buffer_create (uint8_t * bytes, size_t size)
{
    struct _buffer * buffer = (struct _buffer *) malloc(sizeof(struct _buffer));

    buffer->object = &buffer_object;
    buffer->bytes  = (uint8_t *) malloc(size);
    buffer->size   = size;

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
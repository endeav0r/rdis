#ifndef buffer_HEADER
#define buffer_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "object.h"
#include "serialize.h"

#define BUFFER_READ    (1 << 0)
#define BUFFER_WRITE   (1 << 1)
#define BUFFER_EXECUTE (1 << 2)

struct _buffer {
    const struct _object * object;
    uint32_t  permissions;
    uint8_t * bytes;
    size_t    size;
};

struct _buffer * buffer_create      (uint8_t * bytes, size_t size);
void             buffer_delete      (struct _buffer * buffer);
struct _buffer * buffer_copy        (struct _buffer * buffer);
json_t *         buffer_serialize   (struct _buffer * buffer);
struct _buffer * buffer_deserialize (json_t * json);

#endif
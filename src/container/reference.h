#ifndef reference_HEADER
#define reference_HEADER

#include "object.h"

#include <inttypes.h>

enum {
    REFERENCE_UNKNOWN,
    REFERENCE_LOAD,
    REFERENCE_STORE,
    REFERENCE_CONSTANT,
    REFERENCE_CONSTANT_ADDRESSABLE
};

struct _reference {
    const struct _object * object;
    int type;
    uint64_t referencer;
    uint64_t address;
};


struct _reference * reference_create      (int type, uint64_t referencer, uint64_t address);
void                reference_delete      (struct _reference * reference);
struct _reference * reference_copy        (struct _reference * reference);
int                 reference_cmp         (struct _reference * lhs,
                                           struct _reference * rhs);
json_t            * reference_serialize   (struct _reference * reference);
struct _reference * reference_deserialize (json_t * json);

#endif
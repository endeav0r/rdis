#ifndef label_HEADER
#define label_HEADER

#include "object.h"
#include <inttypes.h>

enum {
    LABEL_NONE,
    LABEL_FUNCTION
};

struct _label {
    const struct _object * object;
    char   * text;
    int      type;
    uint64_t address; 
};


struct _label * label_create      (uint64_t address, const char * text, int type);
void            label_delete      (struct _label * label);
struct _label * label_copy        (struct _label * label);
int             label_cmp         (struct _label * lhs, struct _label * rhs);
json_t *        label_serialize   (struct _label * label);
struct _label * label_deserialize (json_t * json);

void            label_set_text (struct _label * label, const char * text);

#endif
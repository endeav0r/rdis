#ifndef object_HEADER
#define object_HEADER

#include <jansson.h>

struct _object {
    void     (* delete)      (void *);
    void *   (* copy)        (void *);
    int      (* cmp)         (void *, void *);
    void     (* merge)       (void *, void *);
    json_t * (* serialize)   (void *);
};

struct _object_header {
    struct _object * object;
};

#define object_delete(XYX) \
    ((struct _object_header *) XYX)->object->delete(XYX)
#define object_copy(XYX) \
    ((struct _object_header *) XYX)->object->copy(XYX)
#define object_cmp(XYX, YXY) \
    (((struct _object_header *) XYX)->object->cmp(XYX, YXY))
#define object_merge(XYX, YXY) \
    (((struct _object_header *) XYX)->object->merge(XYX, YXY))
#define object_serialize(XYX) \
    ((struct _object_header *) XYX)->object->serialize(XYX)

void objects_delete (void * first, ...);

#endif
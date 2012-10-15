#ifndef object_HEADER
#define object_HEADER

struct _object {
    void   (* delete) (void *);
    void * (* copy)   (void *);
    int    (* cmp)    (void *, void *);
    void   (* merge)  (void *, void *);
};

struct _object_header {
    struct _object * object;
};

void   object_delete (void * data);
void * object_copy   (void * data);
int    object_cmp    (void * lhs, void * rhs);
void   object_merge  (void * lhs, void * rhs);

#endif
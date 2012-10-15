#include "object.h"

#include <stdio.h>
#include <stdlib.h>

struct _object_vtable_ptr {
    struct _object * object;
};

#define OBJECT_VTABLE(XX) \
    (((struct _object_vtable_ptr *) XX)->object)

void object_delete (void * data)
{
    struct _object * object = OBJECT_VTABLE(data);
    if (object->delete == NULL) {
        fprintf(stderr, "called delete on object with no delete method\n");
        exit(-1);
    }
    object->delete(data);
}


void * object_copy (void * data)
{
    struct _object * object = OBJECT_VTABLE(data);
    if (object->copy == NULL) {
        fprintf(stderr, "called copy on object with no copy method\n");
        exit(-1);
    }
    return object->copy(data);
}


int object_cmp (void * lhs, void * rhs)
{
    struct _object * object     = OBJECT_VTABLE(lhs);
    struct _object * object_rhs = OBJECT_VTABLE(rhs);
    if (object->cmp == NULL) {
        fprintf(stderr, "called cmp on object with no cmp method\n");
        exit(-1);
    }
    if (object->cmp != object_rhs->cmp) {
        fprintf(stderr, "called cmp different object types\n");
        exit(-1);
    }
    return object->cmp(lhs, rhs);
}


void object_merge (void * lhs, void * rhs)
{
    struct _object * object     = OBJECT_VTABLE(lhs);
    struct _object * object_rhs = OBJECT_VTABLE(rhs);
    if (object->merge == NULL) {
        fprintf(stderr, "called merge on object with no merge method\n");
        exit(-1);
    }
    if (object->merge != object_rhs->merge) {
        fprintf(stderr, "called cmp different object types\n");
        exit(-1);
    }
    object->merge(lhs, rhs);
}
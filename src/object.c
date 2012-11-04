#include "object.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

struct _object_vtable_ptr {
    struct _object * object;
};

#define OBJECT_VTABLE(XX) \
    (((struct _object_vtable_ptr *) XX)->object)


void objects_delete (void * first, ...)
{
    va_list ap;

    object_delete(first);
    va_start(ap, first);

    void * object = va_arg(ap, void *);
    while (object != NULL) {
        object_delete(object);
        object = va_arg(ap, void *);
    }

    va_end(ap);
}
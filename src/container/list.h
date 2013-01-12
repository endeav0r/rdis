#ifndef list_HEADER
#define list_HEADER

#include <stdlib.h>

#include "object.h"
#include "serialize.h"

struct _list_it {
    void * data;
    struct _list_it * next;
    struct _list_it * prev;
};

struct _list {
    const struct _object * object;
    struct _list_it * first;
    struct _list_it * last;
    size_t size;
};

struct _list * list_create      ();
void           list_delete      (struct _list * list);
json_t *       list_serialize   (struct _list * list);
struct _list * list_deserialize (json_t * json);

void              list_append      (struct _list * list, void * data);
void              list_append_list (struct _list * list, struct _list * rhs);
struct _list_it * list_iterator    (struct _list * list);
struct _list    * list_copy        (struct _list * list);

void            * list_first       (struct _list * list);

// returns iterator to next element, or NULL if there is no next element
// don't be fooled by the passing of the list object, this is constant time
struct _list_it * list_remove (struct _list * list, struct _list_it * iterator);

#endif
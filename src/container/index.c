#include "index.h"

#include "serialize.h"
#include <stdlib.h>

static const struct _object index_object = {
    (void     (*) (void *))         index_delete, 
    (void *   (*) (void *))         index_copy,
    (int      (*) (void *, void *)) index_cmp,
    NULL,
    (json_t * (*) (void *))         index_serialize
};



struct _index * index_create (uint64_t index)
{
    struct _index * index_ptr;
    index_ptr = (struct _index *) malloc(sizeof(struct _index));
    index_ptr->object = &index_object;
    index_ptr->index  = index;
    return index_ptr;
}



void index_delete (struct _index * index)
{
    free(index);
}



struct _index * index_copy (struct _index * index)
{
    return index_create(index->index);
}



int index_cmp (struct _index * lhs, struct _index * rhs)
{
    if (lhs->index < rhs->index)
        return -1;
    else if (lhs->index > rhs->index)
        return 1;
    return 0;
}


json_t * index_serialize (struct _index * index)
{
    json_t * json = json_object();

    json_object_set(json, "ot",  json_integer(SERIALIZE_INDEX));
    json_object_set(json, "index", json_uint64_t(index->index));

    return json;
}


struct _index * index_deserialize (json_t * json)
{
    json_t * index = json_object_get(json, "index");
    if (! json_is_uint64_t(index)) {
        serialize_error = SERIALIZE_INDEX;
        return NULL;
    }

    return index_create(json_uint64_t_value(index));
}
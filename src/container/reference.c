#include "reference.h"

#include "serialize.h"
#include <stdlib.h>

static const struct _object reference_object = {
    (void     (*) (void *))         reference_delete, 
    (void *   (*) (void *))         reference_copy,
    (int      (*) (void *, void *)) reference_cmp,
    NULL,
    (json_t * (*) (void *))         reference_serialize
};



struct _reference * reference_create (int type, uint64_t referencer, uint64_t address)
{
    struct _reference * reference;
    reference = (struct _reference *) malloc(sizeof(struct _reference));
    reference->object     = &reference_object;
    reference->type       = type;
    reference->referencer = referencer;
    reference->address    = address;
    return reference;
}


void reference_delete (struct _reference * reference)
{
    free(reference);
}


struct _reference * reference_copy (struct _reference * reference)
{
    return reference_create(reference->type,
                            reference->referencer,
                            reference->address);
}


int reference_cmp (struct _reference * lhs, struct _reference * rhs)
{
    if (lhs->referencer < rhs->referencer)
        return -1;
    else if (lhs->referencer > rhs->referencer)
        return 1;
    return 0;
}


json_t * reference_serialize (struct _reference * reference)
{
    json_t * json = json_object();

    json_object_set(json, "ot",         json_integer(SERIALIZE_REFERENCE));
    json_object_set(json, "type",       json_integer(reference->type));
    json_object_set(json, "referencer", json_uint64_t(reference->referencer));
    json_object_set(json, "address",    json_uint64_t(reference->address));

    return json;
}


struct _reference * reference_deserialize (json_t * json)
{
    json_t * type       = json_object_get(json, "type");
    json_t * referencer = json_object_get(json, "referencer");
    json_t * address    = json_object_get(json, "address");

    if (    (! json_is_integer(type))
         || (! json_is_uint64_t(referencer))
         || (! json_is_uint64_t(address))) {
        serialize_error = SERIALIZE_REFERENCE;
        return NULL;
    }

    return reference_create(json_integer_value(type),
                            json_uint64_t_value(referencer),
                            json_uint64_t_value(address));
}
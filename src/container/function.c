#include "function.h"

#include "serialize.h"
#include <stdlib.h>

static const struct _object function_object = {
    (void     (*) (void *))         function_delete, 
    (void *   (*) (void *))         function_copy,
    (int      (*) (void *, void *)) function_cmp,
    NULL,
    (json_t * (*) (void *))         function_serialize
};


struct _function * function_create (uint64_t address)
{
    struct _function * function;

    function = (struct _function *) malloc(sizeof(struct _function));
    function->object         = &function_object;
    function->crash_and_burn = -1;
    function->address        = address;
    function->flags          = 0;
    function->addr.low       = 0;
    function->addr.high      = 0;
    return function;
}


void function_delete (struct _function * function)
{
    free(function);
}


struct _function * function_copy (struct _function * function)
{
    return function_create(function->address);
}


int function_cmp (struct _function * lhs, struct _function * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    else if (lhs->address > rhs->address)
        return 1;
    return 0;
}


json_t * function_serialize (struct _function * function)
{
    json_t * json = json_object();

    json_object_set(json, "ot",        json_integer(SERIALIZE_FUNCTION));
    json_object_set(json, "address",   json_uint64_t(function->address));
    json_object_set(json, "flags",     json_integer(function->flags));
    json_object_set(json, "addr_low",  json_uint64_t(function->addr.low));
    json_object_set(json, "addr_high", json_uint64_t(function->addr.high));

    return json;
}


struct _function * function_deserialize (json_t * json)
{
    json_t * address   = json_object_get(json, "address");
    json_t * flags     = json_object_get(json, "flags");
    json_t * addr_low  = json_object_get(json, "addr_low");
    json_t * addr_high = json_object_get(json, "addr_high");

    if (    (! json_is_uint64_t(address))
         || (! json_is_integer(flags))
         || (! json_is_integer(addr_low))
         || (! json_is_integer(addr_high))) {
        serialize_error = SERIALIZE_FUNCTION;
        return NULL;
    }

    struct _function * function = function_create(json_uint64_t_value(address));
    function->flags     = json_integer_value(flags);
    function->addr.low  = json_uint64_t_value(addr_low);
    function->addr.high = json_uint64_t_value(addr_high);

    return function;
}
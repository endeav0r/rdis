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

	json_object_set(json, "ot", json_integer(SERIALIZE_FUNCTION));
	json_object_set(json, "address", json_uint64_t(function->address));

	return json;
}


struct _function * function_deserialize (json_t * json)
{
	json_t * address = json_object_get(json, "address");
	if (! json_is_uint64_t(address)) {
		serialize_error = SERIALIZE_FUNCTION;
		return NULL;
	}

	return function_create(json_uint64_t_value(address));
}
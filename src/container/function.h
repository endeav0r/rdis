#ifndef function_HEADER
#define function_HEADER

#include <inttypes.h>

#include "object.h"

#define FUNCTION_REACHABLE 1

struct _function {
	const struct _object * object;
	uint64_t crash_and_burn; // this will cause current rdis stuff to crash until
	                         // all functions are fixed. leave this in place until
							 // beta release;
	uint64_t address;
	int      flags;

	struct {
		uint64_t low;
		uint64_t high;
	} addr;
};


struct _function * function_create      (uint64_t address);
void               function_delete      (struct _function * function);
struct _function * function_copy        (struct _function * function);
int                function_cmp         (struct _function * lhs,
										 struct _function * rhs);
json_t           * function_serialize   (struct _function * function);
struct _function * function_deserialize (json_t * json);

#endif
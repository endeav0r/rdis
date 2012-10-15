#ifndef util_HEADER
#define util_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "object.h"
#include "tree.h"

struct _function {
	const struct _object * object;
	uint64_t address;
	char *   name;
};

struct _memory_segment {
    uint64_t address;
    uint64_t size;
};


struct _byte_table_t {
    uint64_t address;
    uint8_t  byte;
};


struct _byte_table {
    size_t size;
    struct _byte_table_t * table;
};

struct _function * function_create (uint64_t address, char * name);
void			   function_delete (struct _function * function);
struct _function * function_copy   (struct _function * function);
int 			   function_cmp	   (struct _function * lhs,
									struct _function * rhs);

struct _byte_table * byte_table_create  ();
void                 byte_table_destroy (struct _byte_table * byte_table);

#endif
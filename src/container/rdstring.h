#ifndef rdstring_HEADER
#define rdstring_HEADER

#include <stdlib.h>

#include "object.h"

#define RDSTRING_DEFAULT_SIZE 128

struct _rdstring {
	const struct _object * object;
	char * string;
	size_t string_length;
	size_t string_size;
};

struct _rdstring * rdstring_create (const char * string);
void               rdstring_delete (struct _rdstring * rdstring);
struct _rdstring * rdstring_copy   (struct _rdstring * rdstring);
int				   rdstring_cmp    (struct _rdstring * lhs,
									struct _rdstring * rhs);
void			   rdstring_append (struct _rdstring * lhs,
									struct _rdstring * rhs);
void               rdstring_cat    (struct _rdstring * rdstring,
									const char * string);

#endif
#include "rdstring.h"

#include <string.h>

static const struct _object rdstring_object = {
    (void   (*) (void *))         rdstring_delete, 
    (void * (*) (void *))         rdstring_copy,
    (int    (*) (void *, void *)) rdstring_cmp,
    (void   (*) (void *, void *)) rdstring_append
};


struct _rdstring * rdstring_create (const char * string)
{
    struct _rdstring * rdstring;

    rdstring         = malloc(sizeof(struct _rdstring));
    rdstring->object = &rdstring_object;
    rdstring->string_length = strlen(string);
    rdstring->string_size   = rdstring->string_length + 1;
    rdstring->string        = malloc(rdstring->string_size);
    strcpy(rdstring->string, string);

    return rdstring;
}



void rdstring_delete (struct _rdstring * rdstring)
{
    free(rdstring->string);
    free(rdstring);
}



struct _rdstring * rdstring_copy (struct _rdstring * rdstring)
{
    return rdstring_create(rdstring->string);
}



int rdstring_cmp (struct _rdstring * lhs, struct _rdstring * rhs)
{
    return strcmp(lhs->string, rhs->string);
}



void rdstring_append (struct _rdstring * lhs, struct _rdstring * rhs)
{
    char * new_string;

    lhs->string_size = lhs->string_length + rhs->string_size;
    lhs->string_length = lhs->string_length + rhs->string_length;

    new_string = realloc(lhs->string, lhs->string_size);
    lhs->string = new_string;
    strcat(lhs->string, rhs->string);
}



void rdstring_cat (struct _rdstring * rdstring, const char * string)
{
    size_t string_length = strlen(string);
    char * new_string;

    rdstring->string_length += string_length;
    rdstring->string_size   += string_length;

    new_string = realloc(rdstring->string, rdstring->string_size);
    rdstring->string = new_string;
    strcat(rdstring->string, string);
}
#include "util.h"

#include <stdio.h>
#include <string.h>

static const struct _object function_object = {
    (void   (*) (void *))         function_delete, 
    (void * (*) (void *))         function_copy,
    (int    (*) (void *, void *)) function_cmp,
    NULL
};



struct _function * function_create (uint64_t address, char * name)
{
    struct _function * function;

    function = (struct _function *) malloc(sizeof(struct _function));
    function->object  = &function_object;
    function->address = address;
    function->name    = strdup(name);

    return function;
}



void function_delete (struct _function * function)
{
    free(function->name);
    free(function);
}



struct _function * function_copy (struct _function * function)
{
    return function_create(function->address, function->name);
}



int function_cmp (struct _function * lhs, struct _function * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    if (lhs->address > rhs->address)
        return 1;
    return 0;
}



struct _byte_table * byte_table_create (size_t size)
{
    struct _byte_table *   byte_table;
    struct _byte_table_t * byte_table_t;

    byte_table = (struct _byte_table *) malloc(sizeof(struct _byte_table));
    byte_table->size = size;

    byte_table_t = (struct _byte_table_t *)
                   malloc(sizeof(struct _byte_table_t) * size);

    byte_table->table = byte_table_t;

    return byte_table;
}


void byte_table_destroy (struct _byte_table * byte_table)
{
    free(byte_table->table);
    free(byte_table);
}
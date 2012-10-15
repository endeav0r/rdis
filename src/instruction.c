#include "instruction.h"

#include <string.h>

static const struct _object ins_object = {
    (void   (*) (void *))         ins_delete, 
    (void * (*) (void *))         ins_copy,
    (int    (*) (void *, void *)) ins_cmp,
    NULL
};

static const struct _object ins_edge_object = {
    (void   (*) (void *))         ins_edge_delete, 
    (void * (*) (void *))         ins_edge_copy,
    NULL,
    NULL
};

struct _ins * ins_create  (uint64_t address,
                           uint8_t * bytes,
                           size_t size,
                           char * description,
                           char * comment)
{
    struct _ins * ins;

    ins = (struct _ins *) malloc(sizeof(struct _ins));

    ins->object = &ins_object;

    ins->address = address;

    ins->bytes = malloc(size);
    memcpy(ins->bytes, bytes, size);

    ins->size = size;

    if (description == NULL)
        ins->description = NULL;
    else
        ins->description = strdup(description);

    if (comment == NULL)
        ins->comment = NULL;
    else
        ins->comment = strdup(comment);

    ins->flags = 0;

    return ins;
}


void ins_delete (struct _ins * ins)
{
    free(ins->bytes);
    if (ins->description != NULL)
        free(ins->description);
    if (ins->comment != NULL)
        free(ins->comment);
    free(ins);
}


struct _ins * ins_copy (struct _ins * ins)
{
    struct _ins * new_ins = ins_create(ins->address,
                                       ins->bytes,
                                       ins->size,
                                       ins->description,
                                       ins->comment);
    new_ins->target = ins->target;
    new_ins->flags  = ins->flags;

    return new_ins;
}



void ins_s_description (struct _ins * ins, const char * description)
{
    if (ins->description != NULL)
        free(ins->description);

    if (description == NULL)
        ins->description = NULL;
    else
        ins->description = strdup(description);
}


void ins_s_comment (struct _ins * ins, const char * comment)
{
    if (ins->comment != NULL)
        free(ins->comment);

    if (comment == NULL)
        ins->comment = NULL;
    else
        ins->comment = strdup(comment);
}


void ins_s_target (struct _ins * ins, uint64_t target)
{
    ins->flags |= INS_FLAG_TARGET_SET;
    ins->target = target;
}


int ins_cmp (struct _ins * lhs, struct _ins * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    else if (lhs->address > rhs->address)
        return 1;
    return 0;
}


struct _ins_edge * ins_edge_create (int type)
{
    struct _ins_edge * ins_edge;
    ins_edge = (struct _ins_edge *) malloc(sizeof(struct _ins_edge));

    ins_edge->object = &ins_edge_object;
    ins_edge->type = type;

    return ins_edge;
}


void ins_edge_delete (struct _ins_edge * ins_edge)
{
    free(ins_edge);
}


struct _ins_edge * ins_edge_copy (struct _ins_edge * ins_edge)
{
    return ins_edge_create(ins_edge->type);
}
#include "instruction.h"

#include <string.h>

static const struct _object ins_object = {
    (void   (*) (void *))         ins_delete, 
    (void * (*) (void *))         ins_copy,
    (int    (*) (void *, void *)) ins_cmp,
    NULL,
    (json_t * (*) (void *))       ins_serialize
};

static const struct _object ins_edge_object = {
    (void   (*) (void *))         ins_edge_delete, 
    (void * (*) (void *))         ins_edge_copy,
    NULL,
    NULL,
    (json_t * (*) (void *))       ins_edge_serialize
};

struct _ins * ins_create  (uint64_t address,
                           uint8_t * bytes,
                           size_t size,
                           const char * description,
                           const char * comment)
{
    struct _ins * ins;

    ins = (struct _ins *) malloc(sizeof(struct _ins));

    ins->object  = &ins_object;
    ins->address = address;
    ins->target  = -1;

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

    ins->references = list_create();

    return ins;
}


void ins_delete (struct _ins * ins)
{
    free(ins->bytes);
    if (ins->description != NULL)
        free(ins->description);
    if (ins->comment != NULL)
        free(ins->comment);
    if (ins->references != NULL)
        object_delete(ins->references);
    free(ins);
}


struct _ins * ins_copy (struct _ins * ins)
{
    struct _ins * new_ins = ins_create(ins->address,
                                       ins->bytes,
                                       ins->size,
                                       ins->description,
                                       ins->comment);
    new_ins->target     = ins->target;
    new_ins->flags      = ins->flags;
    new_ins->references = object_copy(ins->references);

    return new_ins;
}


int ins_cmp (struct _ins * lhs, struct _ins * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    else if (lhs->address > rhs->address)
        return 1;
    return 0;
}


json_t * ins_serialize (struct _ins * ins)
{
    json_t * json = json_object();
    json_t * bytes = json_array();

    int i;
    for (i = 0; i < ins->size; i++) {
        json_array_append(bytes, json_integer(ins->bytes[i]));
    }

    json_object_set(json, "ot",          json_integer(SERIALIZE_INSTRUCTION));
    json_object_set(json, "address",     json_uint64_t(ins->address));
    json_object_set(json, "target",      json_uint64_t(ins->target));
    json_object_set(json, "bytes",       bytes);
    if (ins->description == NULL)
        json_object_set(json, "description", json_string(""));
    else
        json_object_set(json, "description", json_string(ins->description));
    if (ins->comment == NULL)
        json_object_set(json, "comment", json_string(""));
    else
        json_object_set(json, "comment", json_string(ins->comment));
    json_object_set(json, "flags",      json_integer(ins->flags));
    json_object_set(json, "references", object_serialize(ins->references));

    return json;
}


struct _ins * ins_deserialize (json_t * json)
{
    json_t * address     = json_object_get(json, "address");
    json_t * target      = json_object_get(json, "target");
    json_t * bytes       = json_object_get(json, "bytes");
    json_t * description = json_object_get(json, "description");
    json_t * comment     = json_object_get(json, "comment");
    json_t * flags       = json_object_get(json, "flags");
    json_t * references  = json_object_get(json, "references");

    if (    (! json_is_uint64_t(address))
         || (! json_is_uint64_t(target))
         || (! json_is_array(bytes))
         || (! json_is_string(description))
         || (! json_is_string(comment))
         || (! json_is_integer(flags))
         || (! json_is_object(references))) {
        serialize_error = SERIALIZE_INSTRUCTION;
        return NULL;
    }

    uint8_t * tmp = (uint8_t *) malloc(json_array_size(bytes));
    int i;
    for (i = 0; i < json_array_size(bytes); i++) {
        tmp[i] = json_integer_value(json_array_get(bytes, i));
    }

    const char * sdescription = json_string_value(description);
    const char * scomment     = json_string_value(comment);

    if (strlen(sdescription) == 0)
        sdescription = NULL;
    if (strlen(scomment) == 0)
        scomment = NULL;

    struct _ins * ins = ins_create(json_uint64_t_value(address),
                                   tmp,
                                   json_array_size(bytes),
                                   sdescription,
                                   scomment);

    ins->target = json_uint64_t_value(target);
    ins->flags  = json_integer_value(flags);

    ins->references = deserialize(references);
    if (ins->references == NULL) {
        object_delete(ins);
        return NULL;
    }

    free(tmp);

    return ins;
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

    if ((comment == NULL) || (strlen(comment) == 0))
        ins->comment = NULL;
    else
        ins->comment = strdup(comment);
}


void ins_s_target (struct _ins * ins, uint64_t target)
{
    ins->flags |= INS_TARGET_SET;
    ins->target = target;
}


void ins_s_call (struct _ins * ins)
{
    ins->flags |= INS_CALL;
}


void ins_add_reference (struct _ins * ins, struct _reference * reference)
{
    list_append(ins->references, reference);
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


json_t * ins_edge_serialize (struct _ins_edge * ins_edge)
{
    json_t * json = json_object();

    json_object_set(json, "ot",   json_integer(SERIALIZE_INSTRUCTION_EDGE));
    json_object_set(json, "type", json_integer(ins_edge->type));

    return json;
}


struct _ins_edge * ins_edge_deserialize (json_t * json)
{
    json_t * type = json_object_get(json, "type");

    if (! json_is_integer(type)) {
        serialize_error = SERIALIZE_INSTRUCTION_EDGE;
        return NULL;
    }

    return ins_edge_create(json_integer_value(type));
}
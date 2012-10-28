#include "label.h"

#include "serialize.h"

#include <stdlib.h>
#include <string.h>

static const struct _object label_object = {
    (void   (*) (void *))         label_delete, 
    (void * (*) (void *))         label_copy,
    (int    (*) (void *, void *)) label_cmp,
    NULL,
    (json_t * (*) (void *))       label_serialize
};


struct _label * label_create (uint64_t address, const char * text, int type)
{
    struct _label * label;

    label = (struct _label *) malloc(sizeof(struct _label));
    label->object = &label_object;
    label->address = address;
    label->type    = type;

    if (text == NULL)
        label->text = NULL;
    else
        label->text = strdup(text);

    return label;
}


void label_delete (struct _label * label)
{
    if (label->text != NULL)
        free(label->text);
    free(label);
}


struct _label * label_copy (struct _label * label)
{
    return label_create(label->address, label->text, label->type);
}


int label_cmp (struct _label * lhs, struct _label * rhs)
{
    if (lhs->address < rhs->address)
        return -1;
    else if (lhs->address > rhs->address)
        return 1;
    return 0;
}


json_t * label_serialize (struct _label * label)
{
    json_t * json = json_object();

    json_object_set(json, "ot",      json_integer(SERIALIZE_LABEL));
    json_object_set(json, "text",    json_string(label->text));
    json_object_set(json, "type",    json_integer(label->type));
    json_object_set(json, "address", json_uint64_t(label->address));

    return json;
}


void label_set_text (struct _label * label, const char * text)
{
    if (label->text != NULL)
        free(label->text);
    label->text = strdup(text);
}


struct _label * label_deserialize (json_t * json)
{
    json_t * address = json_object_get(json, "address");
    json_t * text    = json_object_get(json, "text");
    json_t * type    = json_object_get(json, "type");

    if (! json_is_uint64_t(address))
        return NULL;
    if (! json_is_string(text))
        return NULL;
    if (! json_is_integer(type))
        return NULL;

    const char * stext = json_string_value(text);
    if (strlen(stext) == 0)
        stext = NULL;

    return label_create(json_uint64_t_value(address),
                        stext,
                        json_integer_value(type));
}
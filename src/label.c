#include "label.h"

#include <stdlib.h>
#include <string.h>

static const struct _object label_object = {
    (void   (*) (void *))         label_delete, 
    (void * (*) (void *))         label_copy,
    (int    (*) (void *, void *)) label_cmp,
    NULL
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


void label_set_text (struct _label * label, const char * text)
{
    if (label->text != NULL)
        free(label->text);
    label->text = strdup(text);
}
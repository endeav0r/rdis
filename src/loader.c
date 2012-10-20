#include "loader.h"

#include "elf64.h"
#include "elf32.h"


_loader * loader_create (const char * filename)
{
    _loader * loader;

    loader = (_loader *) elf64_create(filename);
    if (loader != NULL)
        return loader;

    loader = (_loader *) elf32_create(filename);
    if (loader != NULL)
        return loader;

    return NULL;
}


uint64_t loader_entry (_loader * loader)
{
    struct _loader_object_ptr * loader_object_ptr;
    struct _loader_object     * loader_object;

    loader_object_ptr = (struct _loader_object_ptr *) loader;
    loader_object = loader_object_ptr->loader_object;

    return loader_object->entry(loader);
}


struct _graph * loader_graph (_loader * loader)
{
    struct _loader_object_ptr * loader_object_ptr;
    struct _loader_object     * loader_object;

    loader_object_ptr = (struct _loader_object_ptr *) loader;
    loader_object = loader_object_ptr->loader_object;

    return loader_object->graph(loader);
}


struct _tree * loader_function_tree (_loader * loader)
{
    struct _loader_object_ptr * loader_object_ptr;
    struct _loader_object     * loader_object;

    loader_object_ptr = (struct _loader_object_ptr *) loader;
    loader_object = loader_object_ptr->loader_object;

    return loader_object->function_tree(loader);
}


struct _map * loader_labels (_loader * loader)
{
    struct _loader_object_ptr * loader_object_ptr;
    struct _loader_object     * loader_object;

    loader_object_ptr = (struct _loader_object_ptr *) loader;
    loader_object = loader_object_ptr->loader_object;

    return loader_object->labels(loader);
}
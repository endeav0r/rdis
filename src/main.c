#include "elf64.h"

#include "buffer.h"
#include "graph.h"
#include "instruction.h"
#include "list.h"
#include "rdg.h"
#include "rdstring.h"
#include "util.h"

#include <cairo.h>
#include <fontconfig/fontconfig.h>

#include <stdio.h>
#include <string.h>


int main (int argc, char * argv[])
{
    _loader * loader;

    loader = loader_create(argv[1]);
    if (loader == NULL) {
        fprintf(stderr, "failed to load file %s\n", argv[1]);
        return -1;
    }

    struct _map * memmap = loader_memory_map(loader);

    struct _map_it * it;
    for (it = map_iterator(memmap); it != NULL; it = map_it_next(it)) {
        struct _buffer * buffer = map_it_data(it);
        uint64_t         addr   = map_it_key(it);
        printf("%llx -> %llx\n",
               (unsigned long long) addr,
               (unsigned long long) addr + buffer->size);
    }

    object_delete(memmap);

    object_delete(loader);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
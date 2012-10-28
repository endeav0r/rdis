#include "elf64.h"

#include "buffer.h"
#include "graph.h"
#include "instruction.h"
#include "list.h"
#include "rdg.h"
#include "rdis.h"
#include "rdstring.h"
#include "serialize.h"
#include "util.h"

#include <cairo.h>
#include <fontconfig/fontconfig.h>

#include <stdio.h>
#include <string.h>


int main (int argc, char * argv[])
{
    /*
    _loader * loader;

    loader = loader_create(argv[1]);
    if (loader == NULL) {
        fprintf(stderr, "failed to load file %s\n", argv[1]);
        return -1;
    }

    struct _rdis * rdis = rdis_create(loader);

    json_t * json = object_serialize(rdis);
    json_dump_file(json, "rdis.json", JSON_INDENT(2));
    json_decref(json);

    object_delete(rdis);
    */
    json_t * json;
    struct _rdis * rdis;

    // prepare for boom

    json_error_t json_error;
    json = json_load_file("rdis.json", 0, &json_error);
    if (! json) {
        printf("%s\n", json_error.text);
        return -1;
    }
    rdis = deserialize(json);

    if (rdis == NULL) {
        printf("serialize_error: %d\n", serialize_error);
    }
    else
        printf("serialization success\n");
    object_delete(rdis);
    json_decref(json);

    cairo_debug_reset_static_data();
    FcFini();

    return 0;
}
#ifndef instruction_HEADER
#define instruction_HEADER

#include <inttypes.h>
#include <stdlib.h>

#include "list.h"
#include "object.h"
#include "reference.h"
#include "serialize.h"

#define INS_TARGET_SET 1
#define INS_CALL       2

/*
*  Changes to instructions should maybe pay attention to rdis_regraph_function
*  which does funky things with instructions...
*/

enum {
    INS_EDGE_NORMAL,
    INS_EDGE_JUMP,
    INS_EDGE_JCC_TRUE,
    INS_EDGE_JCC_FALSE
};

struct _ins {
    const struct _object * object;
    uint64_t       address;
    uint64_t       target; // -1 == not set
    uint8_t *      bytes;
    size_t         size;
    char *         description;
    char *         comment;
    unsigned int   flags;
    struct _list * references;
};


struct _ins_edge {
    const struct _object * object;
    int type;
};


struct _ins * ins_create    (uint64_t address,
                             const uint8_t * bytes,
                             size_t size,
                             const char * description,
                             const char * comment);

void          ins_delete      (struct _ins * ins);
struct _ins * ins_copy        (struct _ins * ins);
int           ins_cmp         (struct _ins * lhs, struct _ins * rhs);
json_t *      ins_serialize   (struct _ins * ins);
struct _ins * ins_deserialize (json_t * json);

void          ins_s_comment     (struct _ins * ins, const char * comment);
void          ins_s_description (struct _ins * ins, const char * description);
void          ins_s_target      (struct _ins * ins, uint64_t target);
void          ins_s_call        (struct _ins * ins);
void          ins_add_reference (struct _ins * ins, struct _reference * reference);


struct _ins_edge * ins_edge_create      (int type);
void               ins_edge_delete      (struct _ins_edge * ins_edge);
struct _ins_edge * ins_edge_copy        (struct _ins_edge * ins_edge);
json_t *           ins_edge_serialize   (struct _ins_edge * ins_edge);
struct _ins_edge * ins_edge_deserialize (json_t * json);

#endif

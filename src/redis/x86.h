#ifndef redis_x86_HEADER
#define redis_x86_HEADER

#include "ins.h"
#include "map.h"
#include "object.h"

enum {
    REDIS_SUCCESS = 0,
    REDIS_INVALID_IP,
    REDIS_INVALID_INSTRUCTION
};

enum {
    RED_EAX,
    RED_EBX,
    RED_ECX,
    RED_EDX,
    RED_ESP,
    RED_EBP,
    RED_ESI,
    RED_EDI,
    RED_EIP,
    RED_REGS_NUM
};

struct _redis_x86 {
    const struct _object * object;
    struct _map * mem;
    uint32_t      regs[RED_REGS_NUM];
    int           error;
};

struct _redis_x86 * redis_x86_create ();
void                redis_x86_delete (struct _redis_x86 * redis_x86);
struct _redis_x86 * redis_x86_copy   (struct _redis_x86 * redis_x86);

void                redis_x86_mem_from_mem_map (struct _redis_x86 * redis_x86,
                                                struct _map * mem_map);

int redis_x86_step (struct _redis_x86 * redis_x86);

void redis_x86_debug_step (struct _redis_x86 * redis_x86);

#endif
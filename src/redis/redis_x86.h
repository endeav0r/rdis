#ifndef redis_x86_HEADER
#define redis_x86_HEADER

#include "instruction.h"
#include "map.h"
#include "object.h"

#include <udis86.h>

#define REDIS_X86_FALSE_STACK_SIZE (1024 * 32)
#define REDIS_X86_FPU_STACK_SIZE   8

enum {
    REDIS_SUCCESS = 0,
    REDIS_INVALID_IP,
    REDIS_INVALID_INSTRUCTION,
    REDIS_EXEC_ERROR,
    REDIS_FPU_STACK_EXHAUSTION
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

    uint32_t      last_fpu_ins; // for getpc through fld2env (or whatever it is)
};

struct _redis_x86 * redis_x86_create ();
void                redis_x86_delete (struct _redis_x86 * redis_x86);
struct _redis_x86 * redis_x86_copy   (struct _redis_x86 * redis_x86);

void  redis_x86_mem_from_mem_map (struct _redis_x86 * redis_x86,
                                  struct _map * mem_map);
void  redis_x86_false_stack (struct _redis_x86 * redis_x86);              

int  redis_x86_step (struct _redis_x86 * redis_x86);

uint32_t redis_x86_reg_value         (struct _redis_x86 * redis_x86,
                                      enum ud_type reg);
uint32_t redix_x86_sib               (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand);
uint32_t redis_x86_get_operand_value (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand);
int      redis_x86_set_operand_value (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand,
                                      uint32_t value);

int redis_x86_fld2lt (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_mov    (struct _redis_x86 * redis_x86, ud_t * ud_obj);

#endif
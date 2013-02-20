#ifndef redis_x86_HEADER
#define redis_x86_HEADER

#include "instruction.h"
#include "map.h"
#include "object.h"

#include <udis86.h>

#define REDIS_X86_FALSE_STACK_ADDR 0xfff80000
#define REDIS_X86_FALSE_STACK_SIZE (1024 * 32)
#define REDIS_X86_FPU_STACK_SIZE   8
#define REDIS_X86_ERROR_MSG_SIZE   256

enum {
    REDIS_SUCCESS = 0,
    REDIS_INVALID_IP,
    REDIS_INVALID_INSTRUCTION,
    REDIS_EXEC_ERROR,
    REDIS_MEM_ERROR
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
    RED_CF,
    RED_OF,
    RED_SF,
    RED_ZF,
    RED_REGS_NUM
};

struct _redis_x86 {
    const struct _object * object;
    struct _map * mem;
    uint32_t      regs[RED_REGS_NUM];
    int           error;

    uint32_t      last_fpu_ins;

    uint64_t  ins_addr;
    size_t    ins_size;
    uint8_t   ins_bytes[16];
    char      error_msg[REDIS_X86_ERROR_MSG_SIZE];
};

struct _redis_x86 * redis_x86_create ();
void                redis_x86_delete (struct _redis_x86 * redis_x86);
struct _redis_x86 * redis_x86_copy   (struct _redis_x86 * redis_x86);

void  redis_x86_mem_from_mem_map (struct _redis_x86 * redis_x86,
                                  struct _map * mem_map);
void  redis_x86_false_stack (struct _redis_x86 * redis_x86);              

int   redis_x86_step (struct _redis_x86 * redis_x86);

int      redis_x86_mem_set32         (struct _redis_x86 * redis_x86,
                                      uint32_t addr,
                                      uint32_t value);
int      redis_x86_mem_set8          (struct _redis_x86 * redis_x86,
                                      uint32_t addr,
                                      uint8_t value);
uint8_t  redis_x86_mem_get8          (struct _redis_x86 * redis_x86,
                                      uint32_t addr,
                                      int * error);
uint16_t redis_x86_mem_get16         (struct _redis_x86 * redis_x86,
                                      uint32_t addr,
                                      int * error);
uint32_t redis_x86_mem_get32         (struct _redis_x86 * redis_x86,
                                      uint32_t addr,
                                      int * error);
uint32_t redis_x86_reg_value         (struct _redis_x86 * redis_x86,
                                      enum ud_type reg,
                                      int * error);
uint32_t redis_lval_signed           (struct _redis_x86 * redis_x86,
                                      struct ud_operand * operand,
                                      int * error);
uint32_t redix_x86_sib               (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand);
uint32_t redis_x86_get_operand_value (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand,
                                      int * error);
int      redis_x86_set_operand       (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand,
                                      uint32_t value);

int redis_x86_add     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_and     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_cdq     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_fldl2t  (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_fnstenv (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_inc     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_int     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_jl      (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_jmp     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_loop    (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_mov     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_mul     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_pop     (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_push    (struct _redis_x86 * redis_x86, ud_t * ud_obj);
int redis_x86_xor     (struct _redis_x86 * redis_x86, ud_t * ud_obj);

#endif
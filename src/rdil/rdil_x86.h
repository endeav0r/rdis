#ifndef rdil_x86_HEADER
#define rdil_x86_HEADER

#include "instruction.h"
#include "list.h"
#include "rdil.h"

#include <udis86.h>

enum {
    RDIL_AL,
    RDIL_AH,
    RDIL_AX,
    RDIL_EAX,

    RDIL_BL,
    RDIL_BH,
    RDIL_BX,
    RDIL_EBX,

    RDIL_CL,
    RDIL_CH,
    RDIL_CX,
    RDIL_ECX,

    RDIL_DL,
    RDIL_DH,
    RDIL_DX,
    RDIL_EDX,

    RDIL_ESI,
    RDIL_EDI,
    RDIL_ESP,
    RDIL_EBP,

    RDIL_FS,

    RDIL_OF,
    RDIL_CF,
    RDIL_ZF,
    RDIL_SF
};

struct _list * rdil_x86 (struct _ins * ins);

char * rdil_x86_ins_text (struct _rdil_ins * rdil_ins);

int      rdil_x86_full_reg             (int reg);
int      rdil_x86_udis_reg_to_rdil_reg (int reg);
int      rdil_x86_reg_bits             (int reg);
uint64_t rdil_x86_operand_lval         (int bits, struct ud_operand operand);

struct _rdil_operand * rdil_x86_operand (ud_t *   ud_obj,
                                         int      operand_i,
                                         uint64_t address,
                                         struct _list * list);

struct _rdil_operand * rdil_x86_operand_load (ud_t *   ud_obj,
                                              int      operand_i,
                                              uint64_t address,
                                              int      bits,
                                              struct _list * list);

struct _rdil_operand * rdil_x86_operand_get (ud_t * ud_obj,
                                             int operand_i,
                                             uint64_t address,
                                             struct _list * list);

void rdil_x86_operand_set (ud_t *   ud_obj,
                           int      operand_i,
                           uint64_t address,
                           struct _rdil_operand * value,
                           struct _list *         list);

struct _list * rdil_x86_add  (ud_t * ud_obj, uint64_t address);
struct _list * rdil_x86_mov  (ud_t * ud_obj, uint64_t address);
struct _list * rdil_x86_push (ud_t * ud_obj, uint64_t address);

#endif
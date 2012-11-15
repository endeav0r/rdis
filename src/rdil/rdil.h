#ifndef rdil_HEADER
#define rdil_HEADER

#include "object.h"

#include <inttypes.h>

enum {
    RDIL_VAR,
    RDIL_CONSTANT,
    RDIL_WILD
};

enum {
    RDIL_NONE,
    RDIL_SYSCALL,
    RDIL_LOAD,
    RDIL_STORE,
    RDIL_BRC,
    RDIL_ASSIGN,
    RDIL_NOT,
    RDIL_SEXT,
    RDIL_HLT,
    RDIL_ADD,
    RDIL_SUB,
    RDIL_MUL,
    RDIL_DIV,
    RDIL_MOD,
    RDIL_SHL,
    RDIL_SHR,
    RDIL_AND,
    RDIL_OR,
    RDIL_XOR,
    RDIL_CMPEQ,
    RDIL_CMPLTU,
    RDIL_CMPLTS,
    RDIL_CMPLEU,
    RDIL_CMPLES
};

struct _rdil_operand {
    const struct _object * object;
    int      type;
    int      bits;
    uint64_t value;
};

struct _rdil_ins {
    const struct _object * object;
    int      type;
    uint64_t address;

    struct _rdil_operand * dst;
    
    union {
        struct _rdil_operand * src;
        struct _rdil_operand * cond;
        struct _rdil_operand * lhs;
    };
    
    union {
        struct _rdil_operand * rhs;
        int bits;
    };
};

struct _rdil_operand * rdil_operand_create (int type, int bits, uint64_t value);
void                   rdil_operand_delete (struct _rdil_operand * operand);
struct _rdil_operand * rdil_operand_copy   (struct _rdil_operand * operand);

struct _rdil_ins * rdil_ins_create (int type, uint64_t address, ...);
void               rdil_ins_delete (struct _rdil_ins * rdil_ins);
struct _rdil_ins * rdil_ins_copy   (struct _rdil_ins * rdil_ins);

const char * rdil_ins_mnemonic (struct _rdil_ins * rdil_ins);


#endif
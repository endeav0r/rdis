#include "rdil.h"

#include <stdarg.h>
#include <stdlib.h>


const char * rdil_mnemonics [] = {
    "NONE",
    "SYSCALL",
    "LOAD",
    "STORE",
    "BRC",
    "ASSIGN",
    "NOT",
    "SEXT",
    "HLT",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "MOD",
    "SHL",
    "SHR",
    "AND",
    "OR",
    "XOR",
    "CMPEQ",
    "CMPLTU",
    "CMPLTS",
    "CMPLEU",
    "CMPLES"
};


static const struct _object rdil_operand_object = {
    (void   (*) (void *)) rdil_operand_delete,
    (void * (*) (void *)) rdil_operand_copy,
    NULL,
    NULL,
    NULL
};

static const struct _object rdil_ins_object = {
    (void   (*) (void *)) rdil_ins_delete,
    (void * (*) (void *)) rdil_ins_copy,
    NULL,
    NULL,
    NULL
};



struct _rdil_operand * rdil_operand_create (int type, int bits, uint64_t value)
{
    struct _rdil_operand * operand;
    operand = (struct _rdil_operand *) malloc(sizeof(struct _rdil_operand));

    operand->object = &rdil_operand_object;
    operand->type   = type;
    operand->bits   = bits;
    operand->value  = value;

    return operand;
}


void rdil_operand_delete (struct _rdil_operand * operand)
{
    free(operand);
}


struct _rdil_operand * rdil_operand_copy (struct _rdil_operand * operand)
{
    return rdil_operand_create(operand->type, operand->bits, operand->value);
}


struct _rdil_ins * rdil_ins_create (int type, uint64_t address, ...)
{
    va_list ap;
    struct _rdil_ins * ins;
    ins = (struct _rdil_ins *) malloc(sizeof(struct _rdil_ins));

    // find the format of the struct we are going to use
    switch (type) {
    case RDIL_SYSCALL :
    case RDIL_HLT     :
        break;

    case RDIL_LOAD  :
    case RDIL_STORE :
        va_start(ap, address);
        ins->dst  = object_copy(va_arg(ap, struct _rdil_operand *));
        ins->src  = object_copy(va_arg(ap, struct _rdil_operand *));
        ins->bits = va_arg(ap, int);
        va_end(ap);
        break;

    case RDIL_BRC :
        va_start(ap, address);
        ins->dst  = object_copy(va_arg(ap, struct _rdil_operand *));
        ins->cond = object_copy(va_arg(ap, struct _rdil_operand *));
        va_end(ap);
        break;

    case RDIL_ASSIGN :
    case RDIL_NOT    :
    case RDIL_SEXT   :
        va_start(ap, address);
        ins->dst = object_copy(va_arg(ap, struct _rdil_operand *));
        ins->src = object_copy(va_arg(ap, struct _rdil_operand *));
        va_end(ap);
        break;

    case RDIL_ADD    :
    case RDIL_SUB    :
    case RDIL_MUL    :
    case RDIL_DIV    :
    case RDIL_MOD    :
    case RDIL_SHL    :
    case RDIL_SHR    :
    case RDIL_AND    :
    case RDIL_OR     :
    case RDIL_XOR    :
    case RDIL_CMPEQ  :
    case RDIL_CMPLTU :
    case RDIL_CMPLTS :
    case RDIL_CMPLEU :
    case RDIL_CMPLES :
        va_start(ap, address);
        ins->dst = object_copy(va_arg(ap, struct _rdil_operand *));
        ins->lhs = object_copy(va_arg(ap, struct _rdil_operand *));
        ins->rhs = object_copy(va_arg(ap, struct _rdil_operand *));
        va_end(ap);
        break;

    default :
        free(ins);
        return NULL;
    }

    ins->object  = &rdil_ins_object;
    ins->type    = type;
    ins->address = address;

    return ins;
}


void rdil_ins_delete (struct _rdil_ins * rdil_ins)
{
    switch (rdil_ins->type) {
    case RDIL_SYSCALL :
    case RDIL_HLT     :
        break;

    // object_delete(rhs)
    case RDIL_ADD    :
    case RDIL_SUB    :
    case RDIL_MUL    :
    case RDIL_DIV    :
    case RDIL_MOD    :
    case RDIL_SHL    :
    case RDIL_SHR    :
    case RDIL_AND    :
    case RDIL_OR     :
    case RDIL_XOR    :
    case RDIL_CMPEQ  :
    case RDIL_CMPLTU :
    case RDIL_CMPLTS :
    case RDIL_CMPLEU :
    case RDIL_CMPLES :
        object_delete(rdil_ins->rhs);


    case RDIL_LOAD   :
    case RDIL_STORE  :
    case RDIL_BRC    :
    case RDIL_ASSIGN :
    case RDIL_NOT    :
    case RDIL_SEXT   :
        object_delete(rdil_ins->lhs);
        object_delete(rdil_ins->dst);
        break;
    }

    free(rdil_ins);
}


struct _rdil_ins * rdil_ins_copy (struct _rdil_ins * rdil_ins)
{

    switch (rdil_ins->type) {
    case RDIL_SYSCALL :
    case RDIL_HLT     :
        return rdil_ins_create(rdil_ins->type, rdil_ins->address);

    // object_delete(rhs)
    case RDIL_ADD    :
    case RDIL_SUB    :
    case RDIL_MUL    :
    case RDIL_DIV    :
    case RDIL_MOD    :
    case RDIL_SHL    :
    case RDIL_SHR    :
    case RDIL_AND    :
    case RDIL_OR     :
    case RDIL_XOR    :
    case RDIL_CMPEQ  :
    case RDIL_CMPLTU :
    case RDIL_CMPLTS :
    case RDIL_CMPLEU :
    case RDIL_CMPLES :
        return rdil_ins_create(rdil_ins->type,
                               rdil_ins->address,
                               rdil_ins->dst,
                               rdil_ins->lhs,
                               rdil_ins->rhs);
    case RDIL_LOAD   :
    case RDIL_STORE  :
        return rdil_ins_create(rdil_ins->type,
                               rdil_ins->address,
                               rdil_ins->dst,
                               rdil_ins->src,
                               rdil_ins->bits);
    case RDIL_BRC    :
    case RDIL_ASSIGN :
    case RDIL_NOT    :
    case RDIL_SEXT   :
        return rdil_ins_create(rdil_ins->type,
                               rdil_ins->address,
                               rdil_ins->dst,
                               rdil_ins->src);
        break;
    }

    return NULL;
}


const char * rdil_ins_mnemonic (struct _rdil_ins * rdil_ins)
{
    return rdil_mnemonics[rdil_ins->type];
}
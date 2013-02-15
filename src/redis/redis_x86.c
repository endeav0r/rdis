#include "redis_x86.h"

#include "buffer.h"

#include <stdlib.h>
#include <string.h>

static const struct _object redis_x86_object = {
    (void     (*) (void *))         redis_x86_delete, 
    (void *   (*) (void *))         redis_x86_copy,
    NULL,
    NULL,
    NULL,
};


struct _redis_x86 * redis_x86_create ()
{
    struct _redis_x86 * redis_x86;

    redis_x86 = (struct _redis_x86 *) malloc(sizeof(struct _redis_x86));

    redis_x86->object = &redis_x86_object;
    redis_x86->mem    = map_create();
    memset(redis_x86->regs, 0, sizeof(redis_x86->regs));

    return redis_x86;
}


void redis_x86_delete (struct _redis_x86 * redis_x86)
{
    object_delete (redis_x86->mem);
    free(redis_x86);
}


struct _redis_x86 * redis_x86_copy (struct _redis_x86 * redis_x86)
{
    struct _redis_x86 * new_redis_x86 = redis_x86_create();

    object_delete (new_redis_x86->mem);
    new_redis_x86->mem = object_copy(redis_x86->mem);

    memcpy(new_redis_x86->regs, redis_x86->regs, sizeof(redis_x86->regs));

    return new_redis_x86;
}


void redis_x86_mem_from_mem_map (struct _redis_x86 * redis_x86,
                                 struct _map * mem_map)
{
    object_delete(redis_x86->mem);
    redis_x86->mem = object_copy(mem_map);
}

void redis_x86_false_stack (struct _redis_x86 * redis_x86)
{
    uint64_t false_stack_addr = 0xfff80000 - REDIS_X86_FALSE_STACK_SIZE;

    struct _buffer * buffer = buffer_create_null(REDIS_X86_FALSE_STACK_SIZE);
    map_insert(redis_x86->mem, false_stack_addr, buffer);
    redis_x86->regs[RED_ESP] = 0xffff0000 - 64;
    redis_x86->regs[RED_EBP] = redis_x86->regs[RED_ESP];
}


int redis_x86_step (struct _redis_x86 * redis_x86)
{
    uint8_t ins_mem[16];
    size_t  ins_mem_size;

    uint64_t         buf_addr = map_fetch_max_key(redis_x86->mem,
                                                  redis_x86->regs[RED_EIP]);
    struct _buffer * buf      = map_fetch_max(redis_x86->mem,
                                              redis_x86->regs[RED_EIP]);

    if (buf == NULL)
        return REDIS_INVALID_IP;

    if (buf->size - redis_x86->regs[RED_EIP] < 16)
        ins_mem_size = buf->size - redis_x86->regs[RED_EIP];
    else
        ins_mem_size = 16;

    memcpy(ins_mem,
           &(buf->bytes[redis_x86->regs[RED_EIP] - buf_addr]),
           ins_mem_size);

    ud_t ud_obj;
    ud_init            (&ud_obj);
    ud_set_mode        (&ud_obj, 32);
    ud_set_syntax      (&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, ins_mem, ins_mem_size);

    if (ud_disassemble(&ud_obj) != 0) {
        printf("redis_x86_step: %s\n", ud_insn_asm(&ud_obj));
        switch (ud_obj.mnemonic) {
        case UD_Imov    : return redis_x86_mov(redis_x86, &ud_obj);
//        case UD_Ifld2lt : return redis_x86_fld2lt(redis_x86, &ud_obj);
        default :
            fprintf(stderr, "invalid/unsupported x86 ins %s\n",
                    ud_insn_asm(&ud_obj));
            return REDIS_INVALID_INSTRUCTION;
        }
    }

    return REDIS_INVALID_INSTRUCTION;
}


uint32_t redis_x86_reg_value (struct _redis_x86 * redis_x86, enum ud_type reg)
{
    switch (reg) {
    case UD_R_AL  : return redis_x86->regs[RED_EAX] & 0xff;
    case UD_R_BL  : return redis_x86->regs[RED_EBX] & 0xff;
    case UD_R_CL  : return redis_x86->regs[RED_ECX] & 0xff;
    case UD_R_DL  : return redis_x86->regs[RED_EDX] & 0xff;
    case UD_R_AX  : return redis_x86->regs[RED_EAX] & 0xffff;
    case UD_R_BX  : return redis_x86->regs[RED_EBX] & 0xffff;
    case UD_R_CX  : return redis_x86->regs[RED_ECX] & 0xffff;
    case UD_R_DX  : return redis_x86->regs[RED_EDX] & 0xffff;
    case UD_R_EAX : return redis_x86->regs[RED_EAX];
    case UD_R_EBX : return redis_x86->regs[RED_EBX];
    case UD_R_ECX : return redis_x86->regs[RED_ECX];
    case UD_R_EDX : return redis_x86->regs[RED_EDX];
    case UD_R_EBP : return redis_x86->regs[RED_EBP];
    case UD_R_ESP : return redis_x86->regs[RED_ESP];
    case UD_R_ESI : return redis_x86->regs[RED_ESI];
    case UD_R_EDI : return redis_x86->regs[RED_EDI];
    default :
        fprintf(stderr, "redis_x86_sib operand.base %d\n", reg);
    }
    return 0;
}


uint32_t redis_x86_sib (struct _redis_x86 * redis_x86, ud_t * ud_obj, int operand)
{
    struct ud_operand * oper = &(ud_obj->operand[operand]);
    
    uint32_t value = (oper->base == UD_NONE
                      ? 0
                      : redis_x86_reg_value(redis_x86, oper->base));
    
    uint32_t scale_index = (oper->index == UD_NONE
                            ? 0
                            : redis_x86_reg_value(redis_x86, oper->index));
    if (oper->scale > 0)
        scale_index *= oper->scale;
    value += scale_index;

    switch (oper->size) {
    case 8  : value += oper->lval.ubyte; break;
    case 16 : value += oper->lval.uword; break;
    case 32 : value += oper->lval.udword; break;
    }

    return value;
}


uint32_t redis_x86_get_operand_value (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand)
{
    struct ud_operand * oper = &(ud_obj->operand[operand]);

    switch (oper->type) {
    case UD_OP_REG   : return redis_x86_reg_value(redis_x86, oper->base);
    case UD_OP_CONST :
    case UD_OP_IMM   :
        switch (oper->size) {
        case 8  : return oper->lval.ubyte;
        case 16 : return oper->lval.uword;
        case 32 : return oper->lval.udword;
        default :
            fprintf(stderr, "redis_x86_get_operand_value UD_OP_CONST size %d\n",
                    oper->size);
        }
        break;
    case UD_OP_MEM  : printf("redis_x86_get_operand_value UD_OP_MEM\n"); break;
    case UD_OP_PTR  : printf("redis_x86_get_operand_value UD_OP_PTR\n"); break;
    case UD_OP_JIMM : printf("redis_x86_get_operand_value UD_OP_JIMM\n"); break;
    default :
        fprintf(stderr, "redis_x86_get_operand_value type %d\n",
                oper->type);
    }
    return -1;
}


int redis_x86_set_operand_value (struct _redis_x86 * redis_x86,
                                 ud_t * ud_obj,
                                 int operand,
                                 uint32_t value)
{
    struct ud_operand * oper = &(ud_obj->operand[operand]);

    switch (oper->type) {
    case UD_OP_REG :
        switch (oper->base) {
        case UD_R_EAX : redis_x86->regs[RED_EAX] = value; break;
        case UD_R_EBX : redis_x86->regs[RED_EBX] = value; break;
        case UD_R_ECX : redis_x86->regs[RED_ECX] = value; break;
        case UD_R_EDX : redis_x86->regs[RED_EDX] = value; break;
        case UD_R_ESP : redis_x86->regs[RED_ESP] = value; break;
        case UD_R_EBP : redis_x86->regs[RED_EBP] = value; break;
        case UD_R_ESI : redis_x86->regs[RED_ESI] = value; break;
        case UD_R_EDI : redis_x86->regs[RED_EDI] = value; break;
        case UD_R_AL  : redis_x86->regs[RED_EAX] = value & 0xff; break;
        case UD_R_BL  : redis_x86->regs[RED_EBX] = value & 0xff; break;
        case UD_R_CL  : redis_x86->regs[RED_ECX] = value & 0xff; break;
        case UD_R_DL  : redis_x86->regs[RED_EDX] = value & 0xff; break;
        default :
            fprintf(stderr, "redis_x86_set_operand_value UD_OP_REG %d\n",
                    oper->base);
        }
        break;
    case UD_OP_MEM  : printf("redis_x86_get_operand_value UD_OP_MEM\n");  return -1;
    case UD_OP_PTR  : printf("redis_x86_get_operand_value UD_OP_PTR\n");  return -1;
    case UD_OP_IMM  : printf("redis_x86_get_operand_value UD_OP_IMM\n");  return -1;
    case UD_OP_JIMM : printf("redis_x86_get_operand_value UD_OP_JIMM\n"); return -1;
    default :
        fprintf(stderr, "redis_x86_set_operand_value type %d\n", oper->type);
        return -1;
    }

    return 0;
}


int redis_x86_fld2lt (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    redis_x86->last_fpu_ins = redis_x86->regs[RED_EIP];
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);

    return REDIS_SUCCESS;
}


int redis_x86_mov (struct _redis_x86 * redis_x86, ud_t * ud_obj) {
    uint32_t rval = redis_x86_get_operand_value (redis_x86, ud_obj, 1);

    if (redis_x86_set_operand_value(redis_x86, ud_obj, 0, rval))
        return REDIS_EXEC_ERROR;
    
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}
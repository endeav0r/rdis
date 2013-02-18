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
    struct _buffer * buffer = buffer_create_null(REDIS_X86_FALSE_STACK_SIZE);
    map_insert(redis_x86->mem, REDIS_X86_FALSE_STACK_ADDR, buffer);
    redis_x86->regs[RED_ESP] = REDIS_X86_FALSE_STACK_ADDR
                               + REDIS_X86_FALSE_STACK_SIZE
                               - 64;
    redis_x86->regs[RED_EBP] = redis_x86->regs[RED_ESP];
}


char * redis_debug_ins_str (ud_t * ud_obj, char * buf)
{
    buf[0] = 0;
    size_t i = 0;
    for (i = 0; i < ud_insn_len(ud_obj) % 8; i++) {
        sprintf(&(buf[i*2]), "%02x", ud_insn_ptr(ud_obj)[i]);
    }
    while (i++ < 8) {
        strcat(buf, "  ");
    }
    return buf;
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
        redis_x86->ins_size = ud_insn_len(&ud_obj);
        memcpy(redis_x86->ins_bytes, ins_mem, ins_mem_size);

        char buff[256];
        printf("redis_x86_step: %x %s %s \teax=%x,ebx=%x,ecx=%x,edx=%x,esi=%x,edi=%x\n",
               redis_x86->regs[RED_EIP],
               redis_debug_ins_str(&ud_obj, buff),
               ud_insn_asm(&ud_obj),
               redis_x86->regs[RED_EAX],
               redis_x86->regs[RED_EBX],
               redis_x86->regs[RED_ECX],
               redis_x86->regs[RED_EDX],
               redis_x86->regs[RED_ESI],
               redis_x86->regs[RED_EDI]);
        switch (ud_obj.mnemonic) {
        case UD_Iadd     : return redis_x86_add(redis_x86, &ud_obj);
        case UD_Iand     : return redis_x86_add(redis_x86, &ud_obj);
        case UD_Icdq     : return redis_x86_cdq(redis_x86, &ud_obj);
        case UD_Ifldl2t  : return redis_x86_fldl2t(redis_x86, &ud_obj);
        case UD_Ifnstenv : return redis_x86_fnstenv(redis_x86, &ud_obj);
        case UD_Iinc     : return redis_x86_int(redis_x86, &ud_obj);
        case UD_Iint     : return redis_x86_int(redis_x86, &ud_obj);
        case UD_Ijl      : return redis_x86_jl(redis_x86, &ud_obj);
        case UD_Iloop    : return redis_x86_loop(redis_x86, &ud_obj);
        case UD_Imov     : return redis_x86_mov(redis_x86, &ud_obj);
        case UD_Imul     : return redis_x86_mul(redis_x86, &ud_obj);
        case UD_Ipop     : return redis_x86_pop(redis_x86, &ud_obj);
        case UD_Ipush    : return redis_x86_push(redis_x86, &ud_obj);
        case UD_Ixor     : return redis_x86_xor(redis_x86, &ud_obj);
        default :
            fprintf(stderr, "invalid/unsupported x86 ins %s\n",
                    ud_insn_asm(&ud_obj));
            return REDIS_INVALID_INSTRUCTION;
        }
    }

    return REDIS_INVALID_INSTRUCTION;
}


uint32_t redis_x86_reg_value (struct _redis_x86 * redis_x86, enum ud_type reg, int * error)
{
    *error = REDIS_SUCCESS;
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
        *error = REDIS_EXEC_ERROR;
        fprintf(stderr, "redis_x86_sib operand.base %d\n", reg);
    }
    return 0;
}


uint32_t redis_x86_sib (struct _redis_x86 * redis_x86, ud_t * ud_obj,
                        int operand,
                        int * error)
{
    struct ud_operand * oper = &(ud_obj->operand[operand]);

    uint32_t value = (oper->base == UD_NONE
                      ? 0
                      : redis_x86_reg_value(redis_x86, oper->base, error));

    if (*error)
        return 0;
    
    uint32_t scale_index = (oper->index == UD_NONE
                            ? 0
                            : redis_x86_reg_value(redis_x86, oper->index, error));
    if (*error)
        return 0;

    if (oper->scale > 0)
        scale_index *= oper->scale;
    value += scale_index;

    switch (oper->offset) {
    case 8  : value += oper->lval.sbyte; break;
    case 16 : value += oper->lval.sword; break;
    case 32 : value += oper->lval.sdword; break;
    }

    return value;
}


uint32_t redis_x86_get_operand_value (struct _redis_x86 * redis_x86,
                                      ud_t * ud_obj,
                                      int operand,
                                      int * error)
{
    struct ud_operand * oper = &(ud_obj->operand[operand]);

    *error = REDIS_EXEC_ERROR;

    switch (oper->type) {
    case UD_OP_REG   : return redis_x86_reg_value(redis_x86, oper->base, error);
    case UD_OP_CONST :
    case UD_OP_IMM   :
        switch (oper->size) {
        case 8  : *error = REDIS_SUCCESS; return oper->lval.ubyte;
        case 16 : *error = REDIS_SUCCESS; return oper->lval.uword;
        case 32 : *error = REDIS_SUCCESS; return oper->lval.udword;
        default :
            fprintf(stderr, "redis_x86_get_operand_value UD_OP_CONST size %d\n",
                    oper->size);
        }
        break;
    case UD_OP_MEM  : {
        uint32_t address = redis_x86_sib(redis_x86, ud_obj, operand, error);
        if (*error)
            return -1;
        switch (oper->size) {
        case 8  :  return redis_x86_mem_get8 (redis_x86, address, error);
        case 16 :  return redis_x86_mem_get16(redis_x86, address, error);
        case 32 :  return redis_x86_mem_get32(redis_x86, address, error);
        default :
            *error = REDIS_EXEC_ERROR;
            return -1;
        }
        break;
    }
    case UD_OP_PTR  : printf("redis_x86_get_operand_value UD_OP_PTR\n"); break;
    case UD_OP_JIMM : printf("redis_x86_get_operand_value UD_OP_JIMM\n"); break;
    default :
        fprintf(stderr, "redis_x86_get_operand_value type %d\n",
                oper->type);
    }
    return -1;
}


int redis_x86_set_operand (struct _redis_x86 * redis_x86,
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
        case UD_R_AX  : redis_x86->regs[RED_EAX] = value & 0xffff; break;
        case UD_R_BX  : redis_x86->regs[RED_EBX] = value & 0xffff; break;
        case UD_R_CX  : redis_x86->regs[RED_ECX] = value & 0xffff; break;
        case UD_R_DX  : redis_x86->regs[RED_EDX] = value & 0xffff; break;
        default :
            fprintf(stderr, "redis_x86_set_operand_value UD_OP_REG %d\n",
                    oper->base);
            return -1;
        }
        break;
    case UD_OP_MEM  : {
        int error;
        uint32_t address = redis_x86_sib(redis_x86, ud_obj, operand, &error);
        if (error)
            return error;
        switch (oper->size) {
        case 32 :  return redis_x86_mem_set32(redis_x86, address, value);
        }
        return REDIS_EXEC_ERROR;
    }
    case UD_OP_PTR  : printf("redis_x86_set_operand_value UD_OP_PTR\n");  return -1;
    case UD_OP_IMM  : printf("redis_x86_set_operand_value UD_OP_IMM\n");  return -1;
    case UD_OP_JIMM : printf("redis_x86_set_operand_value UD_OP_JIMM\n"); return -1;
    default :
        fprintf(stderr, "redis_x86_set_operand_value type %d\n", oper->type);
        return -1;
    }

    return 0;
}


int redis_x86_mem_set32 (struct _redis_x86 * redis_x86, uint32_t addr, uint32_t value)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    printf("[+] (mem_write32) %x = %x\n", addr, value);

    if (buffer == NULL)
        return REDIS_MEM_ERROR;
    if (addr - buf_addr > buffer->size - 4)
        return REDIS_MEM_ERROR;

    uint32_t offset = addr - buf_addr;

    buffer->bytes[offset  ] = (value >> 0 ) & 0xff;
    buffer->bytes[offset+1] = (value >> 8 ) & 0xff;
    buffer->bytes[offset+2] = (value >> 16) & 0xff;
    buffer->bytes[offset+3] = (value >> 24) & 0xff;

    return REDIS_SUCCESS;
}


uint32_t redis_x86_mem_get32 (struct _redis_x86 * redis_x86, uint32_t addr, int * error)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    printf("%x %x %x\n", addr, buf_addr, (unsigned int) buffer->size);

    if (buffer == NULL) {
        *error = REDIS_MEM_ERROR;
        return -1;
    }
    if (addr - buf_addr > buffer->size - 4) {
        *error = REDIS_MEM_ERROR;
        return -1;
    }

    uint32_t offset = addr - buf_addr;
    uint32_t value  = 0;
    value |= buffer->bytes[offset  ] << 0;
    value |= buffer->bytes[offset+1] << 8;
    value |= buffer->bytes[offset+2] << 16;
    value |= buffer->bytes[offset+3] << 24;

    printf("[-] (mem_read32) %x = %x\n", addr, value);

    *error = REDIS_SUCCESS;

    return value;
}


uint16_t redis_x86_mem_get16 (struct _redis_x86 * redis_x86, uint32_t addr, int * error)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    printf("%x %x %x %x\n", addr, buf_addr, addr - buf_addr, (unsigned int) buffer->size);

    if (buffer == NULL) {
        *error = REDIS_MEM_ERROR;
        return -1;
    }
    if (addr - buf_addr > buffer->size - 2) {
        *error = REDIS_MEM_ERROR;
        return -1;
    }

    uint32_t offset = addr - buf_addr;
    uint16_t value  = 0;
    value |= buffer->bytes[offset  ];
    value |= buffer->bytes[offset+1] << 8;

    *error = REDIS_SUCCESS;

    return value;
}


uint8_t redis_x86_mem_get8 (struct _redis_x86 * redis_x86, uint32_t addr, int * error)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    printf("%x %x %x %x\n", addr, buf_addr, addr - buf_addr, (unsigned int) buffer->size);

    if (buffer == NULL) {
        *error = REDIS_MEM_ERROR;
        return -1;
    }
    if (addr - buf_addr > buffer->size - 1) {
        *error = REDIS_MEM_ERROR;
        return -1;
    }

    *error = REDIS_SUCCESS;

    uint32_t offset = addr - buf_addr;
    return buffer->bytes[offset];
}


void redis_x86_set_flags (struct _redis_x86 * redis_x86,
                          uint32_t lval, uint32_t rval, uint32_t result)
{
    uint32_t SFxorOF = ((uint32_t) lval < (uint32_t) rval) ? 1 : 0;

    redis_x86->regs[RED_CF] = ((lval + rval) < lval) ? 1 : 0;
    redis_x86->regs[RED_ZF] = ((lval + rval) == 0)   ? 1 : 0;
    redis_x86->regs[RED_SF] = ((lval + rval) & 0x80000000) ? 1 : 0;
    redis_x86->regs[RED_OF] = (SFxorOF ^ redis_x86->regs[RED_SF]) & 0x1;
}


int redis_x86_add (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    
    uint32_t lval = redis_x86_get_operand_value (redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    uint32_t rval = redis_x86_get_operand_value (redis_x86, ud_obj, 1, &error);
    if (error)
        return error;

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, lval + rval);
    if (error)
        return error;

    redis_x86_set_flags(redis_x86, lval, rval, lval + rval);

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_and (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    
    uint32_t lval = redis_x86_get_operand_value (redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    uint32_t rval = redis_x86_get_operand_value (redis_x86, ud_obj, 1, &error);
    if (error)
        return error;

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, lval & rval);
    if (error)
        return error;

    redis_x86->regs[RED_OF] = 0;
    redis_x86->regs[RED_CF] = 0;
    redis_x86->regs[RED_SF] = ((lval ^ rval) & 0x80000000) ? 1 : 0;
    redis_x86->regs[RED_ZF] = ((lval ^ rval) == 0) ? 1 : 0;

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_cdq (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    if (redis_x86->regs[RED_EAX] & 0x80000000)
        redis_x86->regs[RED_EDX] = 0xffffffff;
    else
        redis_x86->regs[RED_EDX] = 0x00000000;
    
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);

    return REDIS_SUCCESS;
}


int redis_x86_fldl2t (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    redis_x86->last_fpu_ins = redis_x86->regs[RED_EIP];
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);

    return REDIS_SUCCESS;
}


int redis_x86_fnstenv (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    uint32_t addr = redis_x86_sib(redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    printf("last_fpu_ins=%x, addr for last_fpu_ins=%x, esp=%x\n",
           (unsigned int) redis_x86->last_fpu_ins,
           addr + 0xc,
           redis_x86->regs[RED_ESP]);
    error = redis_x86_mem_set32(redis_x86, addr + 0xc, redis_x86->last_fpu_ins);
    if (error)
        return error;

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);

    return REDIS_SUCCESS;
}


int redis_x86_inc (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    
    uint32_t lval = redis_x86_get_operand_value (redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, lval + 1);
    if (error)
        return error;

    uint32_t cf = redis_x86->regs[RED_CF]; // the cf is not affected
    redis_x86_set_flags(redis_x86, lval, 1, lval + 1);
    redis_x86->regs[RED_CF] = cf;

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_int (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_jl (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    if (redis_x86->regs[RED_SF] != redis_x86->regs[RED_OF]) {
        switch (ud_obj->operand[0].size) {
        case 8 :
            redis_x86->regs[RED_EIP] += ud_obj->operand[0].lval.sbyte;
            break;
        case 16 :
            redis_x86->regs[RED_EIP] += ud_obj->operand[0].lval.sword;
            break;
        case 32 :
            redis_x86->regs[RED_EIP] += ud_obj->operand[0].lval.sdword;
            break;
        default :
            return REDIS_EXEC_ERROR;
        }
    }
    
    return REDIS_SUCCESS;
}


int redis_x86_loop (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    printf("%x %x %x %x\n",
           (unsigned int) ud_insn_ptr(ud_obj)[0],
           (unsigned int) ud_insn_ptr(ud_obj)[1],
           (unsigned int) ud_insn_ptr(ud_obj)[2],
           (unsigned int) ud_insn_ptr(ud_obj)[3]);
    redis_x86->regs[RED_ECX]--;

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    if (redis_x86->regs[RED_ECX]) {
        redis_x86->regs[RED_EIP] += ud_obj->operand[0].lval.sbyte;
    }
    
    return REDIS_SUCCESS;
}


int redis_x86_mov (struct _redis_x86 * redis_x86, ud_t * ud_obj) {
    int error;
    uint32_t rval = redis_x86_get_operand_value (redis_x86, ud_obj, 1, &error);
    if (error)
        return error;

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, rval);
    if (error)
        return error;
    
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_mul (struct _redis_x86 * redis_x86, ud_t * ud_obj) {
    int error;
    uint32_t rval = redis_x86_get_operand_value (redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    uint32_t lval = redis_x86->regs[RED_EAX];
    uint64_t result = (uint64_t) lval * (uint64_t) rval;

    redis_x86->regs[RED_EAX] = result & 0xffffffff;
    redis_x86->regs[RED_EDX] = (result >> 32) & 0xffffffff;

    if (result >> 32) {
        redis_x86->regs[RED_OF] = 1;
        redis_x86->regs[RED_CF] = 1;
    }
    else {
        redis_x86->regs[RED_OF] = 0;
        redis_x86->regs[RED_CF] = 0;
    }

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_pop (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    uint32_t value = redis_x86_mem_get32(redis_x86, redis_x86->regs[RED_ESP], &error);
    if (error) {
        printf("mem get error\n");
        return error;
    }

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, value);
    if (error) {
        printf("operand set error\n");
        return error;
    }

    redis_x86->regs[RED_ESP] += 4;

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);

    return REDIS_SUCCESS;
}


int redis_x86_push (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    uint32_t value = redis_x86_get_operand_value(redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    redis_x86->regs[RED_ESP] -= 4;
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);

    return redis_x86_mem_set32(redis_x86, redis_x86->regs[RED_ESP], value);
}


int redis_x86_xor (struct _redis_x86 * redis_x86, ud_t * ud_obj) {
    int error;
    
    uint32_t lval = redis_x86_get_operand_value (redis_x86, ud_obj, 0, &error);
    if (error)
        return error;

    uint32_t rval = redis_x86_get_operand_value (redis_x86, ud_obj, 1, &error);
    if (error)
        return error;

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, lval ^ rval);
    if (error)
        return error;

    redis_x86->regs[RED_OF] = 0;
    redis_x86->regs[RED_CF] = 0;
    redis_x86->regs[RED_SF] = ((lval ^ rval) & 0x80000000) ? 1 : 0;
    redis_x86->regs[RED_ZF] = ((lval ^ rval) == 0) ? 1 : 0;

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}
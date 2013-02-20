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

    new_redis_x86->error = redis_x86->error;
    new_redis_x86->last_fpu_ins = redis_x86->last_fpu_ins;
    new_redis_x86->ins_addr = redis_x86->ins_addr;
    new_redis_x86->ins_size = redis_x86->ins_size;
    memcpy(new_redis_x86->ins_bytes, redis_x86->ins_bytes, 16);
    memcpy(new_redis_x86->error_msg, redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE);

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

    if (buf == NULL) {
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "invalid eip=%08x", redis_x86->regs[RED_EIP]);
        return REDIS_INVALID_IP;
    }

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
        redis_x86->ins_addr = redis_x86->regs[RED_EIP];
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
        case UD_Ijmp     : return redis_x86_jmp(redis_x86, &ud_obj);
        case UD_Iloop    : return redis_x86_loop(redis_x86, &ud_obj);
        case UD_Imov     : return redis_x86_mov(redis_x86, &ud_obj);
        case UD_Imul     : return redis_x86_mul(redis_x86, &ud_obj);
        case UD_Ipop     : return redis_x86_pop(redis_x86, &ud_obj);
        case UD_Ipush    : return redis_x86_push(redis_x86, &ud_obj);
        case UD_Ixor     : return redis_x86_xor(redis_x86, &ud_obj);
        default :
            snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                     "unsupported instruction %s", ud_insn_asm(&ud_obj));
            return REDIS_INVALID_INSTRUCTION;
        }
    }

    snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE, "invalid instruction");
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
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_sib operand.base=%d", reg);
    }
    return 0;
}


uint32_t redis_lval_signed (struct _redis_x86 * redis_x86,
                            struct ud_operand * operand,
                            int * error)
{
    *error = REDIS_SUCCESS;
    switch (operand->size) {
    case 8  : return (int32_t) operand->lval.sbyte;
    case 16 : return (int32_t) operand->lval.sword;
    case 32 : return (int32_t) operand->lval.sdword;
    }

    snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
             "invalid size for redis_lval_signed %d", operand->size);
    *error = REDIS_EXEC_ERROR;
    return 0;
}


uint32_t redis_x86_sib (struct _redis_x86 * redis_x86, ud_t * ud_obj,
                        int operand,
                        int * error)
{
    *error = REDIS_SUCCESS;
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
            snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                     "redis_x86_get_operand_value UD_OP_CONST size %d",
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
            snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                     "invalid mem op size %d", oper->size);
            *error = REDIS_EXEC_ERROR;
            return -1;
        }
        break;
    }
    case UD_OP_PTR  :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_get_operand_value UD_OP_PTR");
        break;
    case UD_OP_JIMM :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_get_operand_value UD_OP_JIMM");
        break;
    default :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_get_operand_value type %d",
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
            snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                     "redis_x86_set_operand_value UD_OP_REG %d\n",
                     oper->base);
            return -1;
        }
        break;
    case UD_OP_MEM  : {
        printf("UD_OP_MEM\n");
        int error;
        uint32_t address = redis_x86_sib(redis_x86, ud_obj, operand, &error);
        if (error)
            return error;
        switch (oper->size) {
        case 32 : return redis_x86_mem_set32(redis_x86, address, value);
        case 8  : return redis_x86_mem_set8(redis_x86, address, value);
        }
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE, "UD_UP_MEM error");
        return REDIS_EXEC_ERROR;
    }
    case UD_OP_PTR :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_set_operand_value UD_OP_PTR");
        return REDIS_EXEC_ERROR;
    case UD_OP_IMM :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_set_operand_value UD_OP_IMM");
        return REDIS_EXEC_ERROR;
    case UD_OP_JIMM :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_set_operand_value UD_OP_JIMM");
        return REDIS_EXEC_ERROR;
    default :
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "redis_x86_set_operand_value type %d",
                 oper->type);
        return REDIS_EXEC_ERROR;
    }

    return REDIS_SUCCESS;
}


int redis_x86_mem_set32 (struct _redis_x86 * redis_x86, uint32_t addr, uint32_t value)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    if ((buffer == NULL) || (addr - buf_addr > buffer->size - 4)) {
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "mem set 32 to invalid address %08x", addr);
        return REDIS_MEM_ERROR;
    }

    printf("[+] (mem_write32) %x = %x\n", addr, value);

    uint32_t offset = addr - buf_addr;

    buffer->bytes[offset  ] = (value >> 0 ) & 0xff;
    buffer->bytes[offset+1] = (value >> 8 ) & 0xff;
    buffer->bytes[offset+2] = (value >> 16) & 0xff;
    buffer->bytes[offset+3] = (value >> 24) & 0xff;

    return REDIS_SUCCESS;
}


int redis_x86_mem_set8 (struct _redis_x86 * redis_x86, uint32_t addr, uint8_t value)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    if ((buffer == NULL) || (addr - buf_addr > buffer->size - 1)) {
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "mem set 8 to invalid address %08x", addr);
        return REDIS_MEM_ERROR;
    }

    printf("[+] (mem_write8) %x = %x\n", addr, value);

    uint32_t offset = addr - buf_addr;

    buffer->bytes[offset] = value;

    return REDIS_SUCCESS;
}


uint32_t redis_x86_mem_get32 (struct _redis_x86 * redis_x86, uint32_t addr, int * error)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    printf("%x %x %x\n", addr, buf_addr, (unsigned int) buffer->size);

    if ((buffer == NULL) || (addr - buf_addr > buffer->size - 4)) {
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "mem get 32 from invalid address %08x", addr);
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

    if ((buffer == NULL) || (addr - buf_addr > buffer->size - 2)) {
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "mem get 16 from invalid address %08x", addr);
        *error = REDIS_MEM_ERROR;
        return -1;
    }

    uint32_t offset = addr - buf_addr;
    uint16_t value  = 0;
    value |= buffer->bytes[offset  ];
    value |= buffer->bytes[offset+1] << 8;

    printf("[-] (mem_read16) %x = %x\n", addr, value);

    *error = REDIS_SUCCESS;

    return value;
}


uint8_t redis_x86_mem_get8 (struct _redis_x86 * redis_x86, uint32_t addr, int * error)
{
    struct _buffer * buffer;
    uint32_t buf_addr;

    buf_addr = map_fetch_max_key(redis_x86->mem, addr);
    buffer   = map_fetch_max(redis_x86->mem, addr);

    if ((buffer == NULL) || (addr - buf_addr > buffer->size - 1)) {
        snprintf(redis_x86->error_msg, REDIS_X86_ERROR_MSG_SIZE,
                 "mem get 8 from invalid address %08x", addr);
        *error = REDIS_MEM_ERROR;
        return -1;
    }

    *error = REDIS_SUCCESS;

    uint32_t offset = addr - buf_addr;

    printf("[-] (mem_read8) %x = %x\n", addr, buffer->bytes[offset]);

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
    if (redis_x86->regs[RED_SF] != redis_x86->regs[RED_OF]) {
        int error;
        uint32_t offset = redis_lval_signed(redis_x86, &(ud_obj->operand[0]), &error);
        if (error)
            return error;

        redis_x86->regs[RED_EIP] += offset;
    }

    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_jmp (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
    int error;
    uint32_t offset = redis_lval_signed(redis_x86, &(ud_obj->operand[0]), &error);
    if (error)
        return error;

    redis_x86->regs[RED_EIP] += offset;
    
    redis_x86->regs[RED_EIP] += ud_insn_len(ud_obj);
    
    return REDIS_SUCCESS;
}


int redis_x86_loop (struct _redis_x86 * redis_x86, ud_t * ud_obj)
{
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
    if (error)
        return error;

    error = redis_x86_set_operand(redis_x86, ud_obj, 0, value);
    if (error)
        return error;

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
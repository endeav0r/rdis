#include "rdil_x86.h"

#include "list.h"


const char * rdil_x86_vars [] = {
    "al",
    "ah",
    "ax",
    "eax",
    "bl",
    "bh",
    "bx",
    "ebx",
    "cl",
    "ch",
    "cx",
    "ecx",
    "dl",
    "dh",
    "dx"
    "edx",
    "esi",
    "edi",
    "esp",
    "ebp",
    "fs",
    "of",
    "cf"
    "zf",
    "sf"
};


struct _list * rdil_x86 (struct _ins * ins)
{
    ud_t ud_obj;
    
    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, 32);

    ud_set_input_buffer(&ud_obj, ins->bytes, ins->size);

    if (ud_disassemble(&ud_obj)) {
        switch (ud_obj.mnemonic) {
            case UD_Iadd  : return rdil_x86_add (&ud_obj, ins->address);
            case UD_Ipush : return rdil_x86_push(&ud_obj, ins->address);
            case UD_Imov  : return rdil_x86_mov (&ud_obj, ins->address);
            default :
                break;
        }
    }

    return NULL;
}


char * rdil_x86_operand_text (struct _rdil_operand * operand)
{
    char * operand_text = malloc(32);

    if (operand->type == RDIL_CONSTANT) {
        snprintf(operand_text, 32, "(%d:%llx)",
                 operand->bits,
                 (unsigned long long) operand->value);
    }
    else if (operand->value < 0x100) {
        snprintf(operand_text, 32,
                 "(%d:%s)",    operand->bits,
                 rdil_x86_vars[operand->value]);
    }
    else
        snprintf(operand_text, 32, "(%d:tmp)", operand->bits);

    return operand_text;
}


char * rdil_x86_ins_text (struct _rdil_ins * rdil_ins)
{
    char * result = malloc(64);
    char * dst;
    char * src;
    char * cond;
    char * lhs;
    char * rhs;

    const char * mnemonic = rdil_ins_mnemonic(rdil_ins);

    switch(rdil_ins->type) {
    case RDIL_SYSCALL :
    case RDIL_HLT     :
        snprintf(result, 64, "%s", mnemonic);
        break;

    case RDIL_LOAD  :
    case RDIL_STORE :
        dst = rdil_x86_operand_text(rdil_ins->dst);
        src = rdil_x86_operand_text(rdil_ins->src);
        snprintf(result, 64, "%s (%d) %s, %s",
                 mnemonic,
                 rdil_ins->bits,
                 dst,
                 src);
        free(dst);
        free(src);
        break;

    case RDIL_BRC :
        dst  = rdil_x86_operand_text(rdil_ins->dst);
        cond = rdil_x86_operand_text(rdil_ins->cond);
        snprintf(result, 64, "%s (?%s) %s",
                 mnemonic,
                 cond,
                 dst);
        break;

    case RDIL_ASSIGN :
    case RDIL_NOT    :
    case RDIL_SEXT   :
        dst = rdil_x86_operand_text(rdil_ins->dst);
        src = rdil_x86_operand_text(rdil_ins->src);
        snprintf(result, 64, "%s %s, %s",
                 mnemonic,
                 dst,
                 src);
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
        dst = rdil_x86_operand_text(rdil_ins->dst);
        lhs = rdil_x86_operand_text(rdil_ins->lhs);
        rhs = rdil_x86_operand_text(rdil_ins->rhs);
        snprintf(result, 64, "%s %s, %s, %s",
                 mnemonic,
                 dst,
                 lhs,
                 rhs);
        break;

    default :
        snprintf(result, 64, "invalid instruction");
    }

    return result;
}


int rdil_x86_full_reg (int reg)
{
    switch (reg) {
        case RDIL_AL  :
        case RDIL_AH  :
        case RDIL_AX  :
        case RDIL_EAX :
            return RDIL_EAX;
        case RDIL_BL  :
        case RDIL_BH  :
        case RDIL_BX  :
        case RDIL_EBX :
            return RDIL_EBX;
        case RDIL_CL  :
        case RDIL_CH  :
        case RDIL_CX  :
        case RDIL_ECX :
            return RDIL_ECX;
        case RDIL_DL  :
        case RDIL_DH  :
        case RDIL_DX  :
        case RDIL_EDX :
            return RDIL_EDX;
        case RDIL_ESI :
        case RDIL_EDI :
        case RDIL_ESP :
        case RDIL_EBP :
        case RDIL_FS  :
        case RDIL_OF  :
        case RDIL_CF  :
        case RDIL_ZF  :
        case RDIL_SF  :
            return reg;
    }

    fprintf(stderr, "rx86fr unknown rdil reg %d\n", reg);
    exit(-1);
}


int rdil_x86_udis_reg_to_rdil_reg (int reg)
{
    switch (reg) {
    case UD_R_AL  : return RDIL_AL;
    case UD_R_AH  : return RDIL_AH;
    case UD_R_AX  : return RDIL_AX;
    case UD_R_EAX : return RDIL_EAX;
    case UD_R_BL  : return RDIL_BL;
    case UD_R_BH  : return RDIL_BH;
    case UD_R_BX  : return RDIL_BX;
    case UD_R_EBX : return RDIL_EBX;
    case UD_R_CL  : return RDIL_CL;
    case UD_R_CH  : return RDIL_CH;
    case UD_R_CX  : return RDIL_CX;
    case UD_R_ECX : return RDIL_ECX;
    case UD_R_DL  : return RDIL_DL;
    case UD_R_DH  : return RDIL_DH;
    case UD_R_DX  : return RDIL_DX;
    case UD_R_EDX : return RDIL_EDX;
    case UD_R_ESI : return RDIL_ESI;
    case UD_R_EDI : return RDIL_EDI;
    case UD_R_ESP : return RDIL_ESP;
    case UD_R_EBP : return RDIL_EBP;
    case UD_R_FS  : return RDIL_FS;
    }

    fprintf(stderr, "rx86urtrr unknown udis reg %d\n", reg);
    exit(-1);
}


int rdil_x86_reg_bits (int reg)
{
    switch (reg) {
    case RDIL_OF :
    case RDIL_CF :
    case RDIL_ZF :
    case RDIL_SF :
        return 1;
    case RDIL_AL :
    case RDIL_AH :
    case RDIL_BL :
    case RDIL_BH :
    case RDIL_CL :
    case RDIL_CH :
    case RDIL_DL :
    case RDIL_DH :
        return 8;
    case RDIL_AX :
    case RDIL_BX :
    case RDIL_CX :
    case RDIL_DX :
        return 16;
    case RDIL_EAX :
    case RDIL_EBX :
    case RDIL_ECX :
    case RDIL_EDX :
    case RDIL_ESP :
    case RDIL_EBP :
    case RDIL_FS  :
        return 32;
    }

    fprintf(stderr, "rx86rb unknown reg %d\n", reg);
    exit(-1);

    return -1;
}


uint64_t rdil_x86_operand_lval (int bits, struct ud_operand operand)
{
    switch (bits) {
    case 8  : return (uint64_t) operand.lval.ubyte;
    case 16 : return (uint64_t) operand.lval.uword;
    case 32 : return (uint64_t) operand.lval.udword;
    case 64 : return (uint64_t) operand.lval.uqword;
    }

    fprintf(stderr, "rx86ol invalid bits %d\n", bits);
    exit(-1);

    return 0;
}


struct _rdil_operand * rdil_x86_operand (ud_t *   ud_obj,
                                         int      operand_i,
                                         uint64_t address,
                                         struct _list * list)
{
    struct ud_operand operand = ud_obj->operand[operand_i];

    struct _rdil_operand * full_operand;
    struct _rdil_operand * rh;
    struct _rdil_operand * eight;
    int rdil_reg, full_reg;

    // AH BH CH DH registers
    if (operand.type == UD_OP_REG) {
        switch (operand.base) {
        case UD_R_AH :
        case UD_R_BH :
        case UD_R_CH :
        case UD_R_DH :
            rdil_reg = rdil_x86_udis_reg_to_rdil_reg(operand.base);
            full_reg = rdil_x86_full_reg(rdil_reg);

            full_operand = rdil_operand_create(RDIL_VAR,      32, full_reg);
            rh           = rdil_operand_create(RDIL_VAR,      8,  rdil_reg);
            eight        = rdil_operand_create(RDIL_CONSTANT, 8,  8);

            struct _rdil_ins * ins;
            ins = rdil_ins_create(RDIL_SHR, address, rh, full_operand, eight);
            list_append(list, ins);

            objects_delete(ins, full_operand, eight, NULL);

            return rh;
        default :
            break;
        }
    }

    // segment memory operands
    else if (    (operand.type == UD_OP_MEM)
              && (ud_obj->pfx_seg)) {
        int seg_reg = rdil_x86_udis_reg_to_rdil_reg(ud_obj->pfx_seg);
        struct _rdil_operand * seg;
        seg = rdil_operand_create(RDIL_VAR, rdil_x86_reg_bits(seg_reg), seg_reg);

        if (operand.offset) {
            struct _rdil_operand * offset;
            struct _rdil_operand * result;
            offset = rdil_operand_create(RDIL_CONSTANT,
                                         operand.offset,
                               rdil_x86_operand_lval(operand.offset, operand));
            result = rdil_operand_create(RDIL_VAR, 32, -1);
            
            struct _rdil_ins * ins;
            ins = rdil_ins_create(RDIL_ADD, address, result, seg, offset);
            
            list_append(list, ins);

            objects_delete(ins, offset, seg, NULL);

            return result;
        }
        else if (operand.base) {
            struct _rdil_operand * base;
            struct _rdil_operand * result;

            int reg = rdil_x86_udis_reg_to_rdil_reg(operand.base);

            base = rdil_operand_create(RDIL_VAR,
                                       rdil_x86_reg_bits(reg),
                                       reg);
            result = rdil_operand_create(RDIL_VAR, 32, -1);

            struct _rdil_ins * ins;
            ins = rdil_ins_create(RDIL_ADD, address, result, seg, base);

            list_append(list, ins);

            objects_delete(ins, base, seg, NULL);

            return result;
        }
    }

    // memory operands
    else if (operand.type == UD_OP_MEM) {
        struct _rdil_operand * base        = NULL;
        struct _rdil_operand * index       = NULL;
        struct _rdil_operand * scale       = NULL;
        struct _rdil_operand * displ       = NULL;
        struct _rdil_operand * index_scale = NULL;
        struct _rdil_operand * base_displ  = NULL;
        struct _rdil_operand * result      = NULL;

        if (operand.base) {
            int base_reg = rdil_x86_udis_reg_to_rdil_reg(operand.base);
            base = rdil_operand_create(RDIL_VAR,
                                       rdil_x86_reg_bits(base_reg),
                                       base_reg);
        }
        if (operand.index) {
            int index_reg = rdil_x86_udis_reg_to_rdil_reg(operand.index);
            index = rdil_operand_create(RDIL_VAR,
                                        rdil_x86_reg_bits(index_reg),
                                        index_reg);
        }
        if (operand.scale) {
            scale = rdil_operand_create(RDIL_CONSTANT, 8, operand.scale);
        }
        if (operand.offset) {
            displ = rdil_operand_create(RDIL_CONSTANT,
                                        operand.offset,
                               rdil_x86_operand_lval(operand.offset, operand));
        }

        if (operand.index && operand.scale) {
            index_scale = rdil_operand_create(RDIL_VAR, 32, -1);
            struct _rdil_ins * ins;
            ins = rdil_ins_create(RDIL_MUL, address, index_scale, index, scale);

            list_append(list, ins);

            object_delete(ins);
        }

        if (operand.base && operand.offset) {
            int base_reg = rdil_x86_udis_reg_to_rdil_reg(operand.base);
            base_displ = rdil_operand_create(RDIL_VAR,
                                             rdil_x86_reg_bits(base_reg),
                                             -1);

            struct _rdil_ins * ins0;
            struct _rdil_ins * ins1;
            ins0 = rdil_ins_create(RDIL_SEXT, address, base_displ, displ);
            ins1 = rdil_ins_create(RDIL_ADD,  address, base_displ, base, base_displ);
            list_append(list, ins0);
            list_append(list, ins1);
            object_delete(ins0);
            object_delete(ins1);
        }

        else if ((! operand.base) && (operand.offset)) {
            base_displ = displ;
        }

        if (operand.index) {
            result = rdil_operand_create(RDIL_VAR, 32, -1);
            struct _rdil_ins * ins;
            ins = rdil_ins_create(RDIL_ADD, address, result, index_scale, base_displ);
            list_append(list, ins);
            object_delete(ins);
        }
        else {
            result = object_copy(base_displ);
        }

        if (base  != NULL) object_delete(base);
        if (index != NULL) object_delete(index);
        if (scale != NULL) object_delete(scale);
        if (displ != NULL) object_delete(displ);
        if (index_scale != NULL) object_delete(index_scale);
        if ((base_displ != NULL) && (base_displ != base))
            object_delete(base_displ);

        return result;
    }

    else if (    (operand.type == UD_OP_CONST)
              || (operand.type == UD_OP_JIMM)
              || (operand.type == UD_OP_IMM)) {
        struct _rdil_operand * result;
        result = rdil_operand_create(RDIL_CONSTANT,
                                     operand.size,
                           rdil_x86_operand_lval(operand.size, operand));
        return result;
    }

    fprintf(stderr, "unsupported operand type %d\n", operand.type);
    exit(-1);

    return NULL;
}


struct _rdil_operand * rdil_x86_operand_load (ud_t *   ud_obj,
                                              int      operand_i,
                                              uint64_t address,
                                              int      bits,
                                              struct _list * list)
{
    struct _rdil_operand * addr;
    struct _rdil_operand * result = rdil_operand_create(RDIL_VAR, bits, -1);
    addr = rdil_x86_operand(ud_obj, operand_i, address, list);

    struct _rdil_ins * ins;

    ins = rdil_ins_create(RDIL_LOAD, address, result, addr, bits);
    list_append(list, ins);

    object_delete(ins);
    object_delete(addr);

    return result;
}


struct _rdil_operand * rdil_x86_operand_get (ud_t *   ud_obj,
                                             int      operand_i,
                                             uint64_t address,
                                             struct _list * list)
{
    if (ud_obj->operand[operand_i].type == UD_OP_MEM) {
        return rdil_x86_operand_load(ud_obj,
                                     operand_i,
                                     address,
                                     ud_obj->operand[operand_i].size,
                                     list);
    }

    return rdil_x86_operand(ud_obj, operand_i, address, list);
}


void rdil_x86_operand_set (ud_t *   ud_obj,
                           int      operand_i,
                           uint64_t address,
                           struct _rdil_operand * value,
                           struct _list *         list)
{
    if (ud_obj->operand[operand_i].type == UD_OP_MEM) {
        struct _rdil_operand * lhs;
        lhs = rdil_x86_operand(ud_obj, operand_i, address, list);
        struct _rdil_ins * ins;

        ins = rdil_ins_create(RDIL_STORE, address, lhs,
                              value,      ud_obj->operand[operand_i].size);

        list_append(list, ins);

        object_delete(ins);
        object_delete(lhs);
        return;
    }

    int reg = rdil_x86_udis_reg_to_rdil_reg(ud_obj->operand[operand_i].base);

    // special case for AH/BH etc. registers
    struct _rdil_operand * fullreg;
    struct _rdil_operand * tmp;
    struct _rdil_operand * eight;
    struct _rdil_operand * ffff00ff;
    switch (ud_obj->operand[operand_i].base) {
    case UD_R_AH :
    case UD_R_BH :
    case UD_R_CH :
    case UD_R_DH :
        fullreg  = rdil_operand_create(RDIL_VAR, 32, rdil_x86_full_reg(reg));
        tmp      = rdil_operand_create(RDIL_VAR, 32, -1);
        eight    = rdil_operand_create(RDIL_CONSTANT, 8, 8);
        ffff00ff = rdil_operand_create(RDIL_CONSTANT, 32, 0xffff00ff);

        struct _rdil_ins * ins[4];

        ins[0] = rdil_ins_create(RDIL_ASSIGN, address, tmp, value);
        ins[1] = rdil_ins_create(RDIL_SHL, address, tmp, tmp, eight);
        ins[2] = rdil_ins_create(RDIL_AND, address, fullreg, fullreg, ffff00ff);
        ins[3] = rdil_ins_create(RDIL_OR,  address, fullreg, fullreg, tmp);

        int i;
        for (i = 0; i < 4; i++) {
            list_append(list, ins[i]);
            object_delete(ins);
        }

        objects_delete(fullreg, tmp, eight, ffff00ff, NULL);

        return;
    default :
        break;
    }

    int full_reg = rdil_x86_full_reg(reg);
    struct _rdil_operand * dst = rdil_operand_create(RDIL_VAR, 32, full_reg);
    struct _rdil_ins     * ins = rdil_ins_create(RDIL_ASSIGN, address, dst, value);
    list_append(list, ins);
    object_delete(ins);
    object_delete(dst);

    return;
}



struct _list * rdil_x86_add (ud_t * ud_obj, uint64_t address)
{
    struct _list * list = list_create();

    struct _rdil_operand * lhs     = rdil_x86_operand_get(ud_obj, 0, address, list);
    struct _rdil_operand * rhs     = rdil_x86_operand_get(ud_obj, 1, address, list);
    struct _rdil_operand * tmp     = rdil_operand_create(RDIL_VAR, lhs->bits, -1);
    struct _rdil_operand * OF      = rdil_operand_create(RDIL_VAR, 1, RDIL_OF);
    struct _rdil_operand * CF      = rdil_operand_create(RDIL_VAR, 1, RDIL_CF);
    struct _rdil_operand * ZF      = rdil_operand_create(RDIL_VAR, 1, RDIL_ZF);
    struct _rdil_operand * SF      = rdil_operand_create(RDIL_VAR, 1, RDIL_SF);
    struct _rdil_operand * zero    = rdil_operand_create(RDIL_CONSTANT, lhs->bits, 0);
    struct _rdil_operand * SFxorOF = rdil_operand_create(RDIL_VAR, 32, -1);

    struct _rdil_ins * ins[6];
    ins[0] = rdil_ins_create(RDIL_ADD,    address, tmp,     lhs,     rhs);
    ins[1] = rdil_ins_create(RDIL_CMPLTU, address, CF,      tmp,     zero);
    ins[2] = rdil_ins_create(RDIL_CMPEQ,  address, ZF,      tmp,     zero);
    ins[3] = rdil_ins_create(RDIL_CMPLTS, address, SF,      tmp,     zero);
    ins[4] = rdil_ins_create(RDIL_CMPLTS, address, SFxorOF, lhs,     rhs);
    ins[5] = rdil_ins_create(RDIL_XOR,    address, OF,      SFxorOF, SF);

    int i;
    for (i = 0; i < 6; i++) {
        list_append(list, ins[i]);
        object_delete(ins[i]);
    }

    objects_delete(lhs, rhs, tmp, OF, CF, ZF, SF, zero, SFxorOF, NULL);

    rdil_x86_operand_set(ud_obj, 0, address, tmp, list);

    return list;
}



struct _list * rdil_x86_mov (ud_t * ud_obj, uint64_t address)
{
    struct _list * list = list_create();

    struct _rdil_operand * src = rdil_x86_operand_get(ud_obj, 1, address, list);

    rdil_x86_operand_set(ud_obj, 0, address, src, list);

    object_delete(src);

    return list;
}



struct _list * rdil_x86_push (ud_t * ud_obj, uint64_t address)
{
    struct _list * list = list_create();
    
    struct _rdil_operand * src     = rdil_x86_operand_get(ud_obj, 0, address, list);
    struct _rdil_operand * esp     = rdil_operand_create(RDIL_VAR, 32, RDIL_ESP);
    struct _rdil_operand * subsize = rdil_operand_create(RDIL_CONSTANT, 8, 4);
    struct _rdil_operand * sext    = rdil_operand_create(RDIL_VAR, 32, -1);

    struct _rdil_ins * ins[3];
    ins[0] = rdil_ins_create(RDIL_SUB,   address, esp,  esp,  subsize);
    ins[1] = rdil_ins_create(RDIL_SEXT,  address, sext, src);
    ins[3] = rdil_ins_create(RDIL_STORE, address, esp,  sext, 32);

    int i;
    for (i = 0; i < 3; i++) {
        list_append(list, ins[i]);
        object_delete(ins[i]);
    }

    objects_delete(src, esp, subsize, sext, NULL);

    return list;
}
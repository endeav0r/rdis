#include "x86.h"


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

    redis_x86->object = redis_x86_object;
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
                                 struct _map * mem_map);
{
    object_delete(redis_x86->mem);
    redis_x86->mem = object_copy(mem_map);
}


int redis_x86_step (struct _redis_x86 * redis_x86)
{
    uint8_t ins_mem[16];
    size_t  ins_mem_size;

    uint64_t         buf_addr = map_fetch_max_key(ins_mem, redis_x86->RED_EIP);
    struct _buffer * buf      = map_fetch_max(ins_mem, redis_x86->RED_EIP);

    if (buf == NULL)
        return REDIS_INVALID_IP;

    if (buf->size - redis_x86->RED_EIP < 16)
        ins_mem_size = buf->size - redis_x86->RED_EIP;
    else
        ins_mem_size = 16;

    memcpy(ins_mem, buf->bytes[redis_x86->RED_EIP - buf_addr], ins_mem_size);

    ud_t ud_obj;
    ud_init            (&ud_obj);
    ud_set_mode        (&ud_obj, 32);
    ud_set_syntax      (&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, &ins_mme, ins_mem_size);

    if (ud_disassemble(&ud_obj) != 0) {
        switch (ud_obj.mnemonic) {
        default :
            fprintf(stderr, "invalid/unsupported x86 ins %s\n",
                    ud_insn_asm(&ud_obj));
            return REDIS_INVALID_INSTRUCTION;
        }
    }

    return REDIS_SUCCESS;
}
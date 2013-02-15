#ifndef redis_mem_HEADER
#define redis_mem_HEADER

#include <inttypes.h>

struct _re_mems {
    uint32_t addr;
    size_t   size;
    uint8_t  bytes;
    struct _re_mems * next;
};


struct _re_mem {
    struct _re_mems * mems [256];
    unsigned char buf[256];
};


struct _re_mem *  re_mem_create ();
void              re_mem_delete (struct _re_mem * mem);

void      re_mem_insert (struct _re_mem * mem, struct _re_mems * mems);
uint8_t * re_mem_fetch  (struct _re_mem * mem, uint32_t address, size_t * size);
int       re_mem_set    (struct _re_mem * mem, uint32_t address, uint8_t byte);

struct _re_mems * re_mems_create (uint32_t addr, uint8_t * bytes, size_t size);
void              re_mems_delete (struct _re_mems * mems);

uint8_t re_mem_hash_address32 (uint32_t address);

#endif
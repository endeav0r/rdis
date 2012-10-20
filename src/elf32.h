#ifndef elf32_HEADER
#define elf32_HEADER

#include <elf.h>
#include <inttypes.h>
#include <stdio.h>

#include "graph.h"
#include "list.h"
#include "loader.h"
#include "tree.h"

struct _elf32 {
    const struct _loader_object * loader_object;
    union {
        uint8_t    * data;  
        Elf32_Ehdr * ehdr;
    };
    size_t data_size;
};

struct _elf32 * elf32_create  (const char * filename);
void            elf32_delete  (struct _elf32 * elf32);

uint64_t        elf32_entry         (struct _elf32 * elf32);
int             elf32_memory        (struct _elf32 * elf32, uint64_t address);
struct _graph * elf32_graph         (struct _elf32 * elf32);
struct _tree  * elf32_function_tree (struct _elf32 * elf32);

struct _list *  elf32_memory_segments (struct _elf32 * elf32);


// internal use
Elf32_Phdr *    elf32_phdr            (struct _elf32 * elf32, size_t index);
uint64_t        elf32_vaddr_to_offset (struct _elf32 * elf32, uint64_t address);


#endif
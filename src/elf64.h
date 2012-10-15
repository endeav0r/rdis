#ifndef elf64_HEADER
#define elf64_HEADER

#include <elf.h>
#include <inttypes.h>
#include <stdio.h>

#include "list.h"
#include "graph.h"

struct _elf64 {
    union {
        uint8_t    * data;  
        Elf64_Ehdr * ehdr;
    };
    size_t data_size;
};

struct _elf64 * elf64_create  (char * filename);
void            elf64_delete (struct _elf64 * elf64);

uint64_t        elf64_entry         (struct _elf64 * elf64);
int             elf64_memory        (struct _elf64 * elf64, uint64_t address);
struct _graph * elf64_graph         (struct _elf64 * elf64);
struct _tree  * elf64_function_tree (struct _elf64 * elf64);

struct _list *  elf64_memory_segments (struct _elf64 * elf64);


// internal use
Elf64_Phdr *    elf64_phdr (struct _elf64 * elf64, size_t index);
uint64_t        elf64_vaddr_to_offset (struct _elf64 * elf64, uint64_t address);


#endif
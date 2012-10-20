#ifndef elf64_HEADER
#define elf64_HEADER

#include <elf.h>
#include <inttypes.h>
#include <stdio.h>

#include "list.h"
#include "loader.h"
#include "graph.h"
#include "tree.h"

struct _elf64 {
    const struct _loader_object * loader_object;
    union {
        uint8_t    * data;  
        Elf64_Ehdr * ehdr;
    };
    size_t data_size;
};

struct _elf64 * elf64_create  (const char * filename);
void            elf64_delete  (struct _elf64 * elf64);

uint64_t        elf64_entry         (struct _elf64 * elf64);
struct _graph * elf64_graph         (struct _elf64 * elf64);
struct _tree  * elf64_function_tree (struct _elf64 * elf64);
struct _map   * elf64_labels        (struct _elf64 * elf64);

struct _list *  elf64_memory_segments (struct _elf64 * elf64);
int             elf64_memory        (struct _elf64 * elf64, uint64_t address);


// internal use
uint64_t        elf64_base_address    (struct _elf64 * elf64);
Elf64_Phdr *    elf64_phdr            (struct _elf64 * elf64, size_t index);
Elf64_Shdr *    elf64_shdr            (struct _elf64 * elf64, size_t index);
void *          elf64_section_element (struct _elf64 * elf64,
                                       size_t section,
                                       size_t index);
char *          elf64_strtab_str      (struct _elf64 * elf64,
                                       unsigned int strtab,
                                       unsigned int offset);
const char *    elf64_sym_name_by_address (struct _elf64 * elf64, uint64_t address);
Elf64_Shdr *    elf64_shdr_by_name    (struct _elf64 * elf64, const char * name);
struct _graph * elf64_dis_symtab      (struct _elf64 * elf64, int section);
uint64_t        elf64_vaddr_to_offset (struct _elf64 * elf64, uint64_t address);


#endif
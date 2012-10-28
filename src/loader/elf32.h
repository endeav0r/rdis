#ifndef elf32_HEADER
#define elf32_HEADER

#include <elf.h>
#include <inttypes.h>
#include <stdio.h>

#include "list.h"
#include "loader.h"
#include "graph.h"
#include "serialize.h"
#include "tree.h"

struct _elf32 {
    const struct _loader_object * loader_object;
    union {
        uint8_t    * data;  
        Elf32_Ehdr * ehdr;
    };
    size_t data_size;
};

struct _elf32 * elf32_create      (const char * filename);
void            elf32_delete      (struct _elf32 * elf32);
json_t *        elf32_serialize   (struct _elf32 * elf32);
struct _elf32 * elf32_deserialize (json_t * json);

uint64_t        elf32_entry         (struct _elf32 * elf32);
struct _graph * elf32_graph         (struct _elf32 * elf32);
struct _tree  * elf32_function_tree (struct _elf32 * elf32);
struct _map   * elf32_labels        (struct _elf32 * elf32);
struct _graph * elf32_graph_address (struct _elf32 * elf32, uint64_t address);
struct _map   * elf32_memory_map    (struct _elf32 * elf32);


// internal use
uint64_t        elf32_base_address    (struct _elf32 * elf32);
Elf32_Phdr *    elf32_phdr            (struct _elf32 * elf32, size_t index);
Elf32_Shdr *    elf32_shdr            (struct _elf32 * elf32, size_t index);
void *          elf32_section_element (struct _elf32 * elf32,
                                       size_t section,
                                       size_t index);
char *          elf32_strtab_str      (struct _elf32 * elf32,
                                       unsigned int strtab,
                                       unsigned int offset);
const char *    elf32_sym_name_by_address (struct _elf32 * elf32, uint64_t address);
Elf32_Shdr *    elf32_shdr_by_name    (struct _elf32 * elf32, const char * name);
struct _graph * elf32_dis_symtab      (struct _elf32 * elf32, int section);
uint64_t        elf32_vaddr_to_offset (struct _elf32 * elf32, uint64_t address);

#endif
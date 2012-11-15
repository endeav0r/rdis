#ifndef elf64_HEADER
#define elf64_HEADER

#include <elf.h>
#include <inttypes.h>
#include <stdio.h>

#include "list.h"
#include "loader.h"
#include "graph.h"
#include "serialize.h"
#include "tree.h"

struct _elf64 {
    const struct _loader_object * loader_object;
    union {
        uint8_t    * data;  
        Elf64_Ehdr * ehdr;
    };
    size_t data_size;
};

struct _elf64 * elf64_create      (const char * filename);
void            elf64_delete      (struct _elf64 * elf64);
json_t *        elf64_serialize   (struct _elf64 * elf64);
struct _elf64 * elf64_deserialize (json_t * json);

uint64_t        elf64_entry         (struct _elf64 * elf64);
struct _graph * elf64_graph         (struct _elf64 * elf64);
struct _map   * elf64_functions     (struct _elf64 * elf64);
struct _map   * elf64_labels        (struct _elf64 * elf64);
struct _graph * elf64_graph_address (struct _elf64 * elf64, uint64_t address);
struct _map   * elf64_memory_map    (struct _elf64 * elf64);
struct _label * elf64_label_address (struct _elf64 * elf64, uint64_t address);

struct _map *  elf64_function_address (struct _elf64 * elf64, uint64_t address);

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
Elf64_Shdr *    elf64_shdr_by_name    (struct _elf64 * elf64, const char * name);
struct _graph * elf64_dis_symtab      (struct _elf64 * elf64, int section);
uint64_t        elf64_vaddr_to_offset (struct _elf64 * elf64, uint64_t address);
const char *    elf64_sym_name_by_address (struct _elf64 * elf64, uint64_t address);
const char *    elf64_rel_name_by_address (struct _elf64 * elf64, uint64_t address);

#endif
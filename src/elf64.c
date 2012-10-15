#include "elf64.h"

#include "util.h"
#include "x8664.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <udis86.h>

struct _elf64 * elf64_create (char * filename)
{
    FILE * fh;
    size_t filesize;
    struct _elf64 * elf64;

    fh = fopen(filename, "rb");
    if (fh == NULL)
        return NULL;

    fseek(fh, 0, SEEK_END);
    filesize = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    elf64 = (struct _elf64 *) malloc(sizeof(struct _elf64));
    elf64->data = malloc(filesize);

    elf64->data_size = fread(elf64->data, 1, filesize, fh);

    fclose(fh);

    return elf64;
}



void elf64_delete (struct _elf64 * elf64)
{
    free(elf64->data);
    free(elf64);
}



uint64_t elf64_entry (struct _elf64 * elf64)
{
    return elf64->ehdr->e_entry;
}



uint64_t elf64_base_address (struct _elf64 * elf64)
{
    int phdr_i;

    for (phdr_i = 0; phdr_i < elf64->ehdr->e_phnum; phdr_i++) {
        Elf64_Phdr * phdr = elf64_phdr(elf64, phdr_i);
        if (phdr->p_offset == 0)
            return phdr->p_vaddr;
    }

    return 0;
}



Elf64_Phdr * elf64_phdr (struct _elf64 * elf64, size_t index)
{
    return (Elf64_Phdr *) &(elf64->data[elf64->ehdr->e_phoff
                                        + (index * elf64->ehdr->e_phentsize)]);
}



Elf64_Shdr * elf64_shdr (struct _elf64 * elf64, size_t index)
{
    return (Elf64_Shdr *) &(elf64->data[elf64->ehdr->e_shoff
                                        + (index * elf64->ehdr->e_shentsize)]);
}



void * elf64_section_element (struct _elf64 * elf64,
                              size_t section,
                              size_t index)
{
    Elf64_Shdr * shdr = elf64_shdr(elf64, section);
    return (Elf64_Sym *) &(elf64->data[shdr->sh_offset
                                       + (index * shdr->sh_entsize)]);
}



char * elf64_strtab_str (struct _elf64 * elf64,
                         unsigned int strtab,
                         unsigned int offset)
{
    Elf64_Shdr * shdr = elf64_shdr(elf64, strtab);
    return (char *) &(elf64->data[shdr->sh_offset + offset]);
}



uint64_t elf64_vaddr_to_offset (struct _elf64 * elf64, uint64_t address)
{
    Elf64_Phdr * phdr;
    int i;
    int64_t result;

    for (i = 0; i < elf64->ehdr->e_phnum; i++) {
        phdr = elf64_phdr(elf64, i);
        if (    (phdr->p_vaddr <= address)
             && (phdr->p_vaddr + phdr->p_filesz >= address)) {
            result = address - phdr->p_vaddr;
            result += phdr->p_offset;
            return result;
        }
    }

    return -1;
}



struct _graph * elf64_dis_symtab (struct _elf64 * elf64, int section)
{
    Elf64_Shdr * shdr = elf64_shdr(elf64, section);
    int sym_i;

    struct _graph * graph;
    struct _graph * sym_graph;

    graph = graph_create();

    for (sym_i = 0; sym_i < shdr->sh_size / shdr->sh_entsize; sym_i++) {
        Elf64_Sym * sym = elf64_section_element(elf64, section, sym_i);
        if (ELF64_ST_TYPE(sym->st_info) == STT_FUNC) {
            if (sym->st_value == 0)
                continue;
            uint64_t offset = elf64_vaddr_to_offset(elf64, sym->st_value);
            if (offset == -1)
                continue;
            /*
            if (strcmp("tree_node_insert", elf64_strtab_str(elf64, shdr->sh_link, sym->st_name)))
                continue;
            */
            sym_graph = x8664_graph(elf64_base_address(elf64),
                                    sym->st_value - elf64_base_address(elf64),
                                    elf64->data,
                                    elf64->data_size);
            graph_merge(graph, sym_graph);
            graph_delete(sym_graph);
        }
    }

    return graph;
}



struct _graph * elf64_graph (struct _elf64 * elf64)
{
    struct _graph * graph;

    // disassemble from entry point
    graph = x8664_graph(elf64_base_address(elf64),
                        elf64_entry(elf64) - elf64_base_address(elf64),
                        elf64->data,
                        elf64->data_size);

    // check for __libc_start_main loader
    struct _list * ins_list;
    ins_list = graph_fetch_data(graph, elf64_entry(elf64) + 0x1d);
    if (ins_list != NULL) {
        struct _ins * ins = list_first(ins_list);
        if (ins != NULL) {
            ud_t ud_obj;
            ud_init      (&ud_obj);
            ud_set_mode  (&ud_obj, 64);
            ud_set_input_buffer(&ud_obj, ins->bytes, ins->size);
            ud_disassemble(&ud_obj);
            if (    (ud_obj.mnemonic == UD_Imov)
                 && (ud_obj.operand[0].base == UD_R_RDI)) {
                struct _graph * main_graph;
                main_graph = x8664_graph(elf64_base_address(elf64),
                                         udis86_sign_extend_lval(&(ud_obj.operand[1]))
                                          - elf64_base_address(elf64),
                                         elf64->data,
                                         elf64->data_size);
                graph_merge(graph, main_graph);
                graph_delete(main_graph);
            }
        }
    }

    // find symtab sections
    int si;
    for (si = 0; si < elf64->ehdr->e_shnum; si++) {
        Elf64_Shdr * shdr = elf64_shdr(elf64, si);
        if (shdr->sh_type == SHT_SYMTAB) {
            struct _graph * sym_graph = elf64_dis_symtab(elf64, si);
            graph_merge(graph, sym_graph);
            graph_delete(sym_graph);
        }
    }

    return graph;
}



struct _tree * elf64_function_tree (struct _elf64 * elf64)
{
    struct _tree     * tree = tree_create();
    struct _function * function;

    int sec_i;
    // symbols are easy
    for (sec_i = 0; sec_i < elf64->ehdr->e_shnum; sec_i++) {
        Elf64_Shdr * shdr = elf64_shdr(elf64, sec_i);
        if (shdr->sh_type != SHT_SYMTAB)
            continue;

        int sym_i;
        for (sym_i = 0; sym_i < shdr->sh_size / shdr->sh_entsize; sym_i++) {
            Elf64_Sym * sym = elf64_section_element(elf64, sec_i, sym_i);
            if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC)
                continue;

            if (sym->st_value == 0)
                continue;

            char * name = elf64_strtab_str(elf64, shdr->sh_link, sym->st_name);
            printf("found function %s at %llx\n",
                   name, (unsigned long long) sym->st_value);
            function = function_create(sym->st_value, name);
            if (tree_fetch(tree, function) == NULL)
                tree_insert(tree, function);
                
            object_delete(function);
        }
    }

    return tree;
}



int elf64_memory (struct _elf64 * elf64, uint64_t address)
{
    uint64_t offset = elf64_vaddr_to_offset(elf64, address);
    if (offset == -1)
        return -1;
    if (offset > elf64->data_size)
        return -1;

    return elf64->data[offset];
}



struct _memory_segment * elf64_memory_segments_contains (struct _list * list,
                                                  Elf64_Phdr * phdr)
{
    struct _list_it * it;
    struct _memory_segment * mem_seg;

    for (it = list_iterator(list); it != NULL; it = it->next) {
        mem_seg = (struct _memory_segment *) it->data;
        if (    (mem_seg->address <= phdr->p_vaddr)
             && (mem_seg->address + mem_seg->size >= phdr->p_vaddr))
            return it->data;
        else if (    (phdr->p_vaddr <= mem_seg->address)
                  && (phdr->p_vaddr + phdr->p_memsz >= mem_seg->address))
            return it->data;
    }
    return NULL;
}



struct _list * elf64_memory_segments (struct _elf64 * elf64)
{
    struct _list * list = list_create();
    Elf64_Phdr   * phdr;
    int            phdr_i;
    struct _memory_segment * mem_seg_ptr;
    struct _memory_segment mem_seg;

    for (phdr_i = 0; phdr_i < elf64->ehdr->e_phnum; phdr_i++) {
        phdr = elf64_phdr(elf64, phdr_i);
        if (phdr->p_memsz == 0)
            continue;
        // if this phdr falls within the bounds of an existing memory segment
        mem_seg_ptr = elf64_memory_segments_contains(list, phdr);
        if (mem_seg_ptr != NULL) {
            // create a new mem_seg with correct address and size
            if (phdr->p_vaddr < mem_seg_ptr->address)
                mem_seg.address = phdr->p_vaddr;
            else
                mem_seg.address = mem_seg_ptr->address;

            if (mem_seg_ptr->address + mem_seg_ptr->size
                > phdr->p_vaddr + phdr->p_memsz)
                mem_seg.size = mem_seg_ptr->address 
                               + mem_seg_ptr->size
                               - mem_seg.address;
            else
                mem_seg.size = phdr->p_vaddr
                               + phdr->p_memsz
                               - mem_seg.address;
            // copy over correct values
            mem_seg_ptr->address = mem_seg.address;
            mem_seg_ptr->size    = mem_seg.size;
        }
        else {
            mem_seg.address = phdr->p_vaddr;
            mem_seg.size    = phdr->p_memsz;
            //list_append(list, &mem_seg, sizeof(struct _memory_segment));
        }
    }

    return list;
}
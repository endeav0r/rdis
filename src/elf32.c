#include "elf32.h"

#include "index.h"
#include "label.h"
#include "queue.h"
#include "tree.h"
#include "util.h"
#include "wqueue.h"
#include "x86.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <udis86.h>


static const struct _loader_object elf32_loader_object = {
    {
        (void            (*) (void *)) elf32_delete,
        NULL,
        NULL,
        NULL,
    },
    (uint64_t        (*) (void *)) elf32_entry,
    (struct _graph * (*) (void *)) elf32_graph,
    (struct _tree *  (*) (void *)) elf32_function_tree,
    (struct _map  *  (*) (void *)) elf32_labels
};


struct _elf32 * elf32_create (const char * filename)
{
    FILE * fh;
    size_t filesize;
    struct _elf32 * elf32;

    fh = fopen(filename, "rb");
    if (fh == NULL)
        return NULL;

    fseek(fh, 0, SEEK_END);
    filesize = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    elf32 = (struct _elf32 *) malloc(sizeof(struct _elf32));
    elf32->data = malloc(filesize);
    elf32->loader_object = &elf32_loader_object;

    elf32->data_size = fread(elf32->data, 1, filesize, fh);

    fclose(fh);

    // make sure this is a 32-bit ELF
    if (    (elf32->data_size < 0x200)
         || (elf32->ehdr->e_ident[EI_MAG0]  != ELFMAG0)
         || (elf32->ehdr->e_ident[EI_MAG1]  != ELFMAG1)
         || (elf32->ehdr->e_ident[EI_MAG2]  != ELFMAG2)
         || (elf32->ehdr->e_ident[EI_MAG3]  != ELFMAG3)
         || (elf32->ehdr->e_ident[EI_CLASS] != ELFCLASS32)) {
        elf32_delete(elf32);
        return NULL;       
    }

    return elf32;
}



void elf32_delete (struct _elf32 * elf32)
{
    free(elf32->data);
    free(elf32);
}



uint64_t elf32_entry (struct _elf32 * elf32)
{
    return elf32->ehdr->e_entry;
}



uint64_t elf32_base_address (struct _elf32 * elf32)
{
    int phdr_i;

    for (phdr_i = 0; phdr_i < elf32->ehdr->e_phnum; phdr_i++) {
        Elf32_Phdr * phdr = elf32_phdr(elf32, phdr_i);
        if (phdr->p_offset == 0)
            return phdr->p_vaddr;
    }

    return 0;
}



Elf32_Phdr * elf32_phdr (struct _elf32 * elf32, size_t index)
{
    return (Elf32_Phdr *) &(elf32->data[elf32->ehdr->e_phoff
                                        + (index * elf32->ehdr->e_phentsize)]);
}



Elf32_Shdr * elf32_shdr (struct _elf32 * elf32, size_t index)
{
    return (Elf32_Shdr *) &(elf32->data[elf32->ehdr->e_shoff
                                        + (index * elf32->ehdr->e_shentsize)]);
}



void * elf32_section_element (struct _elf32 * elf32,
                              size_t section,
                              size_t index)
{
    Elf32_Shdr * shdr = elf32_shdr(elf32, section);
    return (Elf32_Sym *) &(elf32->data[shdr->sh_offset
                                       + (index * shdr->sh_entsize)]);
}



char * elf32_strtab_str (struct _elf32 * elf32,
                         unsigned int strtab,
                         unsigned int offset)
{
    Elf32_Shdr * shdr = elf32_shdr(elf32, strtab);
    return (char *) &(elf32->data[shdr->sh_offset + offset]);
}


const char * elf32_sym_name_by_address (struct _elf32 * elf32, uint64_t address)
{
    int shdr_i;
    for (shdr_i = 0; shdr_i < elf32->ehdr->e_shnum; shdr_i++) {
        Elf32_Shdr * shdr = elf32_shdr(elf32, shdr_i);
        if (shdr->sh_type != SHT_SYMTAB)
            continue;

        int sym_i;
        for (sym_i = 0; sym_i < shdr->sh_size / shdr->sh_entsize; sym_i++) {
            Elf32_Sym * sym = elf32_section_element(elf32, shdr_i, sym_i);
            if (sym->st_value != address)
                continue;
            // found matching symbol
            return elf32_strtab_str(elf32, shdr->sh_link, sym->st_name);
        }
    }

    return NULL;
}



Elf32_Shdr * elf32_shdr_by_name (struct _elf32 * elf32, const char * name)
{
    int i;
    Elf32_Shdr * shdr;

    for (i = 0; i < elf32->ehdr->e_shnum; i++) {
        shdr = elf32_shdr(elf32, i);
        const char * shdr_name;
        shdr_name = elf32_strtab_str(elf32, 
                                     elf32->ehdr->e_shstrndx,
                                     shdr->sh_name);
        if (strcmp(name, shdr_name) == 0)
            return shdr;
    }

    return NULL;
}



uint64_t elf32_vaddr_to_offset (struct _elf32 * elf32, uint64_t address)
{
    Elf32_Phdr * phdr;
    int i;
    int64_t result;

    for (i = 0; i < elf32->ehdr->e_phnum; i++) {
        phdr = elf32_phdr(elf32, i);
        if (    (phdr->p_vaddr <= address)
             && (phdr->p_vaddr + phdr->p_filesz >= address)) {
            result = address - phdr->p_vaddr;
            result += phdr->p_offset;
            return result;
        }
    }

    return -1;
}



struct _graph * elf32_graph (struct _elf32 * elf32)
{

    struct _graph  * graph;
    struct _wqueue * wqueue;

    // disassemble from entry point
    graph = x86_graph(elf32_base_address(elf32),
                      elf32_entry(elf32) - elf32_base_address(elf32),
                      elf32->data,
                      elf32->data_size);

    // disassemble all functions from elf64_function_tree
    struct _tree * function_tree = elf32_function_tree(elf32);
    struct _tree_it * tit;

    wqueue = wqueue_create();
    for (tit = tree_iterator(function_tree);
         tit != NULL;
         tit  = tree_it_next(tit)) {
        struct _index * index = tree_it_data(tit);

        printf("graphing %llx\n", (unsigned long long) index->index);

        struct _x86_graph_wqueue * xgw;
        xgw = x86_graph_wqueue_create(elf32_base_address(elf32),
                                        index->index - elf32_base_address(elf32),
                                        elf32->data,
                                        elf32->data_size);
        wqueue_push(wqueue, WQUEUE_CALLBACK(x86_graph_wqueue), xgw);
        object_delete(xgw);
    }

    wqueue_wait(wqueue);

    while (wqueue_peek(wqueue) != NULL) {
        graph_merge(graph, wqueue_peek(wqueue));
        wqueue_pop(wqueue);
    }

    object_delete(wqueue);

    remove_function_predecessors(graph, function_tree);
    object_delete(function_tree);

    return graph;
}



struct _tree * elf32_function_tree (struct _elf32 * elf32)
{
    struct _tree     * tree = tree_create();

    // add the entry point
    struct _index * index = index_create(elf32_entry(elf32));
    tree_insert(tree, index);
    object_delete(index);

    // recursively disassemble from entry point
    struct _tree * recursive_function_tree;
    recursive_function_tree = x86_functions(elf32_base_address(elf32),
                                            elf32_entry(elf32)
                                            - elf32_base_address(elf32),
                                            elf32->data,
                                            elf32->data_size);

    struct _tree_it * it;
    for (it = tree_iterator(recursive_function_tree);
         it != NULL;
         it = tree_it_next(it)) {
        struct _index * index = tree_it_data(it);
        if (tree_fetch(tree, index) == NULL)
            tree_insert(tree, index);
    }

    object_delete(recursive_function_tree);

    // symbols are easy
    int sec_i;
    for (sec_i = 0; sec_i < elf32->ehdr->e_shnum; sec_i++) {
        Elf32_Shdr * shdr = elf32_shdr(elf32, sec_i);
        if (shdr->sh_type != SHT_SYMTAB)
            continue;

        int sym_i;
        for (sym_i = 0; sym_i < shdr->sh_size / shdr->sh_entsize; sym_i++) {
            Elf32_Sym * sym = elf32_section_element(elf32, sec_i, sym_i);
            if (ELF32_ST_TYPE(sym->st_info) != STT_FUNC)
                continue;

            if (sym->st_value == 0)
                continue;

            struct _index * index = index_create(sym->st_value);
            if (tree_fetch(tree, index) == NULL)
                tree_insert(tree, index);
                
            object_delete(index);
        }
    }

    // check for __libc_start_main loader
    uint64_t target_offset = elf32_entry(elf32) - elf32_base_address(elf32) + 0x17;
    if (target_offset + 0x10 > elf32->data_size)
        return tree;

    uint8_t * data = &(elf32->data[target_offset]);
    size_t    size = elf32->data_size - target_offset;

    ud_t ud_obj;
    ud_init      (&ud_obj);
    ud_set_mode  (&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, data, size);
    ud_disassemble(&ud_obj);
    if (ud_obj.mnemonic == UD_Ipush) {
        printf("found __libc_start_main loader, main at %llx\n",
           (unsigned long long) udis86_sign_extend_lval(&(ud_obj.operand[0])));

        // add main to function tree
        struct _index * index;
        index = index_create(udis86_sign_extend_lval(&(ud_obj.operand[0])));
        if (tree_fetch(tree, index) == NULL)
            tree_insert(tree, index);
        object_delete(index);

        struct _tree * recursive_function_tree;
        recursive_function_tree = x86_functions(elf32_base_address(elf32),
                                  udis86_sign_extend_lval(&(ud_obj.operand[0]))
                                   - elf32_base_address(elf32),
                                                  elf32->data,
                                                  elf32->data_size);
        struct _tree_it * it;
        for (it = tree_iterator(recursive_function_tree);
             it != NULL;
             it = tree_it_next(it)) {
            struct _index * index = tree_it_data(it);
            if (tree_fetch(tree, index) == NULL)
                tree_insert(tree, index);
        }
        object_delete(recursive_function_tree);
    }
    else
        printf("disassembled: %s\n disassembled at %llx\n",
               ud_insn_asm(&ud_obj),
               (unsigned long long) target_offset);

    return tree;
}



struct _map * elf32_labels (struct _elf32 * elf32)
{
    // start by getting an address of all the functions
    struct _tree * function_tree = elf32_function_tree(elf32);

    struct _map * labels_map = map_create();

    // set labels for relocations
    int shdr_i;
    for (shdr_i = 0; shdr_i < elf32->ehdr->e_shnum; shdr_i++) {
        Elf32_Shdr * shdr = elf32_shdr(elf32, shdr_i);
        if (shdr->sh_type != SHT_REL)
            continue;
        Elf32_Shdr * shdr_sym = elf32_shdr(elf32, shdr->sh_link);
        int rel_i;
        for (rel_i = 0; rel_i < shdr->sh_size / shdr->sh_entsize; rel_i++) {
            Elf32_Rel * rel = elf32_section_element(elf32, shdr_i, rel_i);
            if (ELF32_R_SYM(rel->r_info) == STN_UNDEF)
                continue;

            if (map_fetch(labels_map, rel->r_offset) != NULL)
                continue;

            Elf32_Sym * sym = elf32_section_element(elf32,
                                                    shdr->sh_link,
                                                    ELF32_R_SYM(rel->r_info));

            struct _label * label;
            // rela->r_offset needs to be fixed for relocatable objects
            const char * name = elf32_strtab_str(elf32,
                                                 shdr_sym->sh_link,
                                                 sym->st_name);
            label = label_create(rel->r_offset, name, LABEL_NONE);
            map_insert(labels_map, rel->r_offset, label);
            object_delete(label);
        }
    }

    // remove edges into the PLT
    Elf32_Shdr * plt_shdr = elf32_shdr_by_name(elf32, ".plt");
    uint64_t plt_bottom;
    uint64_t plt_top;

    if (plt_shdr == NULL) {
        plt_bottom = -1;
        plt_top = -1;
    }
    else {
        plt_bottom = plt_shdr->sh_addr;
        plt_top    = plt_bottom + plt_shdr->sh_size;
    }
    
    
    // look through all functions in function tree and add a label for each
    struct _tree_it * it;
    for (it = tree_iterator(function_tree); it != NULL; it = tree_it_next(it)) {
        struct _index * index = tree_it_data(it);

        // plt functions are a special case, as we try to identify their targets
        // in the got
        if (    (index->index >= plt_bottom)
             && (index->index <  plt_top)) {
            uint8_t * data      = &(elf32->data[index->index 
                                  - elf32_base_address(elf32)]);
            ud_t ud_obj;
            ud_init(&ud_obj);
            ud_set_mode  (&ud_obj, 32);
            ud_set_syntax(&ud_obj, UD_SYN_INTEL);
            ud_set_input_buffer(&ud_obj, data, 0x20);
            ud_disassemble(&ud_obj);

            if (    (ud_obj.mnemonic == UD_Ijmp)
                 && (udis86_sign_extend_lval(&(ud_obj.operand[0])) != -1)) {
                uint64_t target = udis86_sign_extend_lval(&(ud_obj.operand[0]));

                struct _label * label = map_fetch(labels_map, target);
                if (label != NULL) {
                    char plttmp[256];
                    snprintf(plttmp, 256, "%s@plt", label->text);
                    label = label_create(index->index, plttmp, LABEL_FUNCTION);
                    map_insert(labels_map, index->index, label);
                    object_delete(label);
                    continue;
                }
            }
            else {
                printf("disassembled: %s\n", ud_insn_asm(&ud_obj));
                if (ud_obj.operand[0].type == UD_OP_MEM)
                    printf("UD_OP_MEM\n");
                printf("%llx\n",
                       (unsigned long long) udis86_sign_extend_lval(&(ud_obj.operand[0])));
            }
        }

        const char * name = elf32_sym_name_by_address(elf32, index->index);
        if (name == NULL) {
            char tmp[128];
            snprintf(tmp, 128, "fun_%llx", (unsigned long long) index->index);
            name = tmp;
        }

        struct _label * label = label_create(index->index, name, LABEL_FUNCTION);
        map_insert(labels_map, index->index, label);
        object_delete(label);
    }

    object_delete(function_tree);
    return labels_map;
}
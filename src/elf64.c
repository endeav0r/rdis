#include "elf64.h"

#include "index.h"
#include "label.h"
#include "queue.h"
#include "tree.h"
#include "util.h"
#include "wqueue.h"
#include "x8664.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <udis86.h>


static const struct _loader_object elf64_object = {
    {
        (void            (*) (void *)) elf64_delete, 
        NULL,
        NULL,
        NULL,
    },
    (uint64_t        (*) (void *)) elf64_entry,
    (struct _graph * (*) (void *)) elf64_graph,
    (struct _tree *  (*) (void *)) elf64_function_tree,
    (struct _map  *  (*) (void *)) elf64_labels
};


struct _elf64 * elf64_create (const char * filename)
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
    elf64->loader_object = &elf64_object;

    elf64->data_size = fread(elf64->data, 1, filesize, fh);

    fclose(fh);

    // make sure this is a 64-bit ELF
    if (    (elf64->data_size < 0x200)
         || (elf64->ehdr->e_ident[EI_MAG0] != ELFMAG0)
         || (elf64->ehdr->e_ident[EI_MAG1] != ELFMAG1)
         || (elf64->ehdr->e_ident[EI_MAG2] != ELFMAG2)
         || (elf64->ehdr->e_ident[EI_MAG3] != ELFMAG3)
         || (elf64->ehdr->e_ident[EI_CLASS] != ELFCLASS64)) {
        elf64_delete(elf64);
        return NULL;       
    }
    
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


const char * elf64_sym_name_by_address (struct _elf64 * elf64, uint64_t address)
{
    int shdr_i;
    for (shdr_i = 0; shdr_i < elf64->ehdr->e_shnum; shdr_i++) {
        Elf64_Shdr * shdr = elf64_shdr(elf64, shdr_i);
        if (shdr->sh_type != SHT_SYMTAB)
            continue;

        int sym_i;
        for (sym_i = 0; sym_i < shdr->sh_size / shdr->sh_entsize; sym_i++) {
            Elf64_Sym * sym = elf64_section_element(elf64, shdr_i, sym_i);
            if (sym->st_value != address)
                continue;
            // no section symbols
            if (ELF64_ST_TYPE(sym->st_info) == STT_SECTION)
                continue;
            // found matching symbol
            return elf64_strtab_str(elf64, shdr->sh_link, sym->st_name);
        }
    }

    return NULL;
}



Elf64_Shdr * elf64_shdr_by_name (struct _elf64 * elf64, const char * name)
{
    int i;
    Elf64_Shdr * shdr;

    for (i = 0; i < elf64->ehdr->e_shnum; i++) {
        shdr = elf64_shdr(elf64, i);
        const char * shdr_name;
        shdr_name = elf64_strtab_str(elf64, 
                                     elf64->ehdr->e_shstrndx,
                                     shdr->sh_name);
        if (strcmp(name, shdr_name) == 0)
            return shdr;
    }

    return NULL;
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



struct _graph * elf64_graph (struct _elf64 * elf64)
{
    struct _graph  * graph;
    struct _wqueue * wqueue;

    // disassemble from entry point
    graph = x8664_graph(elf64_base_address(elf64),
                        elf64_entry(elf64) - elf64_base_address(elf64),
                        elf64->data,
                        elf64->data_size);

    // disassemble all functions from elf64_function_tree
    struct _tree * function_tree = elf64_function_tree(elf64);
    struct _tree_it * tit;

    wqueue = wqueue_create();
    for (tit = tree_iterator(function_tree);
         tit != NULL;
         tit  = tree_it_next(tit)) {
        struct _index * index = tree_it_data(tit);

        struct _x8664_graph_wqueue * xgw;
        xgw = x8664_graph_wqueue_create(elf64_base_address(elf64),
                                        index->index - elf64_base_address(elf64),
                                        elf64->data,
                                        elf64->data_size);
        wqueue_push(wqueue, WQUEUE_CALLBACK(x8664_graph_wqueue), xgw);
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


    // remove edges into the PLT
    /*
    Elf64_Shdr * plt_shdr = elf64_shdr_by_name(elf64, ".plt");
    if (plt_shdr == NULL)
        return graph;

    uint64_t plt_bottom = plt_shdr->sh_addr;
    uint64_t plt_top = plt_bottom + plt_shdr->sh_size;

    struct _queue * queue = queue_create();

    struct _graph_it * git;
    for (git = graph_iterator(graph); git != NULL; git = graph_it_next(git)) {
        struct _graph_node * node = graph_it_node(git);
        struct _list_it * eit;
        for (eit = list_iterator(node->edges); eit != NULL; eit = eit->next) {
            struct _graph_edge * edge = eit->data;
            if ((edge->tail < plt_top) && (edge->tail >= plt_bottom)) {
                queue_push(queue, edge);
            }
        }
    }

    while (queue->size > 0) {
        struct _graph_edge * edge = queue_peek(queue);
        graph_remove_edge(graph, edge->head, edge->tail);
        queue_pop(queue);
    }

    object_delete(queue);
    */

    return graph;
}



struct _tree * elf64_function_tree (struct _elf64 * elf64)
{
    struct _tree     * tree = tree_create();

    // add the entry point
    struct _index * index = index_create(elf64_entry(elf64));
    tree_insert(tree, index);
    object_delete(index);

    // recursively disassemble from entry point
    struct _tree * recursive_function_tree;
    recursive_function_tree = x8664_functions(elf64_base_address(elf64),
                                              elf64_entry(elf64)
                                              - elf64_base_address(elf64),
                                              elf64->data,
                                              elf64->data_size);

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

            struct _index * index = index_create(sym->st_value);
            if (tree_fetch(tree, index) == NULL)
                tree_insert(tree, index);
                
            object_delete(index);
        }
    }

    // check for __libc_start_main loader
    uint64_t target_offset = elf64_entry(elf64) - elf64_base_address(elf64) + 0x1d;
    if (target_offset + 0x10 > elf64->data_size)
        return tree;

    uint8_t * data = &(elf64->data[target_offset]);
    size_t    size = elf64->data_size - target_offset;

    ud_t ud_obj;
    ud_init      (&ud_obj);
    ud_set_mode  (&ud_obj, 64);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, data, size);
    ud_disassemble(&ud_obj);
    if (    (ud_obj.mnemonic == UD_Imov)
         && (ud_obj.operand[0].base == UD_R_RDI)) {
        printf("found __libc_start_main loader, main at %llx\n",
           (unsigned long long) udis86_sign_extend_lval(&(ud_obj.operand[1])));

        // add main to function tree
        struct _index * index;
        index = index_create(udis86_sign_extend_lval(&(ud_obj.operand[1])));
        if (tree_fetch(tree, index) == NULL)
            tree_insert(tree, index);
        object_delete(index);

        struct _tree * recursive_function_tree;
        recursive_function_tree = x8664_functions(elf64_base_address(elf64),
                                  udis86_sign_extend_lval(&(ud_obj.operand[1]))
                                   - elf64_base_address(elf64),
                                                  elf64->data,
                                                  elf64->data_size);
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



struct _map * elf64_labels (struct _elf64 * elf64)
{
    // start by getting an address of all the functions
    struct _tree * function_tree = elf64_function_tree(elf64);

    struct _map * labels_map = map_create();

    // set labels for relocations
    int shdr_i;
    for (shdr_i = 0; shdr_i < elf64->ehdr->e_shnum; shdr_i++) {
        Elf64_Shdr * shdr = elf64_shdr(elf64, shdr_i);
        if (shdr->sh_type != SHT_RELA)
            continue;
        Elf64_Shdr * shdr_sym = elf64_shdr(elf64, shdr->sh_link);
        int rela_i;
        for (rela_i = 0; rela_i < shdr->sh_size / shdr->sh_entsize; rela_i++) {
            Elf64_Rela * rela = elf64_section_element(elf64, shdr_i, rela_i);
            if (ELF64_R_SYM(rela->r_info) == STN_UNDEF)
                continue;

            if (map_fetch(labels_map, rela->r_offset) != NULL)
                continue;

            Elf64_Sym * sym = elf64_section_element(elf64,
                                                    shdr->sh_link,
                                                    ELF64_R_SYM(rela->r_info));

            struct _label * label;
            // rela->r_offset needs to be fixed for relocatable objects
            const char * name = elf64_strtab_str(elf64,
                                                 shdr_sym->sh_link,
                                                 sym->st_name);
            label = label_create(rela->r_offset, name, LABEL_NONE);
            map_insert(labels_map, rela->r_offset, label);
            object_delete(label);
        }
    }

    Elf64_Shdr * plt_shdr = elf64_shdr_by_name(elf64, ".plt");
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
            uint8_t * data      = &(elf64->data[index->index 
                                  - elf64_base_address(elf64)]);
            ud_t ud_obj;
            ud_init(&ud_obj);
            ud_set_mode  (&ud_obj, 64);
            ud_set_input_buffer(&ud_obj, data, 0x10);
            ud_disassemble(&ud_obj);

            if (    (ud_obj.mnemonic == UD_Ijmp)
                 && (udis86_rip_offset(index->index, &(ud_obj.operand[0])) != -1)) {
                uint64_t target = udis86_rip_offset(index->index,
                                                    &(ud_obj.operand[0]));
                target += ud_insn_len(&ud_obj);
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
        }

        const char * name = elf64_sym_name_by_address(elf64, index->index);
        if ((name == NULL) || (strcmp(name, "") == 0)) {
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
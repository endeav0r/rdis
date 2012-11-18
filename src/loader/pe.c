#include "pe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "label.h"
#include "util.h"
#include "x8664.h"
#include "x86.h"

static const struct _loader_object pe_object = {
    {
        (void     (*) (void *)) pe_delete, 
        NULL,
        NULL,
        NULL,
        (json_t * (*) (void *)) pe_serialize
    },
    (uint64_t        (*) (void *))                pe_entry,
    (struct _graph * (*) (void *))                pe_graph,
    (struct _map  *  (*) (void *))                pe_functions,
    (struct _map  *  (*) (void *))                pe_labels,
    (struct _graph * (*) (void *, uint64_t))      pe_graph_address,
    (struct _map *   (*) (void *))                pe_memory_map,
    (struct _map  *  (*) (void *, uint64_t))      pe_function_address,
    (struct _label * (*) (void *, uint64_t))      pe_label_address,
    (struct _graph * (*) (void *, struct _map *)) pe_graph_functions,
    (struct _map *   (*) (void *, struct _map *)) pe_labels_functions
};


struct _pe * pe_create (const char * filename)
{
    FILE * fh;
    size_t filesize;
    struct _pe * pe;

    fh = fopen(filename, "rb");
    if (fh == NULL)
        return NULL;

    fseek(fh, 0, SEEK_END);
    filesize = ftell(fh);
    fseek(fh, 0, SEEK_SET);

    pe = (struct _pe *) malloc(sizeof(struct _pe));
    pe->data = malloc(filesize);
    pe->loader_object = &pe_object;

    pe->data_size = fread(pe->data, 1, filesize, fh);

    fclose(fh);

    // make sure this is a PE
    printf("pe 1.");fflush(stdout);
    if (pe->data_size < PE_FILE_OFFSET + 4) {
        pe_delete(pe);
        return NULL;
    }

    uint32_t offset = *((uint32_t *) &(pe->data[PE_FILE_OFFSET]));

    printf("2.");fflush(stdout);
    printf("offset: %x pe->data_size: %x\n", offset, (int) pe->data_size);
    if (offset + 4 + sizeof(Pe_FileHeader) >= pe->data_size) {
        pe_delete(pe);
        return NULL;
    }

    printf("3.");fflush(stdout);
    printf("offset: %x, %x %x %x %x\n",
           offset,
           pe->data[offset],
           pe->data[offset+1],
           pe->data[offset+2],
           pe->data[offset+3]);fflush(stdout);
    if (    (pe->data[offset    ] != 'P')
         || (pe->data[offset + 1] != 'E')
         || (pe->data[offset + 2] != '\0')
         || (pe->data[offset + 3] != '\0')) {
        object_delete(pe);
        return NULL;
    }

    pe->offset = offset + 4;

    printf("4.");fflush(stdout);
    if (pe_plus(pe) == -1) {
        object_delete(pe);
        return NULL;
    }

    printf("5.");fflush(stdout);

    return pe;
}


void pe_delete (struct _pe * pe)
{
    if (pe->data != NULL)
        free(pe->data);

    free(pe);
}


struct _pe * pe_copy (struct _pe * pe)
{
    struct _pe * new_pe = (struct _pe *) malloc(sizeof(struct _pe));

    new_pe->loader_object = &pe_object;
    new_pe->data          = malloc(pe->data_size);
    memcpy(new_pe->data, pe->data, pe->data_size);
    new_pe->data_size = pe->data_size;

    return new_pe;
}


json_t * pe_serialize (struct _pe * pe)
{
    json_t * json = json_object();

    json_object_set(json, "ot", json_integer(SERIALIZE_PE));

    json_t * bytes = json_array();
    size_t i;
    for (i = 0; i < pe->data_size; i++) {
        json_array_append(bytes, json_integer(pe->data[i]));
    }

    json_object_set(json, "data", bytes);

    return json;
}


struct _pe * pe_deserialize (json_t * json)
{
    json_t * bytes = json_object_get(json, "bytes");

    if (! json_is_array(bytes)) {
        serialize_error = SERIALIZE_PE;
        return NULL;
    }

    struct _pe * pe   = (struct _pe *) malloc(sizeof(struct _pe));
    pe->loader_object = &pe_object;
    pe->data_size     = json_array_size(bytes);
    pe->data          = (uint8_t *) malloc(pe->data_size);

    size_t i;
    for (i = 0; i < json_array_size(bytes); i++) {
        json_t * c = json_array_get(bytes, i);
        if (! json_is_integer(c)) {
            serialize_error = SERIALIZE_PE;
            free(pe->data);
            free(pe);
            return NULL;
        }
        pe->data[i] = json_integer_value(c);
    }

    return pe;
}


Pe_FileHeader * pe_fh (struct _pe * pe)
{
    return (Pe_FileHeader *) &(pe->data[pe->offset]);
}


Pe_OptionalHeaderStandard * pe_ohs (struct _pe * pe)
{
    uint32_t offset = pe->offset + sizeof(Pe_FileHeader);
    if (offset + sizeof(Pe_OptionalHeaderStandard) > pe->data_size)
        return NULL;

    return (Pe_OptionalHeaderStandard *) &(pe->data[offset]);
}


Pe_OptionalHeaderStandardPlus * pe_ohsp (struct _pe * pe)
{
    uint32_t offset = pe->offset + sizeof(Pe_FileHeader);
    if (offset + sizeof(Pe_OptionalHeaderStandard) > pe->data_size)
        return NULL;

    return (Pe_OptionalHeaderStandardPlus *) &(pe->data[offset]);
}


Pe_OptionalHeaderWindows * pe_ohw (struct _pe * pe)
{
    uint32_t offset = pe->offset
                      + sizeof(Pe_FileHeader)
                      + sizeof(Pe_OptionalHeaderStandard);
    if (offset + sizeof(Pe_OptionalHeaderWindows) > pe->data_size)
        return NULL;

    return (Pe_OptionalHeaderWindows *) &(pe->data[offset]);
}


Pe_OptionalHeaderWindowsPlus * pe_ohwp (struct _pe * pe)
{
    uint32_t offset = pe->offset
                      + sizeof(Pe_FileHeader)
                      + sizeof(Pe_OptionalHeaderStandardPlus);
    if (offset + sizeof(Pe_OptionalHeaderWindowsPlus) > pe->data_size)
        return NULL;

    return (Pe_OptionalHeaderWindowsPlus *) &(pe->data[offset]);
}


Pe_SectionHeader * pe_sh (struct _pe * pe, unsigned int sh_i)
{
    Pe_FileHeader * pfh = pe_fh(pe);

    if (sh_i > pfh->NumberOfSections)
        return NULL;

    uint32_t offset = pe->offset
                      + sizeof(Pe_FileHeader)
                      + pfh->SizeOfOptionalHeader;
    offset += sizeof(Pe_SectionHeader) * sh_i;

    if (offset + sizeof(Pe_SectionHeader) > pe->data_size)
        return NULL;

    return (Pe_SectionHeader *) &(pe->data[offset]);
}


Pe_Symbol * pe_sym (struct _pe * pe, unsigned int sym_i)
{
    Pe_FileHeader * pfh = pe_fh(pe);

    if (sym_i > pfh->NumberOfSymbols)
        return NULL;

    uint32_t offset = pfh->PointerToSymbolTable;

    unsigned int i;
    for (i = 0; i < sym_i; i++) {
        if (offset + sizeof(Pe_Symbol) > pe->data_size)
            return NULL;

        Pe_Symbol * sym = (Pe_Symbol *) &(pe->data[offset]);
        offset += sizeof(Pe_Symbol) * (sym->NumberOfAuxSymbols + 1);
    }

    if (offset + sizeof(Pe_Symbol) > pe->data_size)
        return NULL;

    return (Pe_Symbol *) &(pe->data[offset]);
}


const char * pe_strtab_str (struct _pe * pe, unsigned int offset)
{
    Pe_FileHeader * pfh = pe_fh(pe);

    uint32_t strtab = pfh->PointerToSymbolTable
                      + sizeof(Pe_Symbol) * pfh->NumberOfSymbols;

    unsigned int i = 0;

    while (1) {
        if (strtab + offset + i > pe->data_size)
            return NULL;
        else if (pe->data[strtab + offset + i] == '\0')
            break;
        i++;
    }

    if (i == 0)
        return NULL;

    return (const char *) &(pe->data[strtab + offset]);
}


int pe_symbol_name (struct _pe * pe, int sym_i, char * buf, int buf_size)
{
    Pe_Symbol * sym = pe_sym(pe, sym_i);

    if (sym == NULL)
        return -1;

    if (sym->Name[0] == '\0') {
        unsigned int offset;
        offset  = (sym->Name[7] << 24) & 0xff000000;
        offset |= (sym->Name[6] << 16) & 0x00ff0000;
        offset |= (sym->Name[5] <<  8) & 0x0000ff00;
        offset |= (sym->Name[4]      ) & 0x000000ff;
        printf("offset: %x, %x %x %x %x %x %x %x %x\n", offset,
               sym->Name[0], sym->Name[1], sym->Name[2], sym->Name[3],
               sym->Name[4], sym->Name[5], sym->Name[6], sym->Name[7]);
        const char * name = pe_strtab_str(pe, offset);
        if (name == NULL)
            return -1;
        strncpy(buf, name, buf_size);
        buf[buf_size - 1] = '\0';
        return strlen(buf);
    }
    else {
        if (buf_size < 9) {
            strncpy(buf, sym->Name, buf_size);
            buf[buf_size - 1] = '\0';
        }
        else {
            strncpy(buf, sym->Name, 8);
            buf[8] = '\0';
        }
        return strlen(buf);
    }
}


uint64_t pe_image_base (struct _pe * pe)
{
    if (pe_plus(pe)) {
        Pe_OptionalHeaderWindowsPlus * ohwp = pe_ohwp(pe);
        return ohwp->ImageBase;
    }
    Pe_OptionalHeaderWindows * ohw = pe_ohw(pe);
    return ohw->ImageBase;
}


int pe_section_index_by_address (struct _pe * pe, uint64_t address)
{
    unsigned int sh_i;

    uint64_t base_address = pe_image_base(pe);

    for (sh_i = 0; ; sh_i++) {
        Pe_SectionHeader * sh = pe_sh(pe, sh_i);
        if (sh == NULL)
            break;

        if (    (base_address + sh->VirtualAddress <= address)
             && (base_address + sh->VirtualAddress + sh->VirtualSize > address))
            return sh_i;
    }

    return -1;
}


Pe_SectionHeader * pe_sh_safe (struct _pe * pe, unsigned int sh_i)
{
    Pe_SectionHeader * sh = pe_sh(pe, sh_i);
    if (sh == NULL)
        return NULL;

    if (sh->PointerToRawData + sh->SizeOfRawData > pe->data_size)
        return NULL;

    return sh;
}


int pe_plus (struct _pe * pe)
{
    Pe_OptionalHeaderStandard * ohs = pe_ohs(pe);
    if (ohs == NULL)
        return -1;

    if (ohs->Magic == PE_TYPE_PE32)
        return 0;
    else if (ohs->Magic == PE_TYPE_PE32PLUS)
        return 1;

    return -1;
}


uint64_t pe_entry (struct _pe * pe)
{
    if (pe_plus(pe) == 0) {
        Pe_OptionalHeaderStandard * ohs = pe_ohs(pe);
        return pe_image_base(pe) + ohs->AddressOfEntryPoint;
    }

    Pe_OptionalHeaderStandardPlus * ohsp = pe_ohsp(pe);
    return pe_image_base(pe) + ohsp->AddressOfEntryPoint;
}


struct _graph * pe_graph (struct _pe * pe)
{
    struct _map   * functions = pe_functions(pe);

    struct _graph * graph     = pe_graph_functions(pe, functions);

    object_delete(functions);

    return graph;
}


struct _map * pe_functions (struct _pe * pe)
{
    uint64_t entry_address = -1;
    if (pe_plus(pe)) {
        Pe_OptionalHeaderStandardPlus * ohsp = pe_ohsp(pe);
        entry_address = pe_image_base(pe) + ohsp->AddressOfEntryPoint;
    }
    else {
        Pe_OptionalHeaderStandard * ohs = pe_ohs(pe);
        entry_address = pe_image_base(pe) + ohs->AddressOfEntryPoint;
    }

    struct _map * functions = pe_function_address(pe, entry_address);

    struct _function * function = function_create(entry_address);
    if (map_fetch(functions, function->address) == NULL)
        map_insert(functions, function->address, function);
    object_delete(function);

    return functions;
}


struct _map * pe_labels (struct _pe * pe)
{
    struct _map * functions = pe_functions(pe);

    struct _map * labels    = pe_labels_functions(pe, functions);

    object_delete(functions);

    return labels;
}


struct _graph * pe_graph_address (struct _pe * pe, uint64_t address)
{
    int section_i = pe_section_index_by_address(pe, address);
    if (section_i == -1)
        return NULL;

    Pe_SectionHeader * sh = pe_sh_safe(pe, section_i);
    if (sh == NULL)
        return NULL;

    uint64_t section_base = pe_image_base(pe) + sh->VirtualAddress;
    uint64_t section_size = sh->SizeOfRawData;
    if (sh->VirtualSize < section_size)
        section_size = sh->VirtualSize;
    uint64_t section_offset = sh->PointerToRawData;

    Pe_FileHeader * pfh = pe_fh(pe);
    struct _graph * graph = NULL;
    if (pfh->Machine == IMAGE_FILE_MACHINE_AMD64) {
        graph = x8664_graph(section_base,
                            address - section_base,
                            &(pe->data[section_offset]),
                            section_size);
    }
    else if (pfh->Machine == IMAGE_FILE_MACHINE_I386) {
        graph = x86_graph(section_base,
                          address - section_base,
                          &(pe->data[section_offset]),
                          section_size);
    }

    if (graph != NULL)
        graph_reduce(graph);

    return graph;
}


struct _map * pe_function_address (struct _pe * pe, uint64_t address)
{
    int section_i = pe_section_index_by_address(pe, address);
    if (section_i == -1)
        return NULL;

    Pe_SectionHeader * sh = pe_sh_safe(pe, section_i);
    if (sh == NULL)
        return NULL;

    uint64_t section_base = pe_image_base(pe) + sh->VirtualAddress;
    uint64_t section_size = sh->SizeOfRawData;
    if (sh->VirtualSize < section_size)
        section_size = sh->VirtualSize;
    uint64_t section_offset = sh->PointerToRawData;

    Pe_FileHeader * pfh = pe_fh(pe);
    struct _map * functions = NULL;
    if (pfh->Machine == IMAGE_FILE_MACHINE_AMD64) {
        functions = x8664_functions(section_base,
                                    address - section_base,
                                    &(pe->data[section_offset]),
                                    section_size);
    }
    else if (pfh->Machine == IMAGE_FILE_MACHINE_I386) {
        functions = x86_functions(section_base,
                                  address - section_base,
                                  &(pe->data[section_offset]),
                                  section_size);
    }

    return functions;
}


struct _label * pe_label_address (struct _pe * pe, uint64_t address)
{
    // see if we have a symbol for this address
    Pe_FileHeader * pfh = pe_fh(pe);
    int sym_i = 0;
    for (sym_i = 0; sym_i < pfh->NumberOfSymbols; sym_i++) {
        Pe_Symbol * sym = pe_sym(pe, sym_i);
        if (sym == NULL)
            break;

        uint64_t value = sym->Value;
        if (    (    (sym->StorageClass != IMAGE_SYM_CLASS_STATIC)
                  && (sym->StorageClass != IMAGE_SYM_CLASS_EXTERNAL))
             || (sym->SectionNumber == 0))
            continue;

            // section numbers start at 1
        unsigned int section_i = sym->SectionNumber - 1;
        Pe_SectionHeader * sh = pe_sh(pe, section_i);
        if (sh == NULL)
            continue;

        value += sh->VirtualAddress + pe_image_base(pe);

        if (    (sym->Type != IMAGE_SYM_MSFT_FUNCTION)
             || (value != address))
            continue;


        char debugname[64];
        if (pe_symbol_name(pe, sym_i, debugname, 64) == -1) {
            debugname[0] = 'X';
            debugname[1] = '\0';
        }

        printf("symbol Name: '%s', StorageClass %x, Section: %x, Type: %x, value: %llx\n",
                debugname,
                sym->StorageClass,
                sym->SectionNumber,
                sym->Type,
                (unsigned long long) value);

        char buf[64];
        if (pe_symbol_name(pe, sym_i, buf, 64) != -1) {
            struct _label * label;
            label = label_create(address, buf, LABEL_FUNCTION);
            return label;
        }
    }


    char tmp[128];

    snprintf(tmp, 128, "fun_%04llx", (unsigned long long) address);

    struct _label * label = label_create(address, tmp, LABEL_FUNCTION);

    return label;
}


struct _map * pe_memory_map (struct _pe * pe)
{
    struct _map * map = map_create();
    Pe_FileHeader * pfh = pe_fh(pe);

    size_t sec_i;
    for (sec_i = 0; sec_i < pfh->NumberOfSections; sec_i++) {
        Pe_SectionHeader * psh = pe_sh(pe, sec_i);
        if (psh == NULL)
            continue;

        if (psh->VirtualSize == 0)
            continue;

        uint64_t size = psh->SizeOfRawData;
        if (psh->SizeOfRawData > psh->VirtualSize)
            size = psh->VirtualSize;

        if (psh->PointerToRawData + size > pe->data_size)
            continue;

        uint8_t * tmp = (uint8_t *) malloc(psh->VirtualSize);
        memset(tmp, 0, psh->VirtualSize);
        memcpy(tmp, &(pe->data[psh->PointerToRawData]), size);

        struct _buffer * buffer = buffer_create(tmp, psh->VirtualSize);
        free(tmp);

        mem_map_set(map, pe_image_base(pe) + psh->VirtualAddress, buffer);
        object_delete(buffer);
    }

    return map;
}


struct _graph * pe_graph_functions (struct _pe * pe, struct _map * functions)
{
    struct _graph * graph     = graph_create();

    struct _map_it * it;
    for (it = map_iterator(functions); it != NULL; it = map_it_next(it)) {
        struct _function * function = map_it_data(it);

        struct _graph * function_graph = pe_graph_address(pe, function->address);
        if (function_graph == NULL) {
            printf("null function graph: %llx\n",
                   (unsigned long long) function->address);
        }
        graph_merge(graph, function_graph);
        object_delete(function_graph);
    }

    remove_function_predecessors(graph, functions);
    graph_reduce(graph);
 
    return graph;
}


struct _map * pe_labels_functions (struct _pe * pe, struct _map * functions)
{
    struct _map * labels    = map_create();

    struct _map_it * it;
    for (it = map_iterator(functions); it != NULL; it = map_it_next(it)) {
        struct _function * function = map_it_data(it);

        struct _label * label = pe_label_address(pe, function->address);
        if (label != NULL)
            map_insert(labels, function->address, label);
        object_delete(label);
    }

    return labels;
}
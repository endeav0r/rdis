#ifndef pe_HEADER
#define pe_HEADER

#include <inttypes.h>

#include "graph.h"
#include "loader.h"
#include "map.h"
#include "object.h"
#include "pe_spec.h"
#include "tree.h"

struct _pe {
    const struct _loader_object * loader_object;
    union {
        uint8_t       * data;  
        Pe_FileHeader * pefh;
    };
    uint32_t offset; // offset to Pe_Fileheader
    size_t data_size;
};

struct _pe * pe_create      (const char * filename);
void         pe_delete      (struct _pe * pe);
struct _pe * pe_copy        (struct _pe * pe);
json_t *     pe_serialize   (struct _pe * pe);
struct _pe * pe_deserialize (json_t * json);

uint64_t        pe_entry            (struct _pe *);
struct _graph * pe_graph            (struct _pe *);
struct _map  *  pe_functions        (struct _pe *);
struct _map *   pe_labels           (struct _pe *);
struct _graph * pe_graph_address    (struct _pe *, uint64_t);
struct _map *   pe_memory_map       (struct _pe *);
struct _map  *  pe_function_address (struct _pe *, uint64_t);
struct _label * pe_label_address    (struct _pe *, uint64_t);


Pe_FileHeader *                 pe_fh     (struct _pe * pe);
Pe_OptionalHeaderStandard *     pe_ohs    (struct _pe * pe);
Pe_OptionalHeaderStandardPlus * pe_ohsp   (struct _pe * pe);
Pe_OptionalHeaderWindows *      pe_ohw    (struct _pe * pe);
Pe_OptionalHeaderWindowsPlus *  pe_ohwp   (struct _pe * pe);
Pe_SectionHeader *              pe_sh     (struct _pe * pe, unsigned int sh_i);
Pe_Symbol *                     pe_sym    (struct _pe * pe, unsigned int sym_i);


// returns a pointer to a string in the string table, or NULL if no string
// could be safely fetched
const char * pe_strtab (struct _pe * pe, unsigned int offset);

// fills the buf with the name of the symbol and returns the new size of
// name, or -1 if the name could not be found. buf will be null-terminated
int pe_symbol_name (struct _pe * pe, int sym_i, char * buf, int buf_size);

uint64_t pe_base_address (struct _pe * pe);

// returns the index of the section that the given address falls within, or -1
// if the section could not be found
int pe_section_index_by_address (struct _pe * pe, uint64_t address);

// like pe_sh, but checks to insure the sh (raw data) is within the bounds pe->data
Pe_SectionHeader * pe_sh_safe (struct _pe * pe, unsigned int sh_i);

// returns 1 if this is a PE+, 0 if PE
int pe_plus (struct _pe * pe);

#endif
#ifndef x8664_HEADER
#define x8664_HEADER

#include <udis86.h>

#include "instruction.h"
#include "graph.h"
#include "map.h"
#include "object.h"

struct _x8664_wqueue {
    const struct _object * object;
    uint64_t address;
    size_t   offset;
    void   * data;
    size_t   data_size;
};


struct _x8664_wqueue * x8664_wqueue_create
    (uint64_t address, size_t offset, void * data, size_t data_size);

void                   x8664_wqueue_delete     (struct _x8664_wqueue * x8664_wqueue);
struct _x8664_wqueue * x8664_wqueue_copy       (struct _x8664_wqueue * x8664_wqueue);
void *                 x8664_graph_wqueue      (struct _x8664_wqueue * x8664_wqueue);
void *                 x8664_functions_wqueue  (struct _x8664_wqueue * x8664_wqueue);

struct _ins *   x8664_ins   (uint64_t address, ud_t * ud_obj);

/*
* @param address   the base address at the beginning of data
* @param offset    the offset into data where we begin disassembly
* @param data      the data to disassemble
* @param data_size the size of data in bytes
*/
struct _graph * x8664_graph (uint64_t address,
                             size_t offset,
                             void * data,
                             size_t data_size);

struct _map * x8664_functions (uint64_t address,
                               size_t offset,
                               void * data,
                               size_t data_size);

uint64_t udis86_target           (uint64_t address, struct ud_operand * operand);
uint64_t udis86_sign_extend_lval (struct ud_operand * operand);
uint64_t udis86_rip_offset       (uint64_t address,
                                  struct ud_operand * operand);
#endif
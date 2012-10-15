#ifndef x8664_HEADER
#define x8664_HEADER

#include <udis86.h>

#include "instruction.h"
#include "graph.h"

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

uint64_t udis86_sign_extend_lval (struct ud_operand * operand);

#endif
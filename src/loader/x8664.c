#include "x8664.h"

#include "function.h"
#include "index.h"
#include "reference.h"


static const struct _object x8664_graph_wqueue_object = {
    (void   (*) (void *))         x8664_graph_wqueue_delete, 
    (void * (*) (void *))         x8664_graph_wqueue_copy,
    NULL,
    NULL
};


struct _x8664_graph_wqueue * x8664_graph_wqueue_create
    (uint64_t address, size_t offset, void * data, size_t data_size)
{
    struct _x8664_graph_wqueue * xgw;

    xgw = (struct _x8664_graph_wqueue *) malloc(sizeof(struct _x8664_graph_wqueue));
    xgw->object    = &x8664_graph_wqueue_object;
    xgw->address   = address;
    xgw->offset    = offset;
    xgw->data      = data;
    xgw->data_size = data_size;

    return xgw;
}


void x8664_graph_wqueue_delete
    (struct _x8664_graph_wqueue * x8664_graph_wqueue)
{
    free(x8664_graph_wqueue);
}


struct _x8664_graph_wqueue * x8664_graph_wqueue_copy 
    (struct _x8664_graph_wqueue * x8664_graph_wqueue)
{
    return x8664_graph_wqueue_create(x8664_graph_wqueue->address,
                                     x8664_graph_wqueue->offset,
                                     x8664_graph_wqueue->data,
                                     x8664_graph_wqueue->data_size);
}


void * x8664_graph_wqueue (struct _x8664_graph_wqueue * x8664_graph_wqueue)
{
    return x8664_graph(x8664_graph_wqueue->address,
                       x8664_graph_wqueue->offset,
                       x8664_graph_wqueue->data,
                       x8664_graph_wqueue->data_size);
}


struct _ins * x8664_ins (uint64_t address, ud_t * ud_obj)
{
    struct _ins * ins;
    ins = ins_create(address,
                     ud_insn_ptr(ud_obj),
                     ud_insn_len(ud_obj),
                     ud_insn_asm(ud_obj),
                     NULL);

    if (udis86_target(address, &(ud_obj->operand[0])) != -1) {
        char * mnemonic_str = NULL;
        switch (ud_obj->mnemonic) {
        case UD_Ijo   : mnemonic_str = "jo"; break;
        case UD_Ijno  : mnemonic_str = "jno"; break;
        case UD_Ijb   : mnemonic_str = "jb"; break;
        case UD_Ijae  : mnemonic_str = "jae"; break;
        case UD_Ijz   : mnemonic_str = "jz"; break;
        case UD_Ijnz  : mnemonic_str = "jnz"; break;
        case UD_Ijbe  : mnemonic_str = "jbe"; break;
        case UD_Ija   : mnemonic_str = "ja"; break;
        case UD_Ijs   : mnemonic_str = "js"; break;
        case UD_Ijns  : mnemonic_str = "jns"; break;
        case UD_Ijp   : mnemonic_str = "jp"; break;
        case UD_Ijnp  : mnemonic_str = "jnp"; break;
        case UD_Ijl   : mnemonic_str = "jl"; break;
        case UD_Ijge  : mnemonic_str = "jge"; break;
        case UD_Ijle  : mnemonic_str = "jle"; break;
        case UD_Ijg   : mnemonic_str = "jg"; break;
        case UD_Ijmp  : mnemonic_str = "jmp"; break;
        case UD_Icall : mnemonic_str = "call"; break;
        default : break;
        }

        uint64_t destination;
        destination  = ud_insn_len(ud_obj);
        destination += udis86_target(address, &(ud_obj->operand[0]));

        if (mnemonic_str != NULL) {
            char tmp[64];
            snprintf(tmp, 64, "%s %llx", mnemonic_str,
                     (unsigned long long) destination);
            ins_s_description(ins, tmp);
            ins_s_target(ins, destination);
        }
        else {
            struct _reference * reference;
            reference = reference_create(REFERENCE_STORE, address, destination);
            ins_add_reference(ins, reference);
            object_delete(reference);
        }
    }
    else if (udis86_target(address, &(ud_obj->operand[1])) != -1) {
        uint64_t destination;
        destination  = ud_insn_len(ud_obj);
        destination += udis86_target(address, &(ud_obj->operand[1]));
        struct _reference * reference;
        reference = reference_create(REFERENCE_LOAD, address, destination);
        ins_add_reference(ins, reference);
        object_delete(reference);
    }
    else if (    (ud_obj->operand[1].type == UD_OP_IMM)
              && (ud_obj->operand[1].size >= 32)) {
        int64_t tmp = udis86_sign_extend_lval(&(ud_obj->operand[1]));
        if (tmp > 0x1000) {
            struct _reference * reference;
            reference = reference_create(REFERENCE_CONSTANT,
                                         address,
                                         udis86_sign_extend_lval(&(ud_obj->operand[1])));
            ins_add_reference(ins, reference);
            object_delete(reference);
        }
    }

    if (ud_obj->mnemonic == UD_Icall)
        ins_s_call(ins);

    return ins;
}


uint64_t udis86_target (uint64_t address, struct ud_operand * operand)
{
    if (operand->type == UD_OP_JIMM)
        return address + udis86_sign_extend_lval(operand);
    return udis86_rip_offset(address, operand);
}


uint64_t udis86_sign_extend_lval (struct ud_operand * operand)
{
    int64_t lval = 0;
    switch (operand->size) {
    case 8  :
        lval = operand->lval.sbyte;
        break;
    case 16 :
        lval = operand->lval.sword;
        break;
    case 32 :
        lval = operand->lval.sdword;
        break;
    case 64 :
        lval = operand->lval.sqword;
        break;
    }
    return lval;
}


uint64_t udis86_rip_offset (uint64_t address, struct ud_operand * operand)
{
    if (    (operand->type  == UD_OP_MEM)
         && (operand->base  == UD_R_RIP)
         && (operand->index == 0)
         && (operand->scale == 0)) {
        switch (operand->size) {
            case 8  : return address + operand->lval.ubyte;
            case 16 : return address + operand->lval.uword;
            case 32 : return address + operand->lval.udword;
            case 64 : return address + operand->lval.uqword;
        }
    }
    return -1;
}


/*
* This is the initial phase, used to populate the graph with all reachable
* nodes. We will worry about fixing edges from jmp-like mnemonics later.
*/
void x8664_graph_0 (struct _graph * graph,
                    uint64_t        address,
                    size_t          offset,
                    uint8_t *       data,
                    size_t          data_size)
{
    ud_t            ud_obj;
    int             continue_disassembling = 1;
    uint64_t        last_address = -1;
    int             edge_type = INS_EDGE_NORMAL;

    if (offset > data_size)
        return;

    ud_init      (&ud_obj);
    ud_set_mode  (&ud_obj, 64);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, (uint8_t *) &(data[offset]), data_size - offset);

    while (continue_disassembling == 1) {
        size_t bytes_disassembled = ud_disassemble(&ud_obj);
        if (bytes_disassembled == 0) {
            break;
        }

        // even if we have already added this node, make sure we add the edge
        // from the preceeding node, in case this node was added from a jump
        // previously. otherwise we won't have an edge from its preceeding
        // instruction
        if (graph_fetch_node(graph, address + offset) != NULL) {
            if (last_address != -1) {
                // not concerned if this call fails
                struct _ins_edge * ins_edge = ins_edge_create(edge_type);
                graph_add_edge(graph, last_address, address + offset, ins_edge);
                object_delete(ins_edge);
            }
            break;
        }

        // create graph node for this instruction
        struct _ins * ins = x8664_ins(address + offset, &ud_obj);
        struct _list * ins_list = list_create();
        list_append(ins_list, ins);
        graph_add_node(graph, address + offset, ins_list);
        object_delete(ins_list);
        object_delete(ins);

        // add edge from previous instruction to this instruction
        if (last_address != -1) {
            struct _ins_edge * ins_edge = ins_edge_create(edge_type);
            graph_add_edge(graph, last_address, address + offset, ins_edge);
            /*
            if (graph_add_edge(graph, last_address, address + offset, ins_edge))
                printf("error adding edge %llx -> %llx\n",
                       (unsigned long long) last_address,
                       (unsigned long long) address + offset);
            */
            object_delete(ins_edge);
        }
        last_address = address + offset;

        // these mnemonics cause us to continue disassembly somewhere else
        struct ud_operand * operand;
        switch (ud_obj.mnemonic) {
        case UD_Ijo   :
        case UD_Ijno  :
        case UD_Ijb   :
        case UD_Ijae  :
        case UD_Ijz   :
        case UD_Ijnz  :
        case UD_Ijbe  :
        case UD_Ija   :
        case UD_Ijs   :
        case UD_Ijns  :
        case UD_Ijp   :
        case UD_Ijnp  :
        case UD_Ijl   :
        case UD_Ijge  :
        case UD_Ijle  :
        case UD_Ijg   :
        case UD_Ijmp  :
        //case UD_Icall :
            operand = &(ud_obj.operand[0]);

            if (operand->type != UD_OP_JIMM)
                break;

            if (ud_obj.mnemonic == UD_Icall)
                edge_type = INS_EDGE_NORMAL;
            else if (ud_obj.mnemonic == UD_Ijmp)
                edge_type = INS_EDGE_NORMAL; // not important, will terminate
            else
                edge_type = INS_EDGE_JCC_FALSE;

            if (operand->type == UD_OP_JIMM) {
                x8664_graph_0(graph,
                              address,
                              offset
                                + ud_insn_len(&ud_obj)
                                + udis86_sign_extend_lval(operand),
                              data,
                              data_size);
            }
            break;
        default :
            edge_type = INS_EDGE_NORMAL;
            break;
        }

        // these mnemonics cause disassembly to stop
        switch (ud_obj.mnemonic) {
        case UD_Iret :
        case UD_Ihlt :
        case UD_Ijmp :
            continue_disassembling = 0;
            break;
        default :
            break;
        }

        offset += bytes_disassembled;
    }
}


/*
* In this pass, we are fixing the edges from jmp-like instructions and their
* targets
*/
void x8664_graph_1 (struct _graph * graph,
                    uint64_t        address,
                    size_t          offset,
                    uint8_t *       data,
                    size_t          data_size)
{
    struct _graph_it * it;
    ud_t               ud_obj;

    for (it = graph_iterator(graph); it != NULL; it = graph_it_next(it)) {
        struct _list * ins_list = graph_it_data(it);
        struct _ins  * ins = list_first(ins_list);

        ud_init      (&ud_obj);
        ud_set_mode  (&ud_obj, 64);
        ud_set_input_buffer(&ud_obj, ins->bytes, ins->size);
        ud_disassemble(&ud_obj);

        struct ud_operand * operand;
        switch (ud_obj.mnemonic) {
        case UD_Ijmp  :
        case UD_Ijo   :
        case UD_Ijno  :
        case UD_Ijb   :
        case UD_Ijae  :
        case UD_Ijz   :
        case UD_Ijnz  :
        case UD_Ijbe  :
        case UD_Ija   :
        case UD_Ijs   :
        case UD_Ijns  :
        case UD_Ijp   :
        case UD_Ijnp  :
        case UD_Ijl   :
        case UD_Ijge  :
        case UD_Ijle  :
        case UD_Ijg   :
            operand = &(ud_obj.operand[0]);

            if (operand->type != UD_OP_JIMM)
                break;
            
            uint64_t head = graph_it_index(it);
            uint64_t tail = head
                             + ud_insn_len(&ud_obj)
                             + udis86_sign_extend_lval(operand);

            int type = INS_EDGE_JCC_TRUE;
            if (ud_obj.mnemonic == UD_Ijmp)
                type = INS_EDGE_JUMP;

            struct _ins_edge * ins_edge = ins_edge_create(type);
            graph_add_edge(graph, head, tail, ins_edge);
            /*
            if (graph_add_edge(graph, head, tail, ins_edge))
                printf("error, pass 1, adding edge %llx -> %llx\n",
                       (unsigned long long) head,
                       (unsigned long long) tail);
            */
            object_delete(ins_edge);
            break;
        default :
            break;
        }
    }
}



struct _graph * x8664_graph (uint64_t address,
                             size_t offset,
                             void * data,
                             size_t data_size)
{
    struct _graph * graph;

    graph = graph_create();

    x8664_graph_0(graph, address, offset, data, data_size);
    x8664_graph_1(graph, address, offset, data, data_size);

    return graph;
}



void x8664_functions_r (struct _map  * functions,
                        struct _tree * disassembled,
                        uint64_t       address,
                        size_t         offset,
                        uint8_t *      data,
                        size_t         data_size)
{
    ud_t            ud_obj;
    int             continue_disassembling = 1;

    if (offset > data_size)
        return;

    ud_init      (&ud_obj);
    ud_set_mode  (&ud_obj, 64);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_input_buffer(&ud_obj, (uint8_t *) &(data[offset]), data_size - offset);

    while (continue_disassembling == 1) {
        size_t bytes_disassembled = ud_disassemble(&ud_obj);
        if (bytes_disassembled == 0) {
            break;
        }

        if (    (ud_obj.mnemonic == UD_Icall)
             && (ud_obj.operand[0].type == UD_OP_JIMM)) {

            uint64_t target_addr = address
                                   + offset
                                   + ud_insn_len(&ud_obj)
                                   + udis86_sign_extend_lval(&(ud_obj.operand[0]));

            if (map_fetch(functions, target_addr) == NULL) {
                struct _function * function = function_create(target_addr);
                map_insert(functions, target_addr, function);
                object_delete(function);
            }
        }

        struct _index * index = index_create(address + offset);
        if (tree_fetch(disassembled, index) != NULL) {
            object_delete(index);
            return;
        }
        tree_insert(disassembled, index);
        object_delete(index);

        // these mnemonics cause us to continue disassembly somewhere else
        struct ud_operand * operand;
        switch (ud_obj.mnemonic) {
        case UD_Ijo   :
        case UD_Ijno  :
        case UD_Ijb   :
        case UD_Ijae  :
        case UD_Ijz   :
        case UD_Ijnz  :
        case UD_Ijbe  :
        case UD_Ija   :
        case UD_Ijs   :
        case UD_Ijns  :
        case UD_Ijp   :
        case UD_Ijnp  :
        case UD_Ijl   :
        case UD_Ijge  :
        case UD_Ijle  :
        case UD_Ijg   :
        case UD_Ijmp  :
        case UD_Icall :
            operand = &(ud_obj.operand[0]);

            if (operand->type == UD_OP_JIMM) {
                x8664_functions_r(functions,
                                  disassembled,
                                  address,
                                  offset
                                   + ud_insn_len(&ud_obj)
                                   + udis86_sign_extend_lval(operand),
                                  data,
                                  data_size);
            }
            break;
        default :
            break;
        }

        // these mnemonics cause disassembly to stop
        switch (ud_obj.mnemonic) {
        case UD_Iret :
        case UD_Ihlt :
        case UD_Ijmp :
            continue_disassembling = 0;
            break;
        default :
            break;
        }

        offset += bytes_disassembled;
    }
}



struct _map * x8664_functions (uint64_t address,
                               size_t offset,
                               void * data,
                               size_t data_size)
{

    struct _map  * functions    = map_create();
    struct _tree * disassembled = tree_create();

    x8664_functions_r(functions, disassembled, address, offset, data, data_size);

    object_delete(disassembled);

    return functions;
}
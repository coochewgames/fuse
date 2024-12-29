#ifndef Z80_OPCODES_H
#define Z80_OPCODES_H

#include <stdbool.h>

#include "mnemonics.h"
#include "execute_z80_opcode.h"

/*
 *  These can consist of larger strings than the actual operand
 *  but the operand should be at least 1 character long.
 *  eg. "(REGISTER+dd)" or "REGISTERH" are valid operands.
 */
#define MAX_OPERAND_LENGTH 15

typedef struct {
    unsigned char id;

    Z80_MNEMONIC op;
    Z80_OP_FUNC_LOOKUP op_func_lookup;

    char operand_1[MAX_OPERAND_LENGTH];
    char operand_2[MAX_OPERAND_LENGTH];
    char extras[MAX_OPERAND_LENGTH];
} Z80_OP;

typedef struct {
    int num_op_codes;
    Z80_OP *op_codes;
} Z80_OPS;

typedef enum {
    OPCODE_BASE = 0,
    OPCODE_CB,
    OPCODE_DDFD,
    OPCODE_DDFDCB,
    OPCODE_ED,
    OPCODE_SET_NUM
} Z80_OP_SET_TYPE;

typedef struct {
    Z80_OP_SET_TYPE set;
    char *name;
} Z80_OP_SET_NAME;

extern Z80_OPS z80_ops_set[];


bool init_op_sets();

#endif // Z80_OPCODES_H
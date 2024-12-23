#ifndef READ_OPS_FROM_DAT_FILE_H
#define READ_OPS_FROM_DAT_FILE_H

#include <stdbool.h>
#include <libspectrum.h>

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

bool readOpcodes(const char *filename);

extern Z80_OP *opcodes;
extern int numOpcodes;

#endif

#ifndef READ_OPS_FROM_DAT_FILE_H
#define READ_OPS_FROM_DAT_FILE_H

#include <stdbool.h>

#include "mnemonics.h"

#define MAX_OPERAND_LENGTH 7

typedef struct {
    unsigned char id;
    Z80_MNEMONIC op;
    char operand_1[MAX_OPERAND_LENGTH];
    char operand_2[MAX_OPERAND_LENGTH];

    void (*operation)(void);
} Z80_OP;

bool readOpcodes(const char *filename);

extern Z80_OP *opcodes;
extern int numOpcodes;

#endif

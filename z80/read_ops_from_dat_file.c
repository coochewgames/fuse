#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "read_ops_from_dat_file.h"
#include "../logging.h"

static const int MAX_LINE_LENGTH = 50;
static const int MAX_MNEMONIC_LENGTH = 10;
static const int INITIAL_CAPACITY = 10;


Z80_OP *opcodes = NULL;
int numOpcodes = 0;

static bool addOpcode(int *capacity, Z80_OP opcode);


bool readOpcodes(const char *filename) {
    FILE *file = fopen(filename, "rt");
    int line_count = 0;

    if (!file) {
        ERROR("Failed to open file '%s': %s", filename, strerror(errno));
        return false;
    }

    char line[MAX_LINE_LENGTH];
    int capacity = 0;

    while (fgets(line, sizeof(line), file)) {
        line_count++;

        if (strlen(line) == 0 || line[0] == '#'|| line[0] == '\n') {
            continue;
        }

        unsigned int id;
        char mnemonic[MAX_MNEMONIC_LENGTH];
        char operands[MAX_LINE_LENGTH] = "";
        char operand1[MAX_OPERAND_LENGTH] = "";
        char operand2[MAX_OPERAND_LENGTH] = "";
        char extras[MAX_OPERAND_LENGTH] = "";

        int numFields = sscanf(line, "%x %s %s %s", &id, mnemonic, operands, extras);

        if (numFields < 2) {
            ERROR("Invalid line format at line %d: %s", line_count, line);
            continue;
        }

        Z80_MNEMONIC op = get_mnemonic_enum(mnemonic);

        if (op == UNKNOWN_MNEMONIC) {
            ERROR("Unknown mnemonic found at line %d: %s", line_count, mnemonic);
            continue;
        }

        // Split operands by comma if present
        char *token = strtok(operands, ",");

        if (token) {
            strncpy(operand1, token, MAX_OPERAND_LENGTH);
            token = strtok(NULL, ",");

            if (token) {
                strncpy(operand2, token, MAX_OPERAND_LENGTH);
            }
        }

        Z80_OP opcode = {0};

        opcode.id = (unsigned char)id;
        opcode.op = op;
        opcode.op_func_lookup = get_z80_op_func(op);

        strncpy(opcode.operand_1, operand1, MAX_OPERAND_LENGTH);
        strncpy(opcode.operand_2, operand2, MAX_OPERAND_LENGTH);
        strncpy(opcode.extras, extras, MAX_OPERAND_LENGTH);

        if (addOpcode(&capacity, opcode) == false) {
            fclose(file);

            return false;
        }
    }

    fclose(file);
    return true;
}

static bool addOpcode(int *capacity, Z80_OP opcode) {
    if (numOpcodes != opcode.id) {
        WARNING("Invalid opcode ID (0x%02X %s) found at array position %d", opcode.id, get_mnemonic_name(opcode.op), numOpcodes);
    }

    if (opcode.id >= *capacity) {
        (*capacity) = opcode.id + INITIAL_CAPACITY;

        Z80_OP *newOpcodes = (Z80_OP *)realloc(opcodes, (*capacity) * sizeof(Z80_OP));

        if (!newOpcodes) {
            FATAL("Failed to reallocate memory");
            return false;
        }
        
        opcodes = newOpcodes;
    }

    opcodes[(int)opcode.id] = opcode;
    numOpcodes = opcode.id + 1;

    return true;
}

#ifdef TEST_READ_OPS_FROM_DAT_FILE
int main() {
    const char *filename = "opcodes_base.dat";
    int result = readOpcodes(filename);

    if (result == true) {
        printf("Successfully read %d opcodes.\n", numOpcodes);

        for (int i = 0; i < numOpcodes; i++) {
            Z80_OP opcode = opcodes[i];
            printf("ID: %02X, OP: %s, Operand 1: %s, Operand 2: %s\n", opcode.id, getMnemonicName(opcode.op), opcode.operand_1, opcode.operand_2);
        }
    } else {
        printf("Failed to read opcodes.\n");
    }

    free(opcodes);
    return 0;
}
#endif

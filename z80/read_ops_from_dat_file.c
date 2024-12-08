#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "read_ops_from_dat_file.h"

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
        fprintf(stderr, "Failed to open file '%s': %s\n", filename, strerror(errno));
        return false;
    }

    char line[MAX_LINE_LENGTH];
    int capacity = 0;

    while (fgets(line, sizeof(line), file)) {
        line_count++;

        if (strlen(line) > 0 && line[0] == '#') {
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
            fprintf(stderr, "Invalid line format at line %d: %s", line_count, line);
            continue;
        }

        Z80_MNEMONIC op = getMnemonicEnum(mnemonic);

        if (op == UNKNOWN_MNEMONIC) {
            fprintf(stderr, "Unknown mnemonic found at line %d: %s\n", line_count, mnemonic);
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
        strncpy(opcode.operand_1, operand1, MAX_OPERAND_LENGTH);
        strncpy(opcode.operand_2, operand2, MAX_OPERAND_LENGTH);

        opcode.operation = NULL; // Set to NULL or appropriate function pointer

        if (addOpcode(&capacity, opcode) == false) {
            free(opcodes);
            fclose(file);

            return false;
        }
    }

    fclose(file);
    return true;
}

static bool addOpcode(int *capacity, Z80_OP opcode) {
    if (numOpcodes >= *capacity) {
        (*capacity) += INITIAL_CAPACITY;

        Z80_OP *newOpcodes = (Z80_OP *)realloc(opcodes, (*capacity) * sizeof(Z80_OP));

        if (!newOpcodes) {
            perror("Failed to reallocate memory");
            return false;
        }
        
        opcodes = newOpcodes;
    }

    opcodes[numOpcodes++] = opcode;
    return true;
}

#ifdef TEST_READ_OPS_FROM_DAT_FILE
int main() {
    const char *filename = "opcodes_base.dat";
    int result = readOpcodes(filename);

    if (result > 0) {
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "read_ops_from_dat_file.h"
#include "../logging.h"

static const int MAX_LINE_LENGTH = 50;
static const int MAX_MNEMONIC_LENGTH = 10;
static const int INITIAL_CAPACITY = 10;


static bool add_op_code(int *capacity, Z80_OP op_code, Z80_OPS *ops);


/*
 *  Read the op codes from the dat files and store them in the Z80_OPS struct.
 *  The files can skip op codes by leaving gaps in the ID sequence and it can also
 *  have IDs with no associated op codes.
 */
Z80_OPS read_op_codes(const char *filename) {
    Z80_OPS ops = {0};

    FILE *file = fopen(filename, "rt");
    char line[MAX_LINE_LENGTH];
    
    int line_count = 0;
    int capacity = 0;

    if (!file) {
        ERROR("Failed to open file '%s': %s", filename, strerror(errno));
        return ops;
    }

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

        Z80_OP op_code = {0};

        op_code.id = (unsigned char)id;
        op_code.op = op;
        op_code.op_func_lookup = get_z80_op_func(op);

        strncpy(op_code.operand_1, operand1, MAX_OPERAND_LENGTH);
        strncpy(op_code.operand_2, operand2, MAX_OPERAND_LENGTH);
        strncpy(op_code.extras, extras, MAX_OPERAND_LENGTH);

        if (add_op_code(&capacity, op_code, &ops) == false) {
            fclose(file);

            return ops;
        }
    }

    fclose(file);
    return ops;
}

static bool add_op_code(int *capacity, Z80_OP op_code, Z80_OPS *ops) {
    if (ops->num_op_codes != op_code.id) {
        WARNING("Invalid opcode ID (0x%02X %d %s) found at array position %d", op_code.id, op_code.id, get_mnemonic_name(op_code.op), ops->num_op_codes);
    }

    if (op_code.id >= *capacity) {
        (*capacity) = op_code.id + INITIAL_CAPACITY;

        Z80_OP *new_op_codes = (Z80_OP *)realloc(ops->op_codes, (*capacity) * sizeof(Z80_OP));

        if (!new_op_codes) {
            FATAL("Failed to reallocate memory");
            return false;
        }
        
        ops->op_codes = new_op_codes;
    }

    ops->op_codes[(int)op_code.id] = op_code;
    ops->num_op_codes = op_code.id + 1;

    return true;
}

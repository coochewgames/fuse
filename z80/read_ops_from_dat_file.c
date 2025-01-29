#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "read_ops_from_dat_file.h"
#include "../logging.h"

#define MAX_LINE_LENGTH 50
#define MAX_MNEMONIC_LENGTH 10


static void init_op_codes(Z80_OPS *ops);


/*
 *  Read the op codes from the dat files and store them in the Z80_OPS struct.
 *  The files can skip op codes by leaving gaps in the ID sequence and it can also
 *  have IDs with no associated op codes.
 *  The function will return a sparse array with the identifier of the op code being
 *  used for the index of the array to avoid having to loop through to find a value.
 */
Z80_OPS read_op_codes(const char *filename) {
    Z80_OPS ops = {0};

    FILE *file = fopen(filename, "rt");
    char line[MAX_LINE_LENGTH];
    int line_count = 0;

    if (!file) {
        ERROR("Failed to open file '%s': %s", filename, strerror(errno));
        return ops;
    }

    init_op_codes(&ops);

    while (fgets(line, sizeof(line), file)) {
        unsigned int id;
        char mnemonic[MAX_MNEMONIC_LENGTH];
        char operands[MAX_LINE_LENGTH] = "";
        char operand1[MAX_OPERAND_LENGTH] = "";
        char operand2[MAX_OPERAND_LENGTH] = "";
        char extras[MAX_OPERAND_LENGTH] = "";

        line_count++;

        if (strlen(line) == 0 || line[0] == '#'|| line[0] == '\n') {
            continue;
        }

        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        int numFields = sscanf(line, "%x %s %s %s", &id, mnemonic, operands, extras);

        if (numFields == 0 || numFields == EOF) {
            ERROR("Skipping line with invalid format at line %d: %s", line_count, line);
            continue;
        }

        //  An id that has an id but with no command, is left as a NOP command.
        if (numFields > 1) {
            Z80_MNEMONIC op_mnemonic = UNKNOWN_MNEMONIC;
            Z80_OP *op = NULL;

            op_mnemonic = get_mnemonic_enum(mnemonic);

            if (op_mnemonic == UNKNOWN_MNEMONIC) {
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

            op  = &ops.op_codes[id];

            op->id = (unsigned char)id;
            op->op = op_mnemonic;
            op->op_func_lookup = get_z80_op_func(op_mnemonic);

            strncpy(op->operand_1, operand1, MAX_OPERAND_LENGTH);
            strncpy(op->operand_2, operand2, MAX_OPERAND_LENGTH);
            strncpy(op->extras, extras, MAX_OPERAND_LENGTH);

            ops.num_op_codes++;
        }
    }

    fclose(file);
    return ops;
}

static void init_op_codes(Z80_OPS *ops) {
    for (int id = 0; id < MAX_OP_CODE_IDS; id++) {
        Z80_OP *op = &ops->op_codes[id];


        op->id = (unsigned char)id;
        op->op = NOP;
        op->op_func_lookup = get_z80_op_func(NOP);
    }
}

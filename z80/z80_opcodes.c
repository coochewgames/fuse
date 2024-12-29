#include "z80_opcodes.h"
#include "read_ops_from_dat_file.h"

#include "../logging.h"

static Z80_OP_SET_NAME z80_ops_sets_list[] = {
    { OPCODE_BASE, "opcodes_base.dat" },
    { OPCODE_CB, "opcodes_cb.dat" },
    { OPCODE_DDFD, "opcodes_ddfd.dat" },
    { OPCODE_DDFDCB, "opcodes_ddfdcb.dat" },
    { OPCODE_ED, "opcodes_ed.dat" },
    { 0, NULL }
};


Z80_OPS z80_ops_set[OPCODE_SET_NUM];


/*
 *  The enum values for the Z80_OP_SET_TYPE match the index of the Z80_OP_SET_NAME in the z80_ops_sets_list.
 */
bool init_op_sets() {
    for (int enum_pos = 0; z80_ops_sets_list[enum_pos].name != NULL; enum_pos++) {
        if (enum_pos != z80_ops_sets_list[enum_pos].set) {
            FATAL("Invalid set configuration found for %s: %d", z80_ops_sets_list[enum_pos].name, z80_ops_sets_list[enum_pos].set);
            return false;
        }

        Z80_OPS ops = read_op_codes(z80_ops_sets_list[enum_pos].name);

        if (ops.num_op_codes == 0) {
            FATAL("Failed to read op codes from %s", z80_ops_sets_list[enum_pos].name);
            return false;
        }

        z80_ops_set[enum_pos] = ops;
    }

    return true;
}

#ifdef TEST_READ_OPS_FROM_DAT_FILE
int main() {
    if (init_op_sets()) {
        for (int enum_pos = 0; z80_ops_sets_list[enum_pos].name != NULL; enum_pos++) {
            Z80_OPS ops = z80_ops_set[enum_pos];

            if (ops.num_op_codes > 0) {
                printf("Successfully read %d opcodes for %s.\n", ops.num_op_codes, z80_ops_sets_list[enum_pos].name);

                for (int i = 0; i < ops.num_op_codes; i++) {
                    Z80_OP op_code = ops.op_codes[i];
                    printf("ID: %02X, OP: %s, Operand 1: %s, Operand 2: %s\n", op_code.id, get_mnemonic_name(op_code.op), op_code.operand_1, op_code.operand_2);
                }
            } else {
                printf("Failed to read op codes for %s.\n", z80_ops_sets_list[enum_pos].name);
            }
        }

        for (int enum_pos = 0; z80_ops_sets_list[enum_pos].name != NULL; enum_pos++) {
            Z80_OPS ops = z80_ops_set[enum_pos];

            free(ops.op_codes);
        }
    }

    return 0;
}
#endif

#include <stdio.h>
#include <stdlib.h>

#include "z80_opcodes.h"
#include "execute_z80_opcode.h"
#include "read_ops_from_dat_file.h"

#include "../logging.h"


static Z80_OP_SET_NAME z80_ops_sets_list[] = {
    { OP_SET_BASE, "./z80/opcodes_base.dat" },
    { OP_SET_CB, "./z80/opcodes_cb.dat" },
    { OP_SET_DDFD, "./z80/opcodes_ddfd.dat" },
    { OP_SET_DDFDCB, "./z80/opcodes_ddfdcb.dat" },
    { OP_SET_ED, "./z80/opcodes_ed.dat" },
    { OP_SET_NUM, NULL }
};

static Z80_OP_FUNC_LOOKUP z80_op_func_lookup[] = {
    { ADD, OP_TYPE_TWO_PARAMS, .func.two_params = op_ADD },
    { ADC, OP_TYPE_TWO_PARAMS, .func.two_params = op_ADC },
    { AND, OP_TYPE_TWO_PARAMS, .func.two_params = op_AND },
    { BIT, OP_TYPE_TWO_PARAMS, .func.two_params = op_BIT },
    { CALL, OP_TYPE_TWO_PARAMS, .func.two_params = op_CALL },
    { CCF, OP_TYPE_NO_PARAMS, .func.no_params = op_CCF },
    { CP, OP_TYPE_ONE_PARAM, .func.one_param = op_CP },
    { CPD, OP_TYPE_NO_PARAMS, .func.no_params = op_CPD },
    { CPDR, OP_TYPE_NO_PARAMS, .func.no_params = op_CPDR },
    { CPI, OP_TYPE_NO_PARAMS, .func.no_params = op_CPI },
    { CPIR, OP_TYPE_NO_PARAMS, .func.no_params = op_CPIR },
    { CPL, OP_TYPE_NO_PARAMS, .func.no_params = op_CPL },
    { DAA, OP_TYPE_NO_PARAMS, .func.no_params = op_DAA },
    { DEC, OP_TYPE_ONE_PARAM, .func.one_param = op_DEC },
    { DI, OP_TYPE_NO_PARAMS, .func.no_params = op_DI },
    { DJNZ, OP_TYPE_ONE_PARAM, .func.one_param = op_DJNZ },
    { EI, OP_TYPE_NO_PARAMS, .func.no_params = op_EI },
    { EX, OP_TYPE_TWO_PARAMS, .func.two_params = op_EX },
    { EXX, OP_TYPE_NO_PARAMS, .func.no_params = op_EXX },
    { HALT, OP_TYPE_NO_PARAMS, .func.no_params = op_HALT },
    { IM, OP_TYPE_ONE_PARAM, .func.one_param = op_IM },
    { IN, OP_TYPE_TWO_PARAMS, .func.two_params = op_IN },
    { INC, OP_TYPE_ONE_PARAM, .func.one_param = op_INC },
    { IND, OP_TYPE_NO_PARAMS, .func.no_params = op_IND },
    { INDR, OP_TYPE_NO_PARAMS, .func.no_params = op_INDR },
    { INI, OP_TYPE_NO_PARAMS, .func.no_params = op_INI },
    { INIR, OP_TYPE_NO_PARAMS, .func.no_params = op_INIR },
    { JP, OP_TYPE_TWO_PARAMS, .func.two_params = op_JP },
    { JR, OP_TYPE_TWO_PARAMS, .func.two_params = op_JR },
    { LD, OP_TYPE_TWO_PARAMS, .func.two_params = op_LD },
    { LDD, OP_TYPE_NO_PARAMS, .func.no_params = op_LDD },
    { LDDR, OP_TYPE_NO_PARAMS, .func.no_params = op_LDDR },
    { LDI, OP_TYPE_NO_PARAMS, .func.no_params = op_LDI },
    { LDIR, OP_TYPE_NO_PARAMS, .func.no_params = op_LDIR },
    { NEG, OP_TYPE_NO_PARAMS, .func.no_params = op_NEG },
    { NOP, OP_TYPE_NO_PARAMS, .func.no_params = op_NOP },
    { OR, OP_TYPE_TWO_PARAMS, .func.two_params = op_OR },
    { OUT, OP_TYPE_TWO_PARAMS, .func.two_params = op_OUT },
    { OTDR, OP_TYPE_NO_PARAMS, .func.no_params = op_OTDR },
    { OTIR, OP_TYPE_NO_PARAMS, .func.no_params = op_OTIR },
    { OUTD, OP_TYPE_NO_PARAMS, .func.no_params = op_OUTD },
    { OUTI, OP_TYPE_NO_PARAMS, .func.no_params = op_OUTI },
    { POP, OP_TYPE_ONE_PARAM, .func.one_param = op_POP },
    { PUSH, OP_TYPE_ONE_PARAM, .func.one_param = op_PUSH },
    { RES, OP_TYPE_TWO_PARAMS, .func.two_params = op_RES },
    { RET, OP_TYPE_ONE_PARAM, .func.one_param = op_RET },
    { RETN, OP_TYPE_NO_PARAMS, .func.no_params = op_RETN },
    { RL, OP_TYPE_ONE_PARAM, .func.one_param = op_RL },
    { RLA, OP_TYPE_NO_PARAMS, .func.no_params = op_RLA },
    { RLC, OP_TYPE_ONE_PARAM, .func.one_param = op_RLC },
    { RLCA, OP_TYPE_NO_PARAMS, .func.no_params = op_RLCA },
    { RLD, OP_TYPE_NO_PARAMS, .func.no_params = op_RLD },
    { RR, OP_TYPE_ONE_PARAM, .func.one_param = op_RR },
    { RRA, OP_TYPE_NO_PARAMS, .func.no_params = op_RRA },
    { RRC, OP_TYPE_ONE_PARAM, .func.one_param = op_RRC },
    { RRCA, OP_TYPE_NO_PARAMS, .func.no_params = op_RRCA },
    { RRD, OP_TYPE_NO_PARAMS, .func.no_params = op_RRD },
    { RST, OP_TYPE_ONE_PARAM, .func.one_param = op_RST },
    { SBC, OP_TYPE_TWO_PARAMS, .func.two_params = op_SBC },
    { SCF, OP_TYPE_NO_PARAMS, .func.no_params = op_SCF },
    { SET, OP_TYPE_TWO_PARAMS, .func.two_params = op_SET },
    { SLA, OP_TYPE_ONE_PARAM, .func.one_param = op_SLA },
    { SLL, OP_TYPE_ONE_PARAM, .func.one_param = op_SLL },
    { SRA, OP_TYPE_ONE_PARAM, .func.one_param = op_SRA },
    { SRL, OP_TYPE_ONE_PARAM, .func.one_param = op_SRL },
    { SUB, OP_TYPE_TWO_PARAMS, .func.two_params = op_SUB },
    { XOR, OP_TYPE_TWO_PARAMS, .func.two_params = op_XOR },
    { SLTTRAP, OP_TYPE_NO_PARAMS, .func.no_params = op_SLTTRAP },
    { SHIFT, OP_TYPE_ONE_PARAM, .func.one_param = op_SHIFT },
    { Z80_MNEMONIC_COUNT, 0, {NULL} } // Sentinel value to mark the end of the array
};

Z80_OPS z80_ops_set[OP_SET_NUM];


/*
 *  The enum values for the Z80_OP_SET_TYPE match the index of the Z80_OP_SET_NAME in the z80_ops_sets_list.
 */
bool init_op_sets(void) {
    for (int enum_pos = 0; z80_ops_sets_list[enum_pos].name != NULL; enum_pos++) {
        if (enum_pos != z80_ops_sets_list[enum_pos].set_type) {
            FATAL("Invalid set configuration found for %s: %d", z80_ops_sets_list[enum_pos].name, z80_ops_sets_list[enum_pos].set_type);
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

Z80_OP_FUNC_LOOKUP get_z80_op_func(Z80_MNEMONIC op) {
    Z80_OP_FUNC_LOOKUP z80_op_func = { 0 };

    for (int i = 0; z80_op_func_lookup[i].op < Z80_MNEMONIC_COUNT; i++) {
        if (z80_op_func_lookup[i].op == op) {
            z80_op_func = z80_op_func_lookup[i];
            break;
        }
    }
    
    return z80_op_func;
}

void call_z80_op_func(Z80_OP op) {
    switch (op.op_func_lookup.function_type) {
        case OP_TYPE_NO_PARAMS:
            op.op_func_lookup.func.no_params();
            break;
        case OP_TYPE_ONE_PARAM:
            op.op_func_lookup.func.one_param(op.operand_1);
            break;
        case OP_TYPE_TWO_PARAMS:
            op.op_func_lookup.func.two_params(op.operand_1, op.operand_2);
            break;
        default:
            ERROR("Unexpected function type found for %s: %d", get_mnemonic_name(op.op), op.op_func_lookup.function_type);
    }
}

#ifdef TEST_READ_OPS_FROM_DAT_FILE
int main() {
    if (init_op_sets()) {
        printf("\n****Init Op Sets complete.\n\n");

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

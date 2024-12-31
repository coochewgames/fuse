#include <stdio.h>
#include <stdlib.h>

#include "z80_opcodes.h"
#include "execute_z80_opcode.h"
#include "read_ops_from_dat_file.h"

#include "../logging.h"


static Z80_OP_SET_NAME z80_ops_sets_list[] = {
    { OP_SET_BASE, "opcodes_base.dat" },
    { OP_SET_CB, "opcodes_cb.dat" },
    { OP_SET_DDFD, "opcodes_ddfd.dat" },
    { OP_SET_DDFDCB, "opcodes_ddfdcb.dat" },
    { OP_SET_ED, "opcodes_ed.dat" },
    { 0, NULL }
};

static Z80_OP_FUNC_LOOKUP z80_op_func_lookup[] = {
    { NOP, OP_TYPE_NO_PARAMS, .func.no_params = op_NOP },
    { LD, OP_TYPE_TWO_PARAMS, .func.two_params = op_LD },
    { INC, OP_TYPE_ONE_PARAM, .func.one_param = op_INC },
    { DEC, OP_TYPE_ONE_PARAM, .func.one_param = op_DEC },
    { ADD, OP_TYPE_TWO_PARAMS, .func.two_params = op_ADD },
    { SUB, OP_TYPE_ONE_PARAM, .func.one_param = op_SUB },
    { NEG, OP_TYPE_NO_PARAMS, .func.no_params = op_NEG },
    { AND, OP_TYPE_ONE_PARAM, .func.one_param = op_AND },
    { OR, OP_TYPE_ONE_PARAM, .func.one_param = op_OR },
    { XOR, OP_TYPE_ONE_PARAM, .func.one_param = op_XOR },
    { CP, OP_TYPE_ONE_PARAM, .func.one_param = op_CP },
    { JP, OP_TYPE_TWO_PARAMS, .func.two_params = op_JP },
    { JR, OP_TYPE_ONE_PARAM, .func.one_param = op_JR },
    { CALL, OP_TYPE_TWO_PARAMS, .func.two_params = op_CALL },
    { RET, OP_TYPE_NO_PARAMS, .func.no_params = op_RET },
    { RETN, OP_TYPE_NO_PARAMS, .func.no_params = op_RETN },
    { PUSH, OP_TYPE_ONE_PARAM, .func.one_param = op_PUSH },
    { POP, OP_TYPE_ONE_PARAM, .func.one_param = op_POP },
    { DAA, OP_TYPE_NO_PARAMS, .func.no_params = op_DAA },
    { CPL, OP_TYPE_NO_PARAMS, .func.no_params = op_CPL },
    { SCF, OP_TYPE_NO_PARAMS, .func.no_params = op_SCF },
    { CCF, OP_TYPE_NO_PARAMS, .func.no_params = op_CCF },
    { HALT, OP_TYPE_NO_PARAMS, .func.no_params = op_HALT },
    { DI, OP_TYPE_NO_PARAMS, .func.no_params = op_DI },
    { EI, OP_TYPE_NO_PARAMS, .func.no_params = op_EI },
    { RL, OP_TYPE_ONE_PARAM, .func.one_param = op_RL },
    { RR, OP_TYPE_ONE_PARAM, .func.one_param = op_RR },
    { SLA, OP_TYPE_ONE_PARAM, .func.one_param = op_SLA },
    { SRA, OP_TYPE_ONE_PARAM, .func.one_param = op_SRA },
    { SLL, OP_TYPE_ONE_PARAM, .func.one_param = op_SLL },
    { SRL, OP_TYPE_ONE_PARAM, .func.one_param = op_SRL },
    { RLA, OP_TYPE_NO_PARAMS, .func.no_params = op_RLA },
    { RRA, OP_TYPE_NO_PARAMS, .func.no_params = op_RRA },
    { RLC, OP_TYPE_ONE_PARAM, .func.one_param = op_RLC },
    { RRC, OP_TYPE_ONE_PARAM, .func.one_param = op_RRC },
    { RLD, OP_TYPE_NO_PARAMS, .func.no_params = op_RLD },
    { RRD, OP_TYPE_NO_PARAMS, .func.no_params = op_RRD },
    { RLCA, OP_TYPE_NO_PARAMS, .func.no_params = op_RLCA },
    { RRCA, OP_TYPE_NO_PARAMS, .func.no_params = op_RRCA },
    { BIT, OP_TYPE_TWO_PARAMS, .func.two_params = op_BIT },
    { RES, OP_TYPE_TWO_PARAMS, .func.two_params = op_RES },
    { SET, OP_TYPE_TWO_PARAMS, .func.two_params = op_SET },
    { EX, OP_TYPE_TWO_PARAMS, .func.two_params = op_EX },
    { DJNZ, OP_TYPE_ONE_PARAM, .func.one_param = op_DJNZ },
    { ADC, OP_TYPE_TWO_PARAMS, .func.two_params = op_ADC },
    { SBC, OP_TYPE_TWO_PARAMS, .func.two_params = op_SBC },
    { RST, OP_TYPE_ONE_PARAM, .func.one_param = op_RST },
    { OUT, OP_TYPE_TWO_PARAMS, .func.two_params = op_OUT },
    { EXX, OP_TYPE_NO_PARAMS, .func.no_params = op_EXX },
    { IN, OP_TYPE_TWO_PARAMS, .func.two_params = op_IN },
    { LDI, OP_TYPE_NO_PARAMS, .func.no_params = op_LDI },
    { CPI, OP_TYPE_NO_PARAMS, .func.no_params = op_CPI },
    { INI, OP_TYPE_NO_PARAMS, .func.no_params = op_INI },
    { OUTI, OP_TYPE_NO_PARAMS, .func.no_params = op_OUTI },
    { LDD, OP_TYPE_NO_PARAMS, .func.no_params = op_LDD },
    { CPD, OP_TYPE_NO_PARAMS, .func.no_params = op_CPD },
    { IND, OP_TYPE_NO_PARAMS, .func.no_params = op_IND },
    { OUTD, OP_TYPE_NO_PARAMS, .func.no_params = op_OUTD },
    { LDIR, OP_TYPE_NO_PARAMS, .func.no_params = op_LDIR },
    { CPIR, OP_TYPE_NO_PARAMS, .func.no_params = op_CPIR },
    { INIR, OP_TYPE_NO_PARAMS, .func.no_params = op_INIR },
    { OTIR, OP_TYPE_NO_PARAMS, .func.no_params = op_OTIR },
    { LDDR, OP_TYPE_NO_PARAMS, .func.no_params = op_LDDR },
    { CPDR, OP_TYPE_NO_PARAMS, .func.no_params = op_CPDR },
    { INDR, OP_TYPE_NO_PARAMS, .func.no_params = op_INDR },
    { OTDR, OP_TYPE_NO_PARAMS, .func.no_params = op_OTDR },
    { IM, OP_TYPE_ONE_PARAM, .func.one_param = op_IM },
    { SHIFT, OP_TYPE_ONE_PARAM, .func.one_param = op_SHIFT },
    { SLTTRAP, OP_TYPE_NO_PARAMS, .func.no_params = op_SLTTRAP },
    { 0, 0, {NULL} } // Sentinel value to mark the end of the array
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

    for (int i = 0; z80_op_func_lookup[i].op != 0; i++) {
        if (z80_op_func_lookup[i].op == op) {
            z80_op_func = z80_op_func_lookup[i];
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

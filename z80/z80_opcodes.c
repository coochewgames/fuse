#include <stdio.h>
#include <stdlib.h>

#include "z80_opcodes.h"
#include "execute_z80_opcode.h"
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

static Z80_OP_FUNC_LOOKUP z80_op_func_lookup[] = {
    { NOP, OP_TYPE_NO_PARAMS, .func.no_params = op_NOP },
    { LD, OP_TYPE_TWO_PARAMS, .func.two_params = op_LD },
    { INC, OP_TYPE_ONE_PARAM, .func.one_param = op_INC },
    { DEC, OP_TYPE_ONE_PARAM, .func.one_param = op_DEC },
    { RLCA, OP_TYPE_NO_PARAMS, .func.no_params = op_RLCA },
    { ADD, OP_TYPE_TWO_PARAMS, .func.two_params = op_ADD },
    { RRCA, OP_TYPE_NO_PARAMS, .func.no_params = op_RRCA },
    { RLA, OP_TYPE_NO_PARAMS, .func.no_params = op_RLA },
    { RRA, OP_TYPE_NO_PARAMS, .func.no_params = op_RRA },
    { DAA, OP_TYPE_NO_PARAMS, .func.no_params = op_DAA },
    { CPL, OP_TYPE_NO_PARAMS, .func.no_params = op_CPL },
    { SCF, OP_TYPE_NO_PARAMS, .func.no_params = op_SCF },
    { CCF, OP_TYPE_NO_PARAMS, .func.no_params = op_CCF },
    { HALT, OP_TYPE_NO_PARAMS, .func.no_params = op_HALT },
    { ADC, OP_TYPE_TWO_PARAMS, .func.two_params = op_ADC },
    { SUB, OP_TYPE_ONE_PARAM, .func.one_param = op_SUB },
    { SBC, OP_TYPE_TWO_PARAMS, .func.two_params = op_SBC },
    { AND, OP_TYPE_ONE_PARAM, .func.one_param = op_AND },
    { XOR, OP_TYPE_ONE_PARAM, .func.one_param = op_XOR },
    { OR, OP_TYPE_ONE_PARAM, .func.one_param = op_OR },
    { CP, OP_TYPE_ONE_PARAM, .func.one_param = op_CP },
    { RET, OP_TYPE_NO_PARAMS, .func.no_params = op_RET },
    { POP, OP_TYPE_ONE_PARAM, .func.one_param = op_POP },
    { JP, OP_TYPE_TWO_PARAMS, .func.two_params = op_JP },
    { CALL, OP_TYPE_TWO_PARAMS, .func.two_params = op_CALL },
    { PUSH, OP_TYPE_ONE_PARAM, .func.one_param = op_PUSH },
    { RST, OP_TYPE_ONE_PARAM, .func.one_param = op_RST },
    { EX, OP_TYPE_TWO_PARAMS, .func.two_params = op_EX },
    { DI, OP_TYPE_NO_PARAMS, .func.no_params = op_DI },
    { EI, OP_TYPE_NO_PARAMS, .func.no_params = op_EI },
    { RL, OP_TYPE_ONE_PARAM, .func.one_param = op_RL },
    { RR, OP_TYPE_ONE_PARAM, .func.one_param = op_RR },
    { SLA, OP_TYPE_ONE_PARAM, .func.one_param = op_SLA },
    { SRA, OP_TYPE_ONE_PARAM, .func.one_param = op_SRA },
    { SRL, OP_TYPE_ONE_PARAM, .func.one_param = op_SRL },
    { RLC, OP_TYPE_ONE_PARAM, .func.one_param = op_RLC },
    { RRC, OP_TYPE_ONE_PARAM, .func.one_param = op_RRC },
    { BIT, OP_TYPE_TWO_PARAMS, .func.two_params = op_BIT },
    { RES, OP_TYPE_TWO_PARAMS, .func.two_params = op_RES },
    { SET, OP_TYPE_TWO_PARAMS, .func.two_params = op_SET },
    { IN, OP_TYPE_TWO_PARAMS, .func.two_params = op_IN },
    { OUT, OP_TYPE_TWO_PARAMS, .func.two_params = op_OUT },
    { DJNZ, OP_TYPE_ONE_PARAM, .func.one_param = op_DJNZ },
    { JR, OP_TYPE_ONE_PARAM, .func.one_param = op_JR },
    { EXX, OP_TYPE_NO_PARAMS, .func.no_params = op_EXX },
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
    { SLL, OP_TYPE_ONE_PARAM, .func.one_param = op_SLL },
    { NEG, OP_TYPE_NO_PARAMS, .func.no_params = op_NEG },
    { RETN, OP_TYPE_NO_PARAMS, .func.no_params = op_RETN },
    { IM, OP_TYPE_ONE_PARAM, .func.one_param = op_IM },
    { RRD, OP_TYPE_NO_PARAMS, .func.no_params = op_RRD },
    { RLD, OP_TYPE_NO_PARAMS, .func.no_params = op_RLD },
    { 0, 0, {NULL} } // Sentinel value to mark the end of the array
};

Z80_OPS z80_ops_set[OPCODE_SET_NUM];


/*
 *  The enum values for the Z80_OP_SET_TYPE match the index of the Z80_OP_SET_NAME in the z80_ops_sets_list.
 */
bool init_op_sets(void) {
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

Z80_OP_FUNC_LOOKUP get_z80_op_func(Z80_MNEMONIC op) {
    Z80_OP_FUNC_LOOKUP z80_op_func = { 0 };

    for (int i = 0; z80_op_func_lookup[i].op != 0; i++) {
        if (z80_op_func_lookup[i].op == op) {
            z80_op_func = z80_op_func_lookup[i];
        }
    }
    
    return z80_op_func;
}

//#ifdef TEST_READ_OPS_FROM_DAT_FILE
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
//#endif

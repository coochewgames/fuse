#include <string.h>
#include <libspectrum.h>

#include "z80.h"
#include "z80_macros.h"
#include "../memory_pages.h"

#include "parse_z80_operands.h"
#include "execute_z80_opcode.h"
#include "execute_z80_command.h"
#include "logging.h"


typedef struct {
    const char *condition;
    unsigned char flag;
    bool is_not;
} FLAG_MAPPING;

static FLAG_MAPPING flag_lookup[] = {
    { "C", FLAG_C, false },
    { "NC", FLAG_C, true },
    { "PE", FLAG_P, false },
    { "PO", FLAG_P, true },
    { "M", FLAG_S, false },
    { "P", FLAG_S, true },
    { "Z", FLAG_Z, false },
    { "NZ", FLAG_Z, true },
    { 0 }
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
    { 0, 0, {NULL} } // Sentinel value to mark the end of the array
};

static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);

static is_flag_true(const char *condition);
static FLAG_MAPPING get_flag_mapping(const char *condition);


/*
 *  This allows the functions for each Z80 ops found in the opcode dat files to be
 *  stored in a lookup table, which can be used to call the appropriate function with
 *  the expected number of parameters.
 * 
 *  This is only called when the opcodes are being read in from the dat files.
 */
Z80_OP_FUNC_LOOKUP get_z80_op_func(Z80_MNEMONIC op) {
    Z80_OP_FUNC_LOOKUP z80_op_func = { 0 };

    for (int i = 0; z80_op_func_lookup[i].op != 0; i++) {
        if (z80_op_func_lookup[i].op == op) {
            z80_op_func = z80_op_func_lookup[i];
        }
    }
    
    return z80_op_func;
}

/*
 *  The following functions are called by Op Codes below and performs the necessary operations
 *  by calling the commands in `execute_z80_command.c`.
 * 
 *  These functions should not call a further operand function in this file.
 */

/*
 *  This can be called by ADC, ADD, AND, CP, OR, SBC, SUB, XOR
 */
static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    libspectrum_byte operand_2_value = 0;

    /*
     *  In Z80 assembly, if only operand_1 is provided then the code assumes that the operation uses the accumulator register A.
     */
    if (operand_2 == NULL || strlen(operand_2) == 0) {
        strcpy(operand_2, operand_1);
        strcpy(operand_1, "A");
    }

    if (strlen(operand_1) == 1) {
        if (operand_1[0] != 'A') {
            ERROR("Unexpected operand 1 found, expected 'A': %s", operand_1);
            return;
        }

        if (strcmp(operand_2, "(REGISTER+dd)") == 0) {
            operand_2_value = get_DD_value();
        } else if (strcmp(operand_2, "(HL)") == 0) {
            operand_2_value = readbyte(HL);
        } else {
            operand_2_value = readbyte(PC++);

            if (strlen(operand_2) > 0) {
                WARNING("Unused operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
            }
        }

        //  Example for an opcode: op_ADC(operand_2_value);
    } else if (strlen(operand_1) == 2) {
        if (strcmp(operand_1, "HL") != 0) {
            WARNING("Unexpected operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
        }

        libspectrum_word operand_1_value = get_word_reg_value(operand_1);

        perform_contend_read_no_mreq_iterations(IR, 7);

        switch(op) {
            case ADD:
                if (strlen(operand_2) != 2) {
                    WARNING("Unexpected register in operand 2 found for ADD: %s", operand_2);
                } else {
                    _ADD16(operand_1_value, get_word_reg_value(operand_2));
                }
                break;
            case ADC:
                _ADC16(operand_1_value);
                break;
            case SBC:
                _SBC16(operand_1_value);
                break;
            default:
                ERROR("Unexpected operation found with 16-bit register operand for %s: %s", get_mnemonic_name(op), operand_1);
        }
    } else {
        ERROR("Unexpected operand 1 found for %s: %s", get_mnemonic_name(op), operand_1);
    }
}

/*
 *  This can be called by CALL, JP
 */
static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    char *condition = operand_1;
    char *offset = operand_2;

    MEMPTR_L= readbyte(PC++);
    MEMPTR_H = readbyte(PC);

    if (offset == NULL || strlen(offset) == 0 || is_flag_true(operand_1)) {
        switch(op) {
            case CALL:
                _CALL();
                break;
            case JP:
                _JP();
                break;
            default:
                ERROR("Unexpected operation called call_jp: %s", get_mnemonic_name(op));
        }
    } else {
        PC++;
    }
}

static void cpi_cpd(Z80_MNEMONIC op) {
	libspectrum_byte value = readbyte(HL);
    libspectrum_byte bytetemp = A - value;
    libspectrum_byte lookup;
    int modifier = (op == CPI) ? 1 : -1;

    lookup = ( (A & FLAG_3) >> 3 ) |
        ( (value & FLAG_3) >> 2 ) |
        ( (bytetemp & FLAG_3) >> 1 );

    for (int i = 0; i < 5; i++) {
        perform_contend_read_no_mreq(HL, 1);
    }

    HL += modifier;
	BC--;

	F = ( F & FLAG_C ) |
        ( BC ? (FLAG_V | FLAG_N) : FLAG_N ) |
	    halfcarry_sub_table[lookup] |
        ( bytetemp ? 0 : FLAG_Z ) |
	    ( bytetemp & FLAG_S );

	if (F & FLAG_H) {
        bytetemp--;
    }

	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp & BIT_1) ? FLAG_5 : 0 );
	Q = F;

    MEMPTR_W += modifier;
}

static void cpir_cpdr(Z80_MNEMONIC op) {
    libspectrum_byte value = readbyte(HL);
    libspectrum_byte bytetemp = A - value;
    libspectrum_byte lookup;
    int modifier = (op == CPIR) ? 1 : -1;

    lookup = ( (A & FLAG_3) >> 3 ) |
        ( (value & FLAG_3) >> 2 ) |
        ( (bytetemp & FLAG_3) >> 1 );

    for (int i = 0; i < 5; i++) {
        perform_contend_read_no_mreq(HL, 1);
    }

	BC--;
	F = ( F & FLAG_C ) |
        ( BC ? (FLAG_V | FLAG_N) : FLAG_N ) |
	    halfcarry_sub_table[lookup] |
        ( bytetemp ? 0 : FLAG_Z ) |
	    ( bytetemp & FLAG_S );

	if (F & FLAG_H) {
        bytetemp--;
    }

	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp & BIT_1) ? FLAG_5 : 0 );
	Q = F;

	if( ( F & ( FLAG_V | FLAG_Z ) ) == FLAG_V ) {
        for (int i = 0; i < 5; i++) {
            perform_contend_read_no_mreq(HL, 1);
        }

        PC -= 2;
	    MEMPTR_W = PC + 1;
	} else {
        MEMPTR_W += modifier;
	}

    HL += modifier;
}


static void inc_dec(Z80_MNEMONIC op, const char *operand) {
    int modifier = (op == INC) ? 1 : -1;

    if (strlen(operand) == 1) {
        _INC(get_byte_reg(operand));
    }
}

static is_flag_true(const char *condition) {
    FLAG_MAPPING flag_mapping = get_flag_mapping(condition);

    if ((flag_mapping.is_not && !(F & flag_mapping.flag)) ||
        (!flag_mapping.is_not && (F & flag_mapping.flag))) {
        return true;
    }

    return false;
}

static FLAG_MAPPING get_flag_mapping(const char *condition) {
    FLAG_MAPPING found_flag_mapping = { 0 };

    for (int i = 0; flag_lookup[i].condition != NULL; i++) {
        if (strcmp(flag_lookup[i].condition, condition) == 0) {
            found_flag_mapping = flag_lookup[i];
        }
    }
    
    return found_flag_mapping;
}

#include <string.h>
#include <stdbool.h>
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

static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);

static bool is_flag_true(const char *condition);
static FLAG_MAPPING get_flag_mapping(const char *condition);


/*
 *  The following functions are called by Op Codes below and performs the necessary operations
 *  by calling the commands in `execute_z80_command.c`.
 * 
 *  These functions should not call a further operand function in this file.
 */

/*
 *  This can be called by ADC, ADD, AND, CP, OR, SBC, SUB, XOR.
 *
 *  TODO: This will have the DD instructions removed and transferred to the CB, FD, DD and ED
 *  specific functions.
 */
static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    libspectrum_byte operand_2_value = 0;
    char *op_1 = strdup(operand_1);
    char *op_2 = strdup(operand_2);

    /*
     *  In Z80 assembly, if only operand_1 is provided then the code assumes that the operation uses the accumulator register A.
     */
    if (op_2 == NULL || strlen(op_2) == 0) {
        strcpy(op_2, op_1);
        strcpy(op_1, "A");
    }

    if (strlen(op_1) == 1) {
        if (op_1[0] != 'A') {
            ERROR("Unexpected operand 1 found, expected 'A': %s", op_1);

            free(op_1);
            free(op_2);
            return;
        }

        if (strcmp(op_2, "(REGISTER+dd)") == 0) {
            operand_2_value = get_DD_value();
        } else if (strcmp(op_2, "(HL)") == 0) {
            operand_2_value = readbyte(HL);
        } else {
            operand_2_value = readbyte(PC++);

            if (strlen(op_2) > 0) {
                WARNING("Unused operand 2 found for %s: %s", get_mnemonic_name(op), op_2);
            }
        }

        switch(op) {
            case ADD:
                _ADD(operand_2_value);
                break;
            case ADC:
                _ADC(operand_2_value);
                break;
            case AND:
                _AND(operand_2_value);
                break;
            case CP:
                _CP(operand_2_value);
                break;
            case SBC:
                _SBC(operand_2_value);
                break;
            case SUB:
                _SUB(operand_2_value);
                break;
            case OR:
                _OR(operand_2_value);
                break;
            default:
                ERROR("Unexpected operation found with register operand for %s: %s", get_mnemonic_name(op), operand_2_value);
        }
    } else if (strlen(op_1) == 2) {
        if (strcmp(op_1, "HL") != 0) {
            WARNING("Unexpected operand 2 found for %s: %s", get_mnemonic_name(op), op_2);
        }

        libspectrum_word operand_1_value = get_word_reg_value(op_1);

        perform_contend_read_no_mreq_iterations(IR, 7);

        switch(op) {
            case ADD:
                if (strlen(operand_2) != 2) {
                    WARNING("Unexpected register in operand 2 found for ADD: %s", op_2);
                } else {
                    _ADD16(operand_1_value, get_word_reg_value(op_2));
                }
                break;
            case ADC:
                _ADC16(operand_1_value);
                break;
            case SBC:
                _SBC16(operand_1_value);
                break;
            default:
                ERROR("Unexpected operation found with 16-bit register operand for %s: %s", get_mnemonic_name(op), op_1);
        }
    } else {
        ERROR("Unexpected operand 1 found for %s: %s", get_mnemonic_name(op), op_1);
    }

    free(op_1);
    free(op_2);
}

/*
 *  This can be called by CALL, JP
 */
static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    const char *condition = operand_1;
    const char *offset = operand_2;

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

/*
 *  To be completed once the shift instructions are implemented
 *
 *  TODO: This will have the DD instructions removed and transferred to the CB, FD, DD and ED
 *  specific functions.
 */
static void inc_dec(Z80_MNEMONIC op, const char *operand) {
    int modifier = (op == INC) ? 1 : -1;

    if (strlen(operand) == 1) {
        _INC(get_byte_reg(operand[0]));
    } else if (strlen(operand) == 2) {
        perform_contend_read_no_mreq(IR, 1);
        perform_contend_read_no_mreq(IR, 1);

        (*get_word_reg(operand)) += modifier;
    } else if (strcmp(operand, "(HL)") == 0) {
        libspectrum_byte bytetemp = readbyte(HL);

	    perform_contend_read_no_mreq(HL, 1);

        (op == INC) ? _INC(&bytetemp) : _DEC(&bytetemp);
	    writebyte(HL, bytetemp);
    } else if (strcmp(operand, "(REGISTER+dd)") == 0) {
        libspectrum_byte offset;
        libspectrum_byte bytetemp;

        offset = readbyte(PC);

        for (int i = 0; i < 5; i++) {
            perform_contend_read_no_mreq(PC, 1);
        }

        PC++;
        MEMPTR_W = IX + (libspectrum_signed_byte)offset;
        bytetemp = readbyte(MEMPTR_W);

        perform_contend_read_no_mreq(MEMPTR_W, 1);

        (op == INC) ? _INC(&bytetemp) : _DEC(&bytetemp);
        writebyte(MEMPTR_W, bytetemp);
    } else {
        ERROR("Unexpected operand found for %s: %s", get_mnemonic_name(op), operand);
    }
}

static bool is_flag_true(const char *condition) {
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

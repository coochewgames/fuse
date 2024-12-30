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

#include <stdio.h>
#include "execute_z80_opcode.h"

// Function implementations for opcodes with no parameters
void op_NOP(void) {
    printf("op_NOP called\n");
}

void op_RLCA(void) {
    printf("op_RLCA called\n");
}

void op_RRCA(void) {
    printf("op_RRCA called\n");
}

void op_RLA(void) {
    printf("op_RLA called\n");
}

void op_RRA(void) {
    printf("op_RRA called\n");
}

void op_DAA(void) {
    printf("op_DAA called\n");
}

void op_CPL(void) {
    printf("op_CPL called\n");
}

void op_SCF(void) {
    printf("op_SCF called\n");
}

void op_CCF(void) {
    printf("op_CCF called\n");
}

void op_HALT(void) {
    printf("op_HALT called\n");
}

void op_RET(void) {
    printf("op_RET called\n");
}

void op_NEG(void) {
    printf("op_NEG called\n");
}

void op_RETN(void) {
    printf("op_RETN called\n");
}

void op_RRD(void) {
    printf("op_RRD called\n");
}

void op_RLD(void) {
    printf("op_RLD called\n");
}

void op_LDI(void) {
    printf("op_LDI called\n");
}

void op_DI(void) {
    printf("op_DI called\n");
}

void op_EI(void) {
    printf("op_EI called\n");
}

void op_EXX(void) {
    printf("op_EXX called\n");
}

void op_CPI(void) {
    printf("op_CPI called\n");
}

void op_INI(void) {
    printf("op_INI called\n");
}

void op_OUTI(void) {
    printf("op_OUTI called\n");
}

void op_LDD(void) {
    printf("op_LDD called\n");
}

void op_CPD(void) {
    printf("op_CPD called\n");
}

void op_IND(void) {
    printf("op_IND called\n");
}

void op_OUTD(void) {
    printf("op_OUTD called\n");
}

void op_LDIR(void) {
    printf("op_LDIR called\n");
}

void op_CPIR(void) {
    printf("op_CPIR called\n");
}

void op_INIR(void) {
    printf("op_INIR called\n");
}

void op_OTIR(void) {
    printf("op_OTIR called\n");
}

void op_LDDR(void) {
    printf("op_LDDR called\n");
}

void op_CPDR(void) {
    printf("op_CPDR called\n");
}

void op_INDR(void) {
    printf("op_INDR called\n");
}

void op_OTDR(void) {
    printf("op_OTDR called\n");
}

// Function implementations for opcodes with one parameter
void op_INC(const char *value) {
    printf("op_INC called with value: %s\n", value);
}

void op_DEC(const char *value) {
    printf("op_DEC called with value: %s\n", value);
}

void op_SUB(const char *value) {
    printf("op_SUB called with value: %s\n", value);
}

void op_AND(const char *value) {
    printf("op_AND called with value: %s\n", value);
}

void op_XOR(const char *value) {
    printf("op_XOR called with value: %s\n", value);
}

void op_OR(const char *value) {
    printf("op_OR called with value: %s\n", value);
}

void op_CP(const char *value) {
    printf("op_CP called with value: %s\n", value);
}

void op_POP(const char *value) {
    printf("op_POP called with value: %s\n", value);
}

void op_PUSH(const char *value) {
    printf("op_PUSH called with value: %s\n", value);
}

void op_RST(const char *value) {
    printf("op_RST called with value: %s\n", value);
}

void op_RL(const char *value) {
    printf("op_RL called with value: %s\n", value);
}

void op_RR(const char *value) {
    printf("op_RR called with value: %s\n", value);
}

void op_SLA(const char *value) {
    printf("op_SLA called with value: %s\n", value);
}

void op_SRA(const char *value) {
    printf("op_SRA called with value: %s\n", value);
}

void op_SRL(const char *value) {
    printf("op_SRL called with value: %s\n", value);
}

void op_RLC(const char *value) {
    printf("op_RLC called with value: %s\n", value);
}

void op_RRC(const char *value) {
    printf("op_RRC called with value: %s\n", value);
}

void op_DJNZ(const char *value) {
    printf("op_DJNZ called with value: %s\n", value);
}

void op_JR(const char *value) {
    printf("op_JR called with value: %s\n", value);
}

void op_SLL(const char *value) {
    printf("op_SLL called with value: %s\n", value);
}

void op_IM(const char *value) {
    printf("op_IM called with value: %s\n", value);
}

// Function implementations for opcodes with two parameters
void op_LD(const char *value1, const char *value2) {
    printf("op_LD called with value1: %s, value2: %s\n", value1, value2);
}

void op_ADD(const char *value1, const char *value2) {
    printf("op_ADD called with value1: %s, value2: %s\n", value1, value2);
}

void op_ADC(const char *value1, const char *value2) {
    printf("op_ADC called with value1: %s, value2: %s\n", value1, value2);
}

void op_JP(const char *value1, const char *value2) {
    printf("op_JP called with value1: %s, value2: %s\n", value1, value2);
}

void op_CALL(const char *value1, const char *value2) {
    printf("op_CALL called with value1: %s, value2: %s\n", value1, value2);
}

void op_SBC(const char *value1, const char *value2) {
    printf("op_SBC called with value1: %s, value2: %s\n", value1, value2);
}

void op_EX(const char *value1, const char *value2) {
    printf("op_EX called with value1: %s, value2: %s\n", value1, value2);
}

void op_BIT(const char *value1, const char *value2) {
    printf("op_BIT called with value1: %s, value2: %s\n", value1, value2);
}

void op_RES(const char *value1, const char *value2) {
    printf("op_RES called with value1: %s, value2: %s\n", value1, value2);
}

void op_SET(const char *value1, const char *value2) {
    printf("op_SET called with value1: %s, value2: %s\n", value1, value2);
}

void op_IN(const char *value1, const char *value2) {
    printf("op_IN called with value1: %s, value2: %s\n", value1, value2);
}

void op_OUT(const char *value1, const char *value2) {
    printf("op_OUT called with value1: %s, value2: %s\n", value1, value2);
}
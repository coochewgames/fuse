#include <string.h>
#include <stdbool.h>

#include <libspectrum.h>

#include "z80.h"
#include "z80_macros.h"
#include "../memory_pages.h"

#include "z80_opcodes.h"
#include "parse_z80_operands.h"
#include "execute_z80_opcode.h"
#include "execute_z80_command.h"
#include "logging.h"


typedef enum {
    CURRENT_OP_BASE = 0,
    CURRENT_OP_FD,
    CURRENT_OP_DD,
    CURRENT_OP_ED,
    CURRENT_OP_CB,
    CURRENT_OP_FDCB,
    CURRENT_OP_DDCB
} CURRENT_OP;

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

static CURRENT_OP current_op = CURRENT_OP_BASE;


static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static void arithmetic_logical_byte(Z80_MNEMONIC op, const char *operand_2);
static void arithmetic_logical_word(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);

static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static void cpi_cpd(Z80_MNEMONIC op);
static void cpir_cpdr(Z80_MNEMONIC op);
static void inc_dec(Z80_MNEMONIC op, const char *operand);

static bool is_DDFD_op(void);
static libspectrum_byte get_byte_DDFD_value(const char *operand);
static libspectrum_word get_DDFD_word_value(const char *operand);

static bool is_flag_true(const char *condition);
static FLAG_MAPPING get_flag_mapping(const char *condition);


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

void op_SLTTRAP(void) {
    printf("op_SLTTRAP called\n");
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

/*
 *  The shift instruction utilises the parameter to determine which op codes set to utilise.
 *  DD instructions use the IX register.
 *  FD instructions use the IY register.
 *  CB instructions represent a group of extended instructions that primarily deal with bit manipulation, shifting and rotating operations.
 *  ED instructions are used for the extended instructions that do not fit into the other categories.
 *
 *  DD or FD can also shift again to use CB instructions.
 */
void op_SHIFT(const char *value) {
    if (strcmp(value, "DDFDCB") == 0) {
        //  This is only called by the DD or FD instruction set to utilise the CB instruction set.
        libspectrum_word register_value = (current_op == CURRENT_OP_DD) ? IX : IY;
        Z80_OP op;

	    perform_contend_read(PC, 3);
	    MEMPTR_W = register_value + (libspectrum_signed_byte)readbyte_internal(PC);
	    PC++;
        perform_contend_read(PC, 3);

	    libspectrum_byte opcode_id = readbyte_internal(PC);
	    perform_contend_read_no_mreq(PC, 1);
        perform_contend_read_no_mreq(PC, 1);
        PC++;

        current_op = (current_op == CURRENT_OP_DD) ? CURRENT_OP_DDCB : CURRENT_OP_FDCB;
        op = z80_ops_set[OP_SET_DDFDCB].op_codes[opcode_id];

        call_z80_op_func(op);
    } else {
	    perform_contend_read(PC, 4);

	    libspectrum_byte opcode_id = readbyte_internal(PC);
        Z80_OP op;

        PC++;
	    R++;

        if (strcmp(value, "DD") == 0) {
            current_op = CURRENT_OP_DD;
            op = z80_ops_set[OP_SET_DDFD].op_codes[opcode_id];
            set_dd_operands(op);
        } else if (strcmp(value, "FD") == 0) {
            current_op = CURRENT_OP_FD;
            op = z80_ops_set[OP_SET_DDFD].op_codes[opcode_id];
            set_dd_operands(op);
        } else if (strcmp(value, "CB") == 0) {
            current_op = CURRENT_OP_CB;
            op = z80_ops_set[OP_SET_CB].op_codes[opcode_id];
        } else if (strcmp(value, "ED") == 0) {
            current_op = CURRENT_OP_ED;
            op = z80_ops_set[OP_SET_ED].op_codes[opcode_id];
        } else {
            ERROR("Unexpected value found for op_SHIFT: %s", value);
            return;
        }

        call_z80_op_func(op);

        //  The shift is complete, so reset the current_op to the base set.
        current_op = CURRENT_OP_BASE;
    }
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

/*
 *  The following functions are called by Op Codes functions and perform the necessary operations
 *  by calling the commands in `execute_z80_command.c`.
 */

/*
 *  This can be called by ADC, ADD, AND, CP, OR, SBC, SUB, XOR.
 */
static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    char *op_1 = strdup(operand_1);
    char *op_2 = strdup(operand_2);

    /*
     *  In Z80 assembly, if only operand_1 is provided then the code assumes that the
     *  operation uses the accumulator register A.
     */
    if (op_2 == NULL || strlen(op_2) == 0) {
        strcpy(op_2, op_1);
        strcpy(op_1, "A");
    }

    /*
     *  All the operations utilise the accumulator register A as the first operand for
     *  single byte instructions.
     */
    if (strlen(op_1) == 1 && op_1[0] != 'A') {
        ERROR("Unexpected single register operand 1 found, expected 'A': %s", op_1);

        free(op_1);
        free(op_2);
        return;
    }

    if (strlen(op_1) == 1) {
        arithmetic_logical_byte(op, op_2);
    } else if (strlen(op_1) == 2) {
        arithmetic_logical_word(op, op_1, op_2);
    } else {
        ERROR("Unexpected operand 1 length found for %s: %s", get_mnemonic_name(op), op_1);
    }

    free(op_1);
    free(op_2);
}

static void arithmetic_logical_byte(Z80_MNEMONIC op, const char *operand_2) {
    libspectrum_byte operand_2_value = 0;

    if (is_DDFD_op()) {
        operand_2_value = get_byte_DDFD_value(operand_2);
    }
    else if (strlen(operand_2) == 1) {
        operand_2_value = get_byte_reg_value(operand_2);
    }
    else {
        if (strcmp(operand_2, "(HL)") == 0) {
            operand_2_value = readbyte(HL);
        } else {
            //  The "nn" operand entails that the byte value is read from the PC address.
            if (strlen(operand_2) > 0 && strcmp(operand_2, "nn") != 0) {
                WARNING("Unused operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
            }

            operand_2_value = readbyte(PC++);
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
}

static void arithmetic_logical_word(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    libspectrum_byte operand_2_value = 0;

    if (strcmp(operand_2, "HL") != 0) {
        WARNING("Expected operand 2 to be HL for %s: Found %s", get_mnemonic_name(op), operand_2);
    }

    libspectrum_word operand_1_value = get_word_reg_value(operand_1);

    perform_contend_read_no_mreq_iterations(IR, 7);

    switch(op) {
        case ADD:
            if (strlen(operand_2) != 2) {
                WARNING("Unexpected register in operand 2 found for ADD16: %s", operand_2);
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
}

/*
 *  This can be called by CALL, JP.
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

/*
 *  This can be called by CPI, CPD.
 */
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

/*
 *  This can be called by CPIR, CPDR.
 */
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
 *  This can be called by INC, DEC.
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

static bool is_DDFD_op(void) {
    return (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB ||
            current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB);
}

static libspectrum_byte get_byte_DDFD_value(const char *operand) {
    libspectrum_byte value = 0;

    if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
        if (strcmp(operand, "REGISTERL") == 0) {
            value = IXL;
        } else if (strcmp(operand, "REGISTERH") == 0) {
            value = IXH;
        } else if (strcmp(operand, "(REGISTER+dd)") == 0) {
            value = get_dd_offset_value(IX);
        } else {
            ERROR("Unexpected DD operand found: %s", operand);
        }
    } else if (current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB) {
        if (strcmp(operand, "REGISTERL") == 0) {
            value = IYL;
        } else if (strcmp(operand, "REGISTERH") == 0) {
            value = IYH;
        } else if (strcmp(operand, "(REGISTER+dd)") == 0) {
            value = get_dd_offset_value(IY);
        } else {
            ERROR("Unexpected FD operand found: %s", operand);
        }
    } else {
        ERROR("Unexpected current_op found: %d", current_op);
    }

    return value;
}

static libspectrum_word get_DDFD_word_value(const char *operand) {
    libspectrum_word value = 0;

    if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
        if (strcmp(operand, "REGISTER") == 0) {
            value = IX;
        } else {
            ERROR("Unexpected DD register found: %s", operand);
        }
    } else if (current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB) {
        if (strcmp(operand, "REGISTER") == 0) {
            value = IY;
        } else {
            ERROR("Unexpected FD register found: %s", operand);
        }
    } else {
        ERROR("Unexpected current_op found: %d", current_op);
    }

    return value;
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

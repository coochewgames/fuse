#include <string.h>
#include <libspectrum.h>

#include "z80.h"
#include "spectrum.h"
#include "z80_macros.h"
#include "../memory_pages.h"
#include "../peripherals/ula.h"

#include "parse_z80_operands.h"
#include "execute_z80_opcode.h"
#include "logging.h"


static void arithmetic_logical(char *opcode, char *operand_1, char *operand_2);
static void op_ADD16(libspectrum_dword value1, libspectrum_dword value2);


/*
 *  This allows the function lookups to be stored in a lookup table,
 *  which can be used to call the appropriate function for a Z80 opcode with the expected
 *  number of parameters.
 */
Z80_OP_FUNC_LOOKUP z80_op_func_lookup(Z80_MNEMONIC op) {
    Z80_OP_FUNC_LOOKUP lookup = {0};

    switch (op) {
        case ADC:
            lookup.function_type = OP_TYPE_ONE_PARAM;
            lookup.func.one_param = op_ADC;
            break;

        case ADD:
            lookup.function_type = OP_TYPE_ONE_PARAM;
            lookup.func.one_param = op_ADD;
            break;
            
        default:
            ERROR("Unknown Z80 opcode found: %s", getMnemonicName(op));
            break;
    }

    return lookup;
}

void op_ADC(libspectrum_byte value) {
    libspectrum_word adctemp = A + value + ( F & FLAG_C );
    libspectrum_byte lookup = ( (A & 0x88) >> 3 ) | ( (value & 0x88 ) >> 2 ) | ( (adctemp & 0x88) >> 1 );

    A = adctemp;
    F = ( (adctemp & 0x100) ? FLAG_C : 0 ) |
        halfcarry_add_table[lookup & 0x07] |
        overflow_add_table[lookup >> 4] |
        sz53_table[A];

    Q = F;
}

void op_ADD(libspectrum_byte value) {
    libspectrum_word addtemp = A + value;
    libspectrum_byte lookup = ( (A & 0x88) >> 3 ) | ( (value & 0x88 ) >> 2 ) | ( (addtemp & 0x88) >> 1 );

    A = addtemp;
    F = ( (addtemp & 0x100) ? FLAG_C : 0 ) |
        halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] |
        sz53_table[A];

    Q = F;
}

static void arithmetic_logical(char *opcode, char *operand_1, char *operand_2) {
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
                WARNING("Unused operand 2 found: %s", operand_2);
            }
        }

        //  Example for an opcode: op_ADC(operand_2_value);
    } else if (strlen(operand_1) == 2) {
        if (strcmp(operand_1, "HL") != 0) {
            WARNING("Unexpected operand 2 found: %s", operand_2);
        }

        libspectrum_word operand_1_value = get_reg_word_value(operand_1);

        perform_contend_read_no_mreq_iterations(IR, 7);

        if (strcmp(opcode, "ADD") == 0) {
            if (strlen(operand_2) != 2) {
                WARNING("Unexpected operand 2 found: %s", operand_2);
            } else {
                libspectrum_word operand_2_value = get_reg_word_value(operand_2);

                //  Example for an opcode: op_ADD16(operand_1_value, operand_2_value);
            }
        } else {
            //  Example for an opcode: op_<opcode>16(operand_1_value);
        }
    } else {
        ERROR("Unexpected operand 1 found: %s", operand_1);
    }
}

/*
 *  This function is used to add two 16-bit values together; the 16 bit register values
 *  are always represented by two 8-bit registers concatenated together.
 */
static void op_ADD16(libspectrum_dword value1, libspectrum_dword value2) {
    libspectrum_dword add16temp = value1 + value2;
    libspectrum_byte lookup = ( (value1 & 0x0800) >> 11 ) |
        ( (value2 & 0x0800 ) >> 10 ) |
        ( (add16temp & 0x0800) >>  9 );

    z80.memptr.w = value1 + 1;
    value1 = add16temp;

    F = ( F & (FLAG_V | FLAG_Z | FLAG_S) ) |
        ( (add16temp & 0x10000) ? FLAG_C : 0 )|
        ( (add16temp >> 8) & ( FLAG_3 | FLAG_5 ) ) |
        halfcarry_add_table[lookup];

    Q = F;
}

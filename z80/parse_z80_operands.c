#include "parse_z80_operands.h"

#include "z80.h"
#include "z80_macros.h"
#include "logging.h"


libspectrum_byte get_DD_value(void) {
    libspectrum_byte offset = readbyte( PC );

    perform_contend_read_no_mreq_iterations(PC, 5);

    PC++;
	MEMPTR_W = IX + (libspectrum_signed_byte)offset;  // IX is a 16-bit register used by DD prefixed instructions
	return readbyte(MEMPTR_W);
}

/*
 *  This will retrieve the value of a byte operand;
 *  the operand can be a register or a literal hex value.
 */
libspectrum_byte get_byte_value(char *operand) {
    libspectrum_byte value = 0;

    if (strlen(operand) == 1) {
        value = get_byte_reg_value(operand[0]);
    } else {
        value = (libspectrum_byte)strtol(operand, NULL, 16);
    }
    
    return value;
}

/*
 *  Use the macro for a byte register from the character name.
 */
libspectrum_byte get_byte_reg_value(char reg) {
    libspectrum_byte value = 0;

    switch (reg) {
        case 'A':
            value = A;
            break;
        case 'F':
            value = F;
            break;
        case 'B':
            value = B;
            break;
        case 'C':
            value = C;
            break;
        case 'D':
            value = D;
            break;
        case 'E':
            value = E;
            break;
        case 'H':
            value = H;
            break;
        case 'L':
            value = L;
            break;
        default:
            ERROR("Unexpected byte register found: %c", reg);
            break;
        }

    return value;
}

libspectrum_byte *get_byte_reg(char reg) {
    libspectrum_byte *value = NULL;

    switch (reg) {
        case 'A':
            value = &A;
            break;
        case 'F':
            value = &F;
            break;
        case 'B':
            value = &B;
            break;
        case 'C':
            value = &C;
            break;
        case 'D':
            value = &D;
            break;
        case 'E':
            value = &E;
            break;
        case 'H':
            value = &H;
            break;
        case 'L':
            value = &L;
            break;
        default:
            ERROR("Unexpected byte register found: %c", reg);
            break;
        }

    return value;
}

/*
 *  Use the macro for a word register from the string name.
 */
libspectrum_word get_word_reg_value(const char *reg) {
    libspectrum_word value = 0;

    if (strcmp(reg, "AF") == 0) {
        value = AF;
    } else if (strcmp(reg, "BC") == 0) {
        value = BC;
    } else if (strcmp(reg, "DE") == 0) {
        value = DE;
    } else if (strcmp(reg, "HL") == 0) {
        value = HL;
    } else if (strcmp(reg, "IX") == 0) {
        value = IX;
    } else if (strcmp(reg, "IY") == 0) {
        value = IY;
    } else if (strcmp(reg, "SP") == 0) {
        value = SP;
    } else {
        ERROR("Unexpected word register found: %s", reg);
    }

    return value;
}

libspectrum_word *get_word_reg(const char *reg) {
    libspectrum_word *value = NULL;

    if (strcmp(reg, "AF") == 0) {
        value = &AF;
    } else if (strcmp(reg, "BC") == 0) {
        value = &BC;
    } else if (strcmp(reg, "DE") == 0) {
        value = &DE;
    } else if (strcmp(reg, "HL") == 0) {
        value = &HL;
    } else if (strcmp(reg, "IX") == 0) {
        value = &IX;
    } else if (strcmp(reg, "IY") == 0) {
        value = &IY;
    } else if (strcmp(reg, "SP") == 0) {
        value = &SP;
    } else {
        ERROR("Unexpected word register found: %s", reg);
    }

    return value;
}

/*
 *  Parase an address operand, which can be a double register, a literal address or an indirect address that may have an offset.
 */
ADDRESS_OPERAND parse_address_operand(const char *operand) {
    ADDRESS_OPERAND result = { false, "", 0, 0 };
    char *open_parenthesis = strchr(operand, '(');
    char *close_parenthesis = strchr(operand, ')');

    if (open_parenthesis && close_parenthesis && close_parenthesis > open_parenthesis) {
        result.is_indirect = true;

        // Extract the register and offset
        char indirect_address[20];

        strncpy(indirect_address, open_parenthesis + 1, (close_parenthesis - open_parenthesis - 1));
        indirect_address[close_parenthesis - open_parenthesis - 1] = '\0';

        if (strncmp(indirect_address, "0x", 2) == 0) {
            result.address = (libspectrum_word)strtol(indirect_address, NULL, 16);
        } else {
            // Check if there is an offset
            char *plus_sign = strchr(indirect_address, '+');

            if (plus_sign) {
                strncpy(result.reg, indirect_address, plus_sign - indirect_address);

                result.reg[plus_sign - indirect_address] = '\0';
                result.offset = atoi(plus_sign + 1);
            } else {
                strcpy(result.reg, indirect_address);
            }
        }
    } else {
        if (strncmp(operand, "0x", 2) == 0) {
            result.address = (libspectrum_word)strtol(operand, NULL, 16);
        } else if (strlen(operand) == 2) {
            // No parentheses, just copy the double register name
            strcpy(result.reg, operand);
        } else {
            ERROR("Unexpected address operand found: %s", operand);
        }
    }

    return result;
}

/*
 *  Perform a read contention without a memory request for a number of iterations.
 */
void perform_contend_read_no_mreq_iterations(libspectrum_word address, int iterations) {
    for(int i = 0; i < iterations; i++) {
        perform_contend_read_no_mreq(address, 1);
    }
}

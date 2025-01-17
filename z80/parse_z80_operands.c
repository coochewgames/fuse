#include "parse_z80_operands.h"

#include "z80.h"
#include "z80_macros.h"
#include "logging.h"


#define INDIRECT_WORD_OPERAND_LEN 4

// value = (libspectrum_byte)strtol(operand, NULL, 16); to be used for getting hex value


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

        //  The I and R registers are special case registers that are not part of the main register set.
        case 'I':
            value = I;
            break;
        case 'R':
            value = (libspectrum_byte)R;  // Cast to ensure lower byte is used as this 8 bit register has been defined as a word
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

        //  The I and R registers are special case registers that are not part of the main register set.
        case 'I':
            value = &I;
            break;
        case 'R':
            value = (libspectrum_byte *)&R;  // Cast to ensure lower byte is used as this 8 bit register has been defined as a word
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
 *  Return the word register from an indirect word operand.
 */
char *get_indirect_word_reg(const char *operand) {
    char *word_reg = NULL;

    if (strlen(operand) == INDIRECT_WORD_OPERAND_LEN &&
        operand[0] == '(' && operand[INDIRECT_WORD_OPERAND_LEN - 1] == ')') {
        word_reg = operand + 1;
    }

    return word_reg;
}

/*
 *  Perform a read contention without a memory request for a number of iterations.
 */
void perform_contend_read_no_mreq_iterations(libspectrum_word address, int iterations) {
    for(int i = 0; i < iterations; i++) {
        perform_contend_read_no_mreq(address, 1);
    }
}

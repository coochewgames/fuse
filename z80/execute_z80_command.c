#include <string.h>
#include <libspectrum.h>

#include "z80.h"
#include "z80_macros.h"

#include "parse_z80_operands.h"
#include "../logging.h"


void _ADC(libspectrum_byte value) {
    libspectrum_word adctemp = A + value + ( F & FLAG_C );
    libspectrum_byte lookup = ( (A & 0x88) >> 3 ) | ( (value & 0x88 ) >> 2 ) | ( (adctemp & 0x88) >> 1 );

    A = adctemp;
    F = ( (adctemp & 0x100) ? FLAG_C : 0 ) |
        halfcarry_add_table[lookup & 0x07] |
        overflow_add_table[lookup >> 4] |
        sz53_table[A];

    Q = F;
}

void _ADD(libspectrum_byte value) {
    libspectrum_word addtemp = A + value;
    libspectrum_byte lookup = ( (A & 0x88) >> 3 ) | ( (value & 0x88 ) >> 2 ) | ( (addtemp & 0x88) >> 1 );

    A = addtemp;
    F = ( (addtemp & 0x100) ? FLAG_C : 0 ) |
        halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] |
        sz53_table[A];

    Q = F;
}

/*
 *  This function is used to add two 16-bit values together; the 16 bit register values
 *  are always represented by two 8-bit registers concatenated together.
 */
void _ADD16(libspectrum_dword value1, libspectrum_dword value2) {
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

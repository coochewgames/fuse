#include <string.h>

#ifdef CORETEST
#include "coretest.h"
#endif

#include "libspectrum.h"

#include "z80.h"
#include "z80_macros.h"
#include "parse_z80_operands.h"
#include "execute_z80_command.h"

#include "../periph.h"  // Rerquired for readport
#include "../memory_pages.h"
#include "../logging.h"

#define FLAG_C_MASK 0x100
#define FLAG_C_MASK_16 0x10000
#define FLAG_H_MASK 0x88
#define FLAG_HL_MASK 0x8800

#define FLAG_7 0x80     // Bit 7 of the result
#define FLAG_11 0x0800  // Bit 11 of the result

#define LOWER_THREE_BITS_MASK 0x07
#define LOWER_NIBBLE_MASK 0x0F
#define HIGHER_NIBBLE_MASK 0xF0
#define HALF_CARRY_MASK 0x0F
#define OVERFLOW_MASK 0x7F


void _AND(libspectrum_byte value) {
    A &= value;
    F = FLAG_H | sz53p_table[A];
    Q = F;
}

void _ADC(libspectrum_byte value) {
    libspectrum_word adctemp = A + value + ( F & FLAG_C );
    libspectrum_byte lookup = ( (A & FLAG_H_MASK) >> 3 ) | 
                              ( (value & FLAG_H_MASK) >> 2 ) | 
                              ( (adctemp & FLAG_H_MASK) >> 1 );

    A = adctemp;
    F = ( (adctemp & FLAG_C_MASK) ? FLAG_C : 0 ) |
        halfcarry_add_table[lookup & LOWER_THREE_BITS_MASK] |
        overflow_add_table[lookup >> 4] |
        sz53_table[A];
    Q = F;
}

void _ADC16(libspectrum_word value) {
    libspectrum_dword add16temp = HL + value + (F & FLAG_C);
    libspectrum_byte lookup = ((HL & FLAG_HL_MASK) >> 11) |
        ( (value & FLAG_HL_MASK) >> 10 ) |
        ( (add16temp & FLAG_HL_MASK) >> 9 );

    MEMPTR_W = HL + 1;

    HL = add16temp;
    F = ( (add16temp & FLAG_C_MASK_16) ? FLAG_C : 0 ) |
        overflow_add_table[lookup >> 4] |
        ( H & (FLAG_3 | FLAG_5 | FLAG_S) ) |
        halfcarry_add_table[lookup & LOWER_THREE_BITS_MASK] |
        ( HL ? 0 : FLAG_Z );
    Q = F;
}

void _ADD(libspectrum_byte value) {
    libspectrum_word addtemp = A + value;
    libspectrum_byte lookup = ( (A & FLAG_H_MASK) >> 3 ) |
        ( (value & FLAG_H_MASK ) >> 2 ) |
        ( (addtemp & FLAG_H_MASK) >> 1 );

    A = addtemp;
    F = ( (addtemp & FLAG_C_MASK) ? FLAG_C : 0 ) |
        halfcarry_add_table[lookup & LOWER_THREE_BITS_MASK] |
        overflow_add_table[lookup >> 4] |
        sz53_table[A];

    Q = F;
}

/*
 *  This function is used to add two 16-bit values together; the 16 bit register values
 *  are always represented by two 8-bit registers concatenated together.
 */
void _ADD16(libspectrum_word value1, libspectrum_word value2) {
    libspectrum_dword add16temp = value1 + value2;
    libspectrum_byte lookup = ( (value1 & FLAG_11) >> 11 ) |
        ( (value2 & FLAG_11 ) >> 10 ) |
        ( (add16temp & FLAG_11) >>  9 );

    MEMPTR_W = value1 + 1;
    value1 = add16temp;

    F = ( F & (FLAG_V | FLAG_Z | FLAG_S) ) |
        ( (add16temp & FLAG_C_MASK_16) ? FLAG_C : 0 )|
        ( (add16temp >> 8) & ( FLAG_3 | FLAG_5 ) ) |
        halfcarry_add_table[lookup];

    Q = F;
}

void _BIT(libspectrum_byte bit_position, libspectrum_byte value) {
    F = (F & FLAG_C) | FLAG_H | (value & (FLAG_3 | FLAG_5));

    if (!(value & (0x01 << bit_position))) {
        F |= FLAG_P | FLAG_Z;
    }

    if (bit_position == 7 && (value & FLAG_S)) {
        F |= FLAG_S;
    }

    Q = F;
}

void _BIT_MEMPTR(libspectrum_byte bit_position, libspectrum_byte value) {
    F = (F & FLAG_C) | FLAG_H | ( MEMPTR_H & (FLAG_3 | FLAG_5) );

    if (!(value & (0x01 << bit_position))) {
        F |= FLAG_P | FLAG_Z;
    }

    if (bit_position == 7 && (value & FLAG_S)) {
        F |= FLAG_S;
    }

    Q = F;
}

void _CALL(void) {
    perform_contend_read_no_mreq(PC, 1);

    PC++;
    _PUSH16(PCL, PCH);

    PC = MEMPTR_W;
}

void _CP(libspectrum_byte value) {
    libspectrum_word cptemp = A - value;
    libspectrum_byte lookup = ( (A & FLAG_H_MASK) >> 3 ) |
        ( (value & FLAG_H_MASK) >> 2 ) |
        ( (cptemp & FLAG_H_MASK) >> 1 );

    F = ( (cptemp & FLAG_C_MASK) ? FLAG_C : (cptemp ? 0 : FLAG_Z) ) |
        FLAG_N |
        halfcarry_sub_table[lookup & LOWER_THREE_BITS_MASK] |
        overflow_sub_table[lookup >> 4] |
        (value & (FLAG_3 | FLAG_5)) |
        (cptemp & FLAG_S);
    Q = F;
}

/*
 *  Have removed the _DDFD_CB_ROTATESHIFT function as it is not used in the codebase.
 *  It required a tempaddr variable that is not defined.
 */

void _DEC(libspectrum_byte *value) {
    F = (F & FLAG_C) | ( (*value & HALF_CARRY_MASK) ? 0 : FLAG_H ) | FLAG_N;
    (*value)--;

    F |= ( (*value == OVERFLOW_MASK) ? FLAG_V : 0 ) | sz53_table[*value];
    Q = F;
}

void _Z80_IN(libspectrum_byte *reg, libspectrum_word port) {
    MEMPTR_W = port + 1;
    *reg = readport(port);

    F = (F & FLAG_C) | sz53p_table[*reg];
    Q = F;
}

void _INC(libspectrum_byte *value) {
    (*value)++;

    F = (F & FLAG_C) |                      // Preserve the Carry flag
        ( (*value == 0x80) ? FLAG_V : 0 ) | // Set the Overflow flag if the value is 0x80
        ( (*value & LOWER_NIBBLE_MASK) ? 0 : FLAG_H ) |  // Set the Half Carry flag if the lower nibble is 0
        sz53_table[*value];                 // Update the Sign, Zero, and Parity flags
    Q = F;
}

void _LD16_NNRR(libspectrum_byte regl, libspectrum_byte regh) {
    libspectrum_word ldtemp;

    ldtemp = readbyte(PC++);
    ldtemp |= readbyte(PC++) << 8;

    writebyte(ldtemp++, regl);

    MEMPTR_W = ldtemp;
    writebyte(ldtemp, regh);
}

void _LD16_RRNN(libspectrum_byte *regl, libspectrum_byte *regh) {
    libspectrum_word ldtemp;

    ldtemp = readbyte(PC++);
    ldtemp |= readbyte(PC++) << 8;

    *regl = readbyte(ldtemp++);
    MEMPTR_W = ldtemp;
    *regh = readbyte(ldtemp);
}

void _JP(void) {
    PC = MEMPTR_W;

    DEBUG("PC set to 0x%04X", PC);
}

void _JR(void) {
    //  The byte value read can be negative, so we need to cast it to a signed byte
    libspectrum_signed_byte jrtemp = readbyte(PC);

    for (int i = 0; i < 5; i++) {
        perform_contend_read_no_mreq(PC, 1);
    }

    PC += jrtemp;
    PC++;
    MEMPTR_W = PC;

    DEBUG("PC relative jump of %d to 0x%04X", (int)jrtemp, PC);
}

void _OR(libspectrum_byte value) {
    A |= value;
    F = sz53p_table[A];
    Q = F;
}

void _POP16(libspectrum_byte *regl, libspectrum_byte *regh) {
    *regl = readbyte(SP++);
    *regh = readbyte(SP++);
}

void _PUSH16(libspectrum_byte regl, libspectrum_byte regh) {
    writebyte(--SP, regh);
    writebyte(--SP, regl);
}

void _RET(void) {
    _POP16(&PCL, &PCH);
    MEMPTR_W = PC;
}

void _RL(libspectrum_byte *value) {
    libspectrum_byte rltemp = *value;

    *value = (*value << 1) | (F & FLAG_C);

    F = (rltemp >> 7) | sz53p_table[*value];
    Q = F;
}

void _RLC(libspectrum_byte *value) {
    *value = (*value << 1) | (*value >> 7);

    F = (*value & FLAG_C) | sz53p_table[*value];
    Q = F;
}

void _RR(libspectrum_byte *value) {
    libspectrum_byte rrtemp = *value;

    *value = (*value >> 1) | (F << 7);
    F = (rrtemp & FLAG_C) | sz53p_table[*value];
    Q = F;
}

void _RRC(libspectrum_byte *value) {
    F = *value & FLAG_C;
    *value = (*value >> 1) | (*value << 7);

    F |= sz53p_table[*value];
    Q = F;
}

void _RST(libspectrum_byte value) {
    _PUSH16(PCL, PCH);

    PC = value;
    MEMPTR_W = PC;
}

void _SBC(libspectrum_byte value) {
    libspectrum_word sbctemp = A - value - (F & FLAG_C);
    libspectrum_byte lookup = ( (A & FLAG_H_MASK) >> 3 ) |
        ( (value & FLAG_H_MASK) >> 2 ) |
        ( (sbctemp & FLAG_H_MASK) >> 1 );

    A = sbctemp;
    F = ( (sbctemp & FLAG_C_MASK) ? FLAG_C : 0 ) |
        FLAG_N |
        halfcarry_sub_table[lookup & LOWER_THREE_BITS_MASK] |
        overflow_sub_table[lookup >> 4] |
        sz53_table[A];
    Q = F;
}

void _SBC16(libspectrum_word value) {
    libspectrum_dword sub16temp = HL - value - (F & FLAG_C);
    libspectrum_byte lookup = ( (HL & FLAG_HL_MASK) >> 11 ) |
        ( (value & FLAG_HL_MASK) >> 10 ) |
        ( (sub16temp & FLAG_HL_MASK) >> 9 );

    MEMPTR_W = HL + 1;
    HL = sub16temp;
    F = ( (sub16temp & FLAG_C_MASK_16) ? FLAG_C : 0 ) |
        FLAG_N |
        overflow_sub_table[lookup >> 4] |
        ( H & (FLAG_3 | FLAG_5 | FLAG_S) ) |
        halfcarry_sub_table[lookup & LOWER_THREE_BITS_MASK] |
        ( HL ? 0 : FLAG_Z );
    Q = F;
}

void _SLA(libspectrum_byte *value) {
    F = *value >> 7;
    *value <<= 1;

    F |= sz53p_table[*value];
    Q = F;
}

void _SLL(libspectrum_byte *value) {
    F = *value >> 7;
    *value = (*value << 1) | 0x01;

    F |= sz53p_table[*value];
    Q = F;
}

void _SRA(libspectrum_byte *value) {
    F = *value & FLAG_C;
    *value = (*value & FLAG_7) | (*value >> 1);

    F |= sz53p_table[*value];
    Q = F;
}

void _SRL(libspectrum_byte *value) {
    F = *value & FLAG_C;
    *value >>= 1;

    F |= sz53p_table[*value];
    Q = F;
}

void _SUB(libspectrum_byte value) {
    libspectrum_word subtemp = A - value;
    libspectrum_byte lookup = ( (A & FLAG_H_MASK) >> 3 ) |
        ( (value & FLAG_H_MASK) >> 2 ) |
        ( (subtemp & FLAG_H_MASK) >>  1);

    A = subtemp;
    F = ( (subtemp & FLAG_C_MASK) ? FLAG_C : 0 ) |
        FLAG_N |
        halfcarry_sub_table[lookup & LOWER_THREE_BITS_MASK] |
        overflow_sub_table[lookup >> 4] |
        sz53_table[A];
    Q = F;
}

void _XOR(libspectrum_byte value) {
    A ^= value;
    F = sz53p_table[A];
    Q = F;
}

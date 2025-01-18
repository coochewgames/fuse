/* z80_macros.h: Some commonly used z80 things as macros
   Copyright (c) 1999-2011 Philip Kendall
   Copyright (c) 2015 Stuart Brady

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

    Author contact information: philip-fuse@shadowmagic.org.uk
    Simplication refactoring by Roddy McNeill: rd.mcn@ccgapps.net.au
*/

#ifndef FUSE_Z80_MACROS_H
#define FUSE_Z80_MACROS_H

/* Macros used for accessing the registers */

/* Accumulator and Flags register pair */
#define A   z80.af.b.h  // High byte of AF register pair (Accumulator)
#define F   z80.af.b.l  // Low byte of AF register pair (Flags)
#define AF  z80.af.w    // Whole AF register pair

/* BC register pair */
#define B   z80.bc.b.h  // High byte of BC register pair
#define C   z80.bc.b.l  // Low byte of BC register pair
#define BC  z80.bc.w    // Whole BC register pair

/* DE register pair */
#define D   z80.de.b.h  // High byte of DE register pair
#define E   z80.de.b.l  // Low byte of DE register pair
#define DE  z80.de.w    // Whole DE register pair

/* HL register pair */
#define H   z80.hl.b.h  // High byte of HL register pair
#define L   z80.hl.b.l  // Low byte of HL register pair
#define HL  z80.hl.w    // Whole HL register pair

/* Alternate Accumulator and Flags register pair */
#define A_  z80.af_.b.h // High byte of alternate AF register pair (Accumulator)
#define F_  z80.af_.b.l // Low byte of alternate AF register pair (Flags)
#define AF_ z80.af_.w   // Whole alternate AF register pair

/* Alternate BC register pair */
#define B_  z80.bc_.b.h // High byte of alternate BC register pair
#define C_  z80.bc_.b.l // Low byte of alternate BC register pair
#define BC_ z80.bc_.w   // Whole alternate BC register pair

/* Alternate DE register pair */
#define D_  z80.de_.b.h // High byte of alternate DE register pair
#define E_  z80.de_.b.l // Low byte of alternate DE register pair
#define DE_ z80.de_.w   // Whole alternate DE register pair

/* Alternate HL register pair */
#define H_  z80.hl_.b.h // High byte of alternate HL register pair
#define L_  z80.hl_.b.l // Low byte of alternate HL register pair
#define HL_ z80.hl_.w   // Whole alternate HL register pair

/* IX index register */
#define IXH z80.ix.b.h  // High byte of IX register
#define IXL z80.ix.b.l  // Low byte of IX register
#define IX  z80.ix.w    // Whole IX register

/* IY index register */
#define IYH z80.iy.b.h  // High byte of IY register
#define IYL z80.iy.b.l  // Low byte of IY register
#define IY  z80.iy.w    // Whole IY register

/* Stack Pointer register */
#define SPH z80.sp.b.h  // High byte of SP register
#define SPL z80.sp.b.l  // Low byte of SP register
#define SP  z80.sp.w    // Whole SP register

/* Program Counter register */
#define PCH z80.pc.b.h  // High byte of PC register
#define PCL z80.pc.b.l  // Low byte of PC register
#define PC  z80.pc.w    // Whole PC register

/* Interrupt and Refresh registers */
#define I  z80.i        // Interrupt register
#define R  z80.r        // Refresh register
#define R7 z80.r7       // 7th bit of the Refresh register

/* Clock registers */
#define CLOCKH  z80.clockh // High byte of clock register
#define CLOCKL  z80.clockl // Low byte of clock register

/* Interrupt Flip-Flops */
#define IFF1  z80.iff1   // Interrupt Flip-Flop 1
#define IFF2  z80.iff2   // Interrupt Flip-Flop 2
#define IMODE z80.im     // Interrupt Mode (named IMODE to avoid conflict with IM opcode in ED set)

/* Memory pointer */
#define MEMPTR_H z80.memptr.b.h   // High byte of the memory pointer register
#define MEMPTR_L z80.memptr.b.l   // Low byte of the memory pointer register
#define MEMPTR_W z80.memptr.w     // Whole memory pointer register

#define HALTED z80.halted  // Halted flag

// Combined Interrupt and Refresh register
#define IR ( ( z80.i ) << 8 | ( z80.r7 & 0x80 ) | ( z80.r & 0x7f ) )

#define Q z80.q  // Q register, used to store the state of flags affected by the last executed instruction

// The flags
#define FLAG_C  0x01    // Carry flag
#define FLAG_N  0x02    // Add/Subtract flag
#define FLAG_P  0x04    // Parity/Overflow flag
#define FLAG_V  FLAG_P  // Overflow flag (same as Parity flag)
#define FLAG_3  0x08    // Bit 3 of the result (undocumented flag)
#define FLAG_H  0x10    // Half Carry flag
#define FLAG_5  0x20    // Bit 5 of the result (undocumented flag)
#define FLAG_Z  0x40    // Zero flag
#define FLAG_S  0x80    // Sign flag

// Bits
#define BIT_0  0x01
#define BIT_1  0x02
#define BIT_2  0x04
#define BIT_3  0x08
#define BIT_4  0x10
#define BIT_5  0x20
#define BIT_6  0x40
#define BIT_7  0x80

#endif		/* #ifndef FUSE_Z80_MACROS_H */

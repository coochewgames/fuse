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
#define IMODE z80.im     // Interrupt Mode (named IMODE to avoid conflict with IM opcode)

/* Memory pointer */
#define MEMPTR_H z80.memptr.b.h   // High byte of the memory pointer register
#define MEMPTR_L z80.memptr.b.l   // Low byte of the memory pointer register
#define MEMPTR_W z80.memptr.w     // Whole memory pointer register

#define HALTED z80.halted  // Halted flag

// Combined Interrupt and Refresh register
#define IR ( ( z80.i ) << 8 | ( z80.r7 & 0x80 ) | ( z80.r & 0x7f ) )

#define IS_CMOS settings_current.z80_is_cmos  // CMOS flag

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

/* Get the appropriate contended memory delay. Use a macro for performance
   reasons in the main core, but a function for flexibility when building
   the core tester */

#ifndef CORETEST

#define contend_read(address,time) \
  if( memory_map_read[ (address) >> MEMORY_PAGE_SIZE_LOGARITHM ].contended ) \
    tstates += ula_contention[ tstates ]; \
  tstates += (time);

#define contend_read_no_mreq(address,time) \
  if( memory_map_read[ (address) >> MEMORY_PAGE_SIZE_LOGARITHM ].contended ) \
    tstates += ula_contention_no_mreq[ tstates ]; \
  tstates += (time);

#define contend_write_no_mreq(address,time) \
  if( memory_map_write[ (address) >> MEMORY_PAGE_SIZE_LOGARITHM ].contended ) \
    tstates += ula_contention_no_mreq[ tstates ]; \
  tstates += (time);

#else				/* #ifndef CORETEST */

void contend_read( libspectrum_word address, libspectrum_dword time );
void contend_read_no_mreq( libspectrum_word address, libspectrum_dword time );
void contend_write_no_mreq( libspectrum_word address, libspectrum_dword time );

#endif				/* #ifndef CORETEST */

/* Some commonly used instructions */
#define AND(value)\
{\
  A &= (value);\
  F = FLAG_H | sz53p_table[A];\
  Q = F;\
}

#define ADC(value)\
{\
  libspectrum_word adctemp = A + (value) + ( F & FLAG_C ); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( ( adctemp & 0x88 ) >> 1 );  \
  A=adctemp;\
  F = ( adctemp & 0x100 ? FLAG_C : 0 ) |\
    halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] |\
    sz53_table[A];\
  Q = F;\
}

#define ADC16(value)\
{\
  libspectrum_dword add16temp= HL + (value) + ( F & FLAG_C ); \
  libspectrum_byte lookup = ( (        HL & 0x8800 ) >> 11 ) | \
			    ( (   (value) & 0x8800 ) >> 10 ) | \
			    ( ( add16temp & 0x8800 ) >>  9 );  \
  z80.memptr.w=HL+1;\
  HL = add16temp;\
  F = ( add16temp & 0x10000 ? FLAG_C : 0 )|\
    overflow_add_table[lookup >> 4] |\
    ( H & ( FLAG_3 | FLAG_5 | FLAG_S ) ) |\
    halfcarry_add_table[lookup&0x07]|\
    ( HL ? 0 : FLAG_Z );\
  Q = F;\
}

#define ADD(value)\
{\
  libspectrum_word addtemp = A + (value); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( ( addtemp & 0x88 ) >> 1 );  \
  A=addtemp;\
  F = ( addtemp & 0x100 ? FLAG_C : 0 ) |\
    halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] |\
    sz53_table[A];\
  Q = F;\
}

#define ADD16(value1,value2)\
{\
  libspectrum_dword add16temp = (value1) + (value2); \
  libspectrum_byte lookup = ( (  (value1) & 0x0800 ) >> 11 ) | \
			    ( (  (value2) & 0x0800 ) >> 10 ) | \
			    ( ( add16temp & 0x0800 ) >>  9 );  \
  z80.memptr.w=(value1)+1;\
  (value1) = add16temp;\
  F = ( F & ( FLAG_V | FLAG_Z | FLAG_S ) ) |\
    ( add16temp & 0x10000 ? FLAG_C : 0 )|\
    ( ( add16temp >> 8 ) & ( FLAG_3 | FLAG_5 ) ) |\
    halfcarry_add_table[lookup];\
  Q = F;\
}

/* This may look fairly inefficient, but the (gcc) optimiser does the
   right thing assuming it's given a constant for 'bit' */
#define BIT( bit, value ) \
{ \
  F = ( F & FLAG_C ) | FLAG_H | ( value & ( FLAG_3 | FLAG_5 ) ); \
  if( ! ( (value) & ( 0x01 << (bit) ) ) ) F |= FLAG_P | FLAG_Z; \
  if( (bit) == 7 && (value) & 0x80 ) F |= FLAG_S; \
  Q = F;\
}

#define BIT_MEMPTR( bit, value ) \
{ \
  F = ( F & FLAG_C ) | FLAG_H | ( z80.memptr.b.h & ( FLAG_3 | FLAG_5 ) ); \
  if( ! ( (value) & ( 0x01 << (bit) ) ) ) F |= FLAG_P | FLAG_Z; \
  if( (bit) == 7 && (value) & 0x80 ) F |= FLAG_S; \
  Q = F;\
}  

#define CALL()\
{\
  contend_read_no_mreq( PC, 1 ); PC++;\
  PUSH16(PCL,PCH);\
  PC=z80.memptr.w;\
}

#define CP(value)\
{\
  libspectrum_word cptemp = A - value; \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( (  cptemp & 0x88 ) >> 1 );  \
  F = ( cptemp & 0x100 ? FLAG_C : ( cptemp ? 0 : FLAG_Z ) ) | FLAG_N |\
    halfcarry_sub_table[lookup & 0x07] |\
    overflow_sub_table[lookup >> 4] |\
    ( value & ( FLAG_3 | FLAG_5 ) ) |\
    ( cptemp & FLAG_S );\
  Q = F;\
}

/* Macro for the {DD,FD} CB dd xx rotate/shift instructions */
#define DDFDCB_ROTATESHIFT(time, target, instruction)\
tstates+=(time);\
{\
  (target) = readbyte( tempaddr );\
  instruction( (target) );\
  writebyte( tempaddr, (target) );\
}\
break

#define DEC(value)\
{\
  F = ( F & FLAG_C ) | ( (value)&0x0f ? 0 : FLAG_H ) | FLAG_N;\
  (value)--;\
  F |= ( (value)==0x7f ? FLAG_V : 0 ) | sz53_table[value];\
  Q = F;\
}

#define Z80_IN( reg, port )\
{\
  z80.memptr.w=port + 1;\
  (reg)=readport((port));\
  F = ( F & FLAG_C) | sz53p_table[(reg)];\
  Q = F;\
}

#define INC(value)\
{\
  (value)++;\
  F = ( F & FLAG_C ) | ( (value)==0x80 ? FLAG_V : 0 ) |\
  ( (value)&0x0f ? 0 : FLAG_H ) | sz53_table[(value)];\
  Q = F;\
}

#define LD16_NNRR(regl,regh)\
{\
  libspectrum_word ldtemp; \
  ldtemp=readbyte(PC++);\
  ldtemp|=readbyte(PC++) << 8;\
  writebyte(ldtemp++,(regl));\
  z80.memptr.w=ldtemp;\
  writebyte(ldtemp,(regh));\
  break;\
}

#define LD16_RRNN(regl,regh)\
{\
  libspectrum_word ldtemp; \
  ldtemp=readbyte(PC++);\
  ldtemp|=readbyte(PC++) << 8;\
  (regl)=readbyte(ldtemp++);\
  z80.memptr.w=ldtemp;\
  (regh)=readbyte(ldtemp);\
  break;\
}

#define JP()\
{\
  PC=z80.memptr.w;\
}

#define JR()\
{\
  libspectrum_signed_byte jrtemp = readbyte( PC ); \
  contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 ); \
  contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 ); \
  contend_read_no_mreq( PC, 1 ); \
  PC += jrtemp; \
  PC++; \
  z80.memptr.w = PC; \
}

#define OR(value)\
{\
  A |= (value);\
  F = sz53p_table[A];\
  Q = F;\
}

#define POP16(regl,regh)\
{\
  (regl)=readbyte(SP++);\
  (regh)=readbyte(SP++);\
}

#define PUSH16(regl,regh)\
{\
  writebyte( --SP, (regh) );\
  writebyte( --SP, (regl) );\
}

#define RET()\
{\
  POP16(PCL,PCH);\
  z80.memptr.w = PC;\
}

#define RL(value)\
{\
  libspectrum_byte rltemp = (value); \
  (value) = ( (value)<<1 ) | ( F & FLAG_C );\
  F = ( rltemp >> 7 ) | sz53p_table[(value)];\
  Q = F;\
}

#define RLC(value)\
{\
  (value) = ( (value)<<1 ) | ( (value)>>7 );\
  F = ( (value) & FLAG_C ) | sz53p_table[(value)];\
  Q = F;\
}

#define RR(value)\
{\
  libspectrum_byte rrtemp = (value); \
  (value) = ( (value)>>1 ) | ( F << 7 );\
  F = ( rrtemp & FLAG_C ) | sz53p_table[(value)];\
  Q = F;\
}

#define RRC(value)\
{\
  F = (value) & FLAG_C;\
  (value) = ( (value)>>1 ) | ( (value)<<7 );\
  F |= sz53p_table[(value)];\
  Q = F;\
}

#define RST(value)\
{\
  PUSH16(PCL,PCH);\
  PC=(value);\
  z80.memptr.w=PC;\
}

#define SBC(value)\
{\
  libspectrum_word sbctemp = A - (value) - ( F & FLAG_C ); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( ( sbctemp & 0x88 ) >> 1 );  \
  A=sbctemp;\
  F = ( sbctemp & 0x100 ? FLAG_C : 0 ) | FLAG_N |\
    halfcarry_sub_table[lookup & 0x07] | overflow_sub_table[lookup >> 4] |\
    sz53_table[A];\
  Q = F;\
}

#define SBC16(value)\
{\
  libspectrum_dword sub16temp = HL - (value) - (F & FLAG_C); \
  libspectrum_byte lookup = ( (        HL & 0x8800 ) >> 11 ) | \
			    ( (   (value) & 0x8800 ) >> 10 ) | \
			    ( ( sub16temp & 0x8800 ) >>  9 );  \
  z80.memptr.w=HL+1;\
  HL = sub16temp;\
  F = ( sub16temp & 0x10000 ? FLAG_C : 0 ) |\
    FLAG_N | overflow_sub_table[lookup >> 4] |\
    ( H & ( FLAG_3 | FLAG_5 | FLAG_S ) ) |\
    halfcarry_sub_table[lookup&0x07] |\
    ( HL ? 0 : FLAG_Z) ;\
  Q = F;\
}

#define SLA(value)\
{\
  F = (value) >> 7;\
  (value) <<= 1;\
  F |= sz53p_table[(value)];\
  Q = F;\
}

#define SLL(value)\
{\
  F = (value) >> 7;\
  (value) = ( (value) << 1 ) | 0x01;\
  F |= sz53p_table[(value)];\
  Q = F;\
}

#define SRA(value)\
{\
  F = (value) & FLAG_C;\
  (value) = ( (value) & 0x80 ) | ( (value) >> 1 );\
  F |= sz53p_table[(value)];\
  Q = F;\
}

#define SRL(value)\
{\
  F = (value) & FLAG_C;\
  (value) >>= 1;\
  F |= sz53p_table[(value)];\
  Q = F;\
}

#define SUB(value)\
{\
  libspectrum_word subtemp = A - (value); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    (  (subtemp & 0x88 ) >> 1 );  \
  A=subtemp;\
  F = ( subtemp & 0x100 ? FLAG_C : 0 ) | FLAG_N |\
    halfcarry_sub_table[lookup & 0x07] | overflow_sub_table[lookup >> 4] |\
    sz53_table[A];\
  Q = F;\
}

#define XOR(value)\
{\
  A ^= (value);\
  F = sz53p_table[A];\
  Q = F;\
}

#endif		/* #ifndef FUSE_Z80_MACROS_H */

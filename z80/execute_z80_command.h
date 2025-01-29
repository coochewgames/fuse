#ifndef EXECUTE_Z80_COMMAND_H
#define EXECUTE_Z80_COMMAND_H

#include "libspectrum.h"


void _AND(libspectrum_byte value);
void _ADC(libspectrum_byte value);
void _ADC16(libspectrum_word value);
void _ADD(libspectrum_byte value);
void _ADD16(libspectrum_word *value1, libspectrum_word value2);
void _BIT(libspectrum_byte bit, libspectrum_byte value);
void _BIT_MEMPTR(libspectrum_byte bit, libspectrum_byte value);
void _CALL(void);
void _CP(libspectrum_byte value);
void _DDFDCB_ROTATESHIFT(int time, libspectrum_byte *target, void (*instruction)(libspectrum_byte *));
void _DEC(libspectrum_byte *value);
void _Z80_IN(libspectrum_byte *reg, libspectrum_word port);
void _INC(libspectrum_byte *value);
void _LD16_NNRR(libspectrum_byte regl, libspectrum_byte regh);
void _LD16_RRNN(libspectrum_byte *regl, libspectrum_byte *regh);
void _JP(void);
void _JR(void);
void _OR(libspectrum_byte value);
void _POP16(libspectrum_byte *regl, libspectrum_byte *regh);
void _PUSH16(libspectrum_byte regl, libspectrum_byte regh);
void _RET(void);
void _RL(libspectrum_byte *value);
void _RLC(libspectrum_byte *value);
void _RR(libspectrum_byte *value);
void _RRC(libspectrum_byte *value);
void _RST(libspectrum_byte value);
void _SBC(libspectrum_byte value);
void _SBC16(libspectrum_word value);
void _SLA(libspectrum_byte *value);
void _SLL(libspectrum_byte *value);
void _SRA(libspectrum_byte *value);
void _SRL(libspectrum_byte *value);
void _SUB(libspectrum_byte value);
void _XOR(libspectrum_byte value);

#endif // EXECUTE_Z80_COMMAND_H

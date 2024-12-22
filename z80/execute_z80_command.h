#ifndef EXECUTE_Z80_COMMAND_H
#define EXECUTE_Z80_COMMAND_H

#include <libspectrum.h>


void _ADC(libspectrum_byte value);
void _ADD(libspectrum_byte value);
void _ADD16(libspectrum_dword value1, libspectrum_dword value2);

#endif

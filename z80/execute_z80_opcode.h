#ifndef EXECUTE_Z80_OPCODE_H
#define EXECUTE_Z80_OPCODE_H

#include <libspectrum.h>

#include "mnemonics.h"

typedef void (*OP_FUNC_NO_PARAMS)(void);
typedef void (*OP_FUNC_ONE_PARAM)(libspectrum_byte value);
typedef void (*OP_FUNC_TWO_PARAMS)(libspectrum_byte value1, libspectrum_byte value2);
//  Will add extras parameter if needed

typedef enum {
    OP_TYPE_NO_PARAMS,
    OP_TYPE_ONE_PARAM,
    OP_TYPE_TWO_PARAMS
} OP_FUNC_TYPE;

typedef struct {
    Z80_MNEMONIC op;

    OP_FUNC_TYPE function_type;
    union {
        OP_FUNC_NO_PARAMS no_params;
        OP_FUNC_ONE_PARAM one_param;
        OP_FUNC_TWO_PARAMS two_params;
    } func;
} Z80_OP_FUNC_LOOKUP;


Z80_OP_FUNC_LOOKUP z80_op_func_lookup(Z80_MNEMONIC op);

void op_ADC(libspectrum_byte value);
void op_ADD(libspectrum_byte value);

#endif

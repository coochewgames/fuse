#ifndef EXECUTE_Z80_OPCODE_H
#define EXECUTE_Z80_OPCODE_H

#include <libspectrum.h>

#include "mnemonics.h"

typedef void (*OP_FUNC_NO_PARAMS)(void);
typedef void (*OP_FUNC_ONE_PARAM)(const char *value);
typedef void (*OP_FUNC_TWO_PARAMS)(const char *value1, const char *value2);
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


Z80_OP_FUNC_LOOKUP get_z80_op_func(Z80_MNEMONIC op);

// Function prototypes for opcodes with no parameters
void op_NOP(void);
void op_RLCA(void);
void op_RRCA(void);
void op_RLA(void);
void op_RRA(void);
void op_DAA(void);
void op_CPL(void);
void op_SCF(void);
void op_CCF(void);
void op_HALT(void);
void op_RET(void);
void op_DI(void);
void op_EI(void);
void op_EXX(void);
void op_LDI(void);
void op_CPI(void);
void op_INI(void);
void op_OUTI(void);
void op_LDD(void);
void op_CPD(void);
void op_IND(void);
void op_OUTD(void);
void op_LDIR(void);
void op_CPIR(void);
void op_INIR(void);
void op_OTIR(void);
void op_LDDR(void);
void op_CPDR(void);
void op_INDR(void);
void op_OTDR(void);

// Function prototypes for opcodes with one parameter
void op_INC(const char *value);
void op_DEC(const char *value);
void op_SUB(const char *value);
void op_AND(const char *value);
void op_XOR(const char *value);
void op_OR(const char *value);
void op_CP(const char *value);
void op_POP(const char *value);
void op_PUSH(const char *value);
void op_RST(const char *value);
void op_RL(const char *value);
void op_RR(const char *value);
void op_SLA(const char *value);
void op_SRA(const char *value);
void op_SRL(const char *value);
void op_RLC(const char *value);
void op_RRC(const char *value);
void op_DJNZ(const char *value);
void op_JR(const char *value);

// Function prototypes for opcodes with two parameters
void op_LD(const char *value1, const char *value2);
void op_ADD(const char *value1, const char *value2);
void op_ADC(const char *value1, const char *value2);
void op_JP(const char *value1, const char *value2);
void op_CALL(const char *value1, const char *value2);
void op_SBC(const char *value1, const char *value2);
void op_EX(const char *value1, const char *value2);
void op_BIT(const char *value1, const char *value2);
void op_RES(const char *value1, const char *value2);
void op_SET(const char *value1, const char *value2);
void op_IN(const char *value1, const char *value2);
void op_OUT(const char *value1, const char *value2);

#endif // EXECUTE_Z80_OPCODE_H

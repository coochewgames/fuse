#ifndef EXECUTE_Z80_OPCODE_H
#define EXECUTE_Z80_OPCODE_H

void op_ADD(const char *operand_1, const char *operand_2);
void op_ADC(const char *operand_1, const char *operand_2);
void op_AND(const char *operand_1, const char *operand_2);
void op_BIT(const char *operand_1, const char *operand_2);
void op_CALL(const char *operand_1, const char *operand_2);
void op_CCF(void);
void op_CP(const char *operand);
void op_CPD(void);
void op_CPDR(void);
void op_CPI(void);
void op_CPIR(void);
void op_CPL(void);
void op_DAA(void);
void op_DEC(const char *operand);
void op_DI(void);
void op_DJNZ(const char *offset);
void op_EI(void);
void op_EX(const char *operand_1, const char *operand_2);
void op_EXX(void);
void op_HALT(void);
void op_IM(const char *operand);
void op_IN(const char *operand_1, const char *operand_2);
void op_INC(const char *operand);
void op_IND(void);
void op_INDR(void);
void op_INI(void);
void op_INIR(void);
void op_JP(const char *operand_1, const char *operand_2);
void op_JR(const char *operand_1, const char *operand_2);
void op_LD(const char *operand_1, const char *operand_2);




// Function prototypes for opcodes with no parameters
void op_NOP(void);
void op_RLCA(void);
void op_RRCA(void);
void op_RLA(void);
void op_RRA(void);
void op_SCF(void);
void op_RET(void);
void op_NEG(void);
void op_RETN(void);
void op_RRD(void);
void op_RLD(void);
void op_LDI(void);
void op_OUTI(void);
void op_LDD(void);
void op_OUTD(void);
void op_LDIR(void);
void op_OTIR(void);
void op_LDDR(void);
void op_OTDR(void);
void op_SLTTRAP(void);

// Function prototypes for opcodes with one parameter
void op_SUB(const char *operand);
void op_XOR(const char *operand);
void op_OR(const char *operand);
void op_POP(const char *operand);
void op_PUSH(const char *operand);
void op_RST(const char *operand);
void op_RL(const char *operand);
void op_RR(const char *operand);
void op_SLA(const char *operand);
void op_SRA(const char *operand);
void op_SRL(const char *operand);
void op_RLC(const char *operand);
void op_RRC(const char *operand);
void op_SLL(const char *operand);
void op_SHIFT(const char *operand);

// Function prototypes for opcodes with two parameters
void op_SBC(const char *operand_1, const char *operand_2);
void op_RES(const char *operand_1, const char *operand_2);
void op_SET(const char *operand_1, const char *operand_2);
void op_OUT(const char *operand_1, const char *operand_2);

#endif // EXECUTE_Z80_OPCODE_H

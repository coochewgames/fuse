#include <string.h>

#include "mnemonics.h"


const char *getMnemonicName(Z80_MNEMONIC mnemonic) {
    switch (mnemonic) {
        case NOP: return "NOP";       // No operation
        case LD: return "LD";         // Load
        case INC: return "INC";       // Increment
        case DEC: return "DEC";       // Decrement
        case RLCA: return "RLCA";     // Rotate left circular accumulator
        case ADD: return "ADD";       // Add
        case SUB: return "SUB";       // Subtract
        case AND: return "AND";       // Logical AND
        case OR: return "OR";         // Logical OR
        case XOR: return "XOR";       // Logical XOR
        case CP: return "CP";         // Compare
        case JP: return "JP";         // Jump
        case JR: return "JR";         // Jump relative
        case CALL: return "CALL";     // Call subroutine
        case RET: return "RET";       // Return from subroutine
        case PUSH: return "PUSH";     // Push onto stack
        case POP: return "POP";       // Pop from stack
        case RLA: return "RLA";       // Rotate left accumulator
        case RRA: return "RRA";       // Rotate right accumulator
        case DAA: return "DAA";       // Decimal adjust accumulator
        case CPL: return "CPL";       // Complement accumulator
        case SCF: return "SCF";       // Set carry flag
        case CCF: return "CCF";       // Complement carry flag
        case HALT: return "HALT";     // Halt
        case DI: return "DI";         // Disable interrupts
        case EI: return "EI";         // Enable interrupts
        case RL: return "RL";         // Rotate left
        case RR: return "RR";         // Rotate right
        case SLA: return "SLA";       // Shift left arithmetic
        case SRA: return "SRA";       // Shift right arithmetic
        case SRL: return "SRL";       // Shift right logical
        case RLC: return "RLC";       // Rotate left circular
        case RRC: return "RRC";       // Rotate right circular
        case BIT: return "BIT";       // Test bit
        case RES: return "RES";       // Reset bit
        case SET: return "SET";       // Set bit
        default: return "UNKNOWN";    // Unknown mnemonic
    }
}

Z80_MNEMONIC getMnemonicEnum(const char *mnemonic) {
    if (strcmp(mnemonic, "NOP") == 0) return NOP;
    if (strcmp(mnemonic, "LD") == 0) return LD;
    if (strcmp(mnemonic, "INC") == 0) return INC;
    if (strcmp(mnemonic, "DEC") == 0) return DEC;
    if (strcmp(mnemonic, "RLCA") == 0) return RLCA;
    if (strcmp(mnemonic, "ADD") == 0) return ADD;
    if (strcmp(mnemonic, "SUB") == 0) return SUB;
    if (strcmp(mnemonic, "AND") == 0) return AND;
    if (strcmp(mnemonic, "OR") == 0) return OR;
    if (strcmp(mnemonic, "XOR") == 0) return XOR;
    if (strcmp(mnemonic, "CP") == 0) return CP;
    if (strcmp(mnemonic, "JP") == 0) return JP;
    if (strcmp(mnemonic, "JR") == 0) return JR;
    if (strcmp(mnemonic, "CALL") == 0) return CALL;
    if (strcmp(mnemonic, "RET") == 0) return RET;
    if (strcmp(mnemonic, "PUSH") == 0) return PUSH;
    if (strcmp(mnemonic, "POP") == 0) return POP;
    if (strcmp(mnemonic, "RLA") == 0) return RLA;
    if (strcmp(mnemonic, "RRA") == 0) return RRA;
    if (strcmp(mnemonic, "DAA") == 0) return DAA;
    if (strcmp(mnemonic, "CPL") == 0) return CPL;
    if (strcmp(mnemonic, "SCF") == 0) return SCF;
    if (strcmp(mnemonic, "CCF") == 0) return CCF;
    if (strcmp(mnemonic, "HALT") == 0) return HALT;
    if (strcmp(mnemonic, "DI") == 0) return DI;
    if (strcmp(mnemonic, "EI") == 0) return EI;
    if (strcmp(mnemonic, "RL") == 0) return RL;
    if (strcmp(mnemonic, "RR") == 0) return RR;
    if (strcmp(mnemonic, "SLA") == 0) return SLA;
    if (strcmp(mnemonic, "SRA") == 0) return SRA;
    if (strcmp(mnemonic, "SRL") == 0) return SRL;
    if (strcmp(mnemonic, "RLC") == 0) return RLC;
    if (strcmp(mnemonic, "RRC") == 0) return RRC;
    if (strcmp(mnemonic, "BIT") == 0) return BIT;
    if (strcmp(mnemonic, "RES") == 0) return RES;
    if (strcmp(mnemonic, "SET") == 0) return SET;

    return UNKNOWN_MNEMONIC;
}

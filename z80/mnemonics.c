#include <string.h>

#include "mnemonics.h"


const char *get_mnemonic_name(Z80_MNEMONIC mnemonic) {
    switch (mnemonic) {
        case NOP: return "NOP";
        case LD: return "LD";
        case INC: return "INC";
        case DEC: return "DEC";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case NEG: return "NEG";
        case AND: return "AND";
        case OR: return "OR";
        case XOR: return "XOR";
        case CP: return "CP";
        case JP: return "JP";
        case JR: return "JR";
        case CALL: return "CALL";
        case RET: return "RET";
        case RETN: return "RETN";
        case PUSH: return "PUSH";
        case POP: return "POP";
        case DAA: return "DAA";
        case CPL: return "CPL";
        case SCF: return "SCF";
        case CCF: return "CCF";
        case HALT: return "HALT";
        case DI: return "DI";
        case EI: return "EI";
        case RL: return "RL";
        case RR: return "RR";
        case SLA: return "SLA";
        case SRA: return "SRA";
        case SLL: return "SLL";
        case SRL: return "SRL";
        case RLA: return "RLA";
        case RRA: return "RRA";
        case RLC: return "RLC";
        case RRC: return "RRC";
        case RLD: return "RLD";
        case RRD: return "RRD";
        case RLCA: return "RLCA";
        case RRCA: return "RRCA";
        case BIT: return "BIT";
        case RES: return "RES";
        case SET: return "SET";
        case EX: return "EX";
        case DJNZ: return "DJNZ";
        case ADC: return "ADC";
        case SBC: return "SBC";
        case RST: return "RST";
        case OUT: return "OUT";
        case EXX: return "EXX";
        case IN: return "IN";
        case LDI: return "LDI";
        case CPI: return "CPI";
        case INI: return "INI";
        case OUTI: return "OUTI";
        case LDD: return "LDD";
        case CPD: return "CPD";
        case IND: return "IND";
        case OUTD: return "OUTD";
        case LDIR: return "LDIR";
        case CPIR: return "CPIR";
        case INIR: return "INIR";
        case OTIR: return "OTIR";
        case LDDR: return "LDDR";
        case CPDR: return "CPDR";
        case INDR: return "INDR";
        case OTDR: return "OTDR";
        case IM: return "IM";
        default: return "UNKNOWN";    // Unknown mnemonic
    }
}

Z80_MNEMONIC get_mnemonic_enum(const char *mnemonic) {
    if (strcmp(mnemonic, "NOP") == 0) return NOP;
    if (strcmp(mnemonic, "LD") == 0) return LD;
    if (strcmp(mnemonic, "INC") == 0) return INC;
    if (strcmp(mnemonic, "DEC") == 0) return DEC;
    if (strcmp(mnemonic, "RLCA") == 0) return RLCA;
    if (strcmp(mnemonic, "ADD") == 0) return ADD;
    if (strcmp(mnemonic, "SUB") == 0) return SUB;
    if (strcmp(mnemonic, "NEG") == 0) return NEG;
    if (strcmp(mnemonic, "AND") == 0) return AND;
    if (strcmp(mnemonic, "OR") == 0) return OR;
    if (strcmp(mnemonic, "XOR") == 0) return XOR;
    if (strcmp(mnemonic, "CP") == 0) return CP;
    if (strcmp(mnemonic, "JP") == 0) return JP;
    if (strcmp(mnemonic, "JR") == 0) return JR;
    if (strcmp(mnemonic, "CALL") == 0) return CALL;
    if (strcmp(mnemonic, "RET") == 0) return RET;
    if (strcmp(mnemonic, "RETN") == 0) return RETN;
    if (strcmp(mnemonic, "PUSH") == 0) return PUSH;
    if (strcmp(mnemonic, "POP") == 0) return POP;
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
    if (strcmp(mnemonic, "SLL") == 0) return SLL;
    if (strcmp(mnemonic, "SRL") == 0) return SRL;
    if (strcmp(mnemonic, "RLA") == 0) return RLA;
    if (strcmp(mnemonic, "RRA") == 0) return RRA;
    if (strcmp(mnemonic, "RLC") == 0) return RLC;
    if (strcmp(mnemonic, "RRC") == 0) return RRC;
    if (strcmp(mnemonic, "RLD") == 0) return RLD;
    if (strcmp(mnemonic, "RRD") == 0) return RRD;
    if (strcmp(mnemonic, "RRCA") == 0) return RRCA;
    if (strcmp(mnemonic, "BIT") == 0) return BIT;
    if (strcmp(mnemonic, "RES") == 0) return RES;
    if (strcmp(mnemonic, "SET") == 0) return SET;
    if (strcmp(mnemonic, "EX") == 0) return EX;
    if (strcmp(mnemonic, "DJNZ") == 0) return DJNZ;
    if (strcmp(mnemonic, "ADC") == 0) return ADC;
    if (strcmp(mnemonic, "SBC") == 0) return SBC;
    if (strcmp(mnemonic, "RST") == 0) return RST;
    if (strcmp(mnemonic, "OUT") == 0) return OUT;
    if (strcmp(mnemonic, "EXX") == 0) return EXX;
    if (strcmp(mnemonic, "IN") == 0) return IN;
    if (strcmp(mnemonic, "LDI") == 0) return LDI;
    if (strcmp(mnemonic, "CPI") == 0) return CPI;
    if (strcmp(mnemonic, "INI") == 0) return INI;
    if (strcmp(mnemonic, "OUTI") == 0) return OUTI;
    if (strcmp(mnemonic, "LDD") == 0) return LDD;
    if (strcmp(mnemonic, "CPD") == 0) return CPD;
    if (strcmp(mnemonic, "IND") == 0) return IND;
    if (strcmp(mnemonic, "OUTD") == 0) return OUTD;
    if (strcmp(mnemonic, "LDIR") == 0) return LDIR;
    if (strcmp(mnemonic, "CPIR") == 0) return CPIR;
    if (strcmp(mnemonic, "INIR") == 0) return INIR;
    if (strcmp(mnemonic, "OTIR") == 0) return OTIR;
    if (strcmp(mnemonic, "LDDR") == 0) return LDDR;
    if (strcmp(mnemonic, "CPDR") == 0) return CPDR;
    if (strcmp(mnemonic, "INDR") == 0) return INDR;
    if (strcmp(mnemonic, "OTDR") == 0) return OTDR;
    if (strcmp(mnemonic, "IM") == 0) return IM;

    return UNKNOWN_MNEMONIC;
}

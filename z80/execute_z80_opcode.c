#include <string.h>
#include <stdbool.h>

#ifdef CORETEST
#include "coretest.h"
#endif

#include "../spectrum.h"  // Includes tstates
#include "../rzx.h"  // Required for LD instruction
#include "../slt.h" // Required for SLTTRAP instruction
#include "../settings.h" // Required for CMOS setting
#include "../event.h" // Required for event_add
#include "../tape.h" // Required for tape_save_trap
#include "../periph.h"  // Rerquired for readport

#include "../memory_pages.h"
#include "../logging.h"

#include "z80.h"
#include "z80_macros.h"
#include "z80_opcodes.h"

#include "parse_z80_operands.h"
#include "execute_z80_opcode.h"
#include "execute_z80_command.h"


#define LOWER_THREE_BITS_MASK 0x07
#define LOWER_SEVEN_BITS_MASK 0x7f
#define TAPE_SAVING_TRAP 0x04d0
#define TIMEX_2068_SAVE 0x0076


typedef enum {
    CURRENT_OP_BASE = 0,
    CURRENT_OP_FD,
    CURRENT_OP_DD,
    CURRENT_OP_ED,
    CURRENT_OP_CB,
    CURRENT_OP_FDCB,
    CURRENT_OP_DDCB
} CURRENT_OP;

typedef struct {
    const char *condition;
    unsigned char flag;
    bool is_not;
} FLAG_MAPPING;

static FLAG_MAPPING flag_lookup[] = {
    { "C", FLAG_C, false },
    { "NC", FLAG_C, true },
    { "PE", FLAG_P, false },
    { "PO", FLAG_P, true },
    { "M", FLAG_S, false },
    { "P", FLAG_S, true },
    { "Z", FLAG_Z, false },
    { "NZ", FLAG_Z, true },
    { 0 }
};

static CURRENT_OP current_op = CURRENT_OP_BASE;

static void ld_dest_byte(const char *operand_1, const char *operand_2);
static void ld_dest_word(const char *operand_1, const char *operand_2);
static void ld_dest_indirect(const char *operand_1, const char *operand_2);
static void ld_dest_indirect_from_PC(const char *operand);
static void ld_dest_DDFD_offset(const char *operand);

static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static void arithmetic_logical_byte(Z80_MNEMONIC op, const char *operand_2);
static void arithmetic_logical_word(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);

static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static void cpi_cpir_cpd_cpdr(Z80_MNEMONIC op);
static void inc_dec(Z80_MNEMONIC op, const char *operand);
static void ini_inir_ind_indr(Z80_MNEMONIC op);
static void ldi_ldd(Z80_MNEMONIC op);
static void ldir_lddr(Z80_MNEMONIC op);
static void otir_otdr(Z80_MNEMONIC op);
static void outi_outd(Z80_MNEMONIC op);
static void push_pop(Z80_MNEMONIC op, const char *operand);

static void res_set(Z80_MNEMONIC op, const char *operand_1, const char *operand_2);
static unsigned char res_set_hexmask(Z80_MNEMONIC op, unsigned char bit_position);

static void rotate_shift(Z80_MNEMONIC op, const char *operand);
static void call_rotate_shift_offset_op(Z80_MNEMONIC op, libspectrum_word address);
static void call_rotate_shift_op(Z80_MNEMONIC op, libspectrum_byte *reg);

static bool is_byte_reg_from_operand(const char *operand);
static libspectrum_byte *get_byte_reg_from_operand(const char *operand);
static bool is_word_reg_from_operand(const char *operand);
static libspectrum_word *get_word_reg_from_operand(const char *operand);

static bool is_DDFD_op(void);
static libspectrum_byte *get_DDFD_byte_reg_from_operand(const char *operand);
static libspectrum_byte get_DDFD_byte_reg_value_from_operand(const char *operand);
static libspectrum_byte get_DDFD_offset_value(void);
static libspectrum_word *get_DDFD_word_reg(void);
static libspectrum_word get_DDFD_word_reg_value(void);

static bool is_flag_true(const char *condition);
static FLAG_MAPPING get_flag_mapping(const char *condition);

static libspectrum_byte last_Q;


/*
 *  Allow the last_Q to be localised for the opcode execution.
 */
void op_set_last_Q(libspectrum_byte q_value) {
    last_Q = q_value;
}

/*
 *  The following functions execute the Z80 instructions and their associated operands from the dat files,
 *  indexed by their instruction identifier.
 */

void op_ADC(const char *operand_1, const char *operand_2) {
    arithmetic_logical(ADC, operand_1, operand_2);
}

void op_ADD(const char *operand_1, const char *operand_2) {
    arithmetic_logical(ADD, operand_1, operand_2);
}

void op_AND(const char *operand_1, const char *operand_2) {
    arithmetic_logical(AND, operand_1, operand_2);
}

void op_BIT(const char *operand_1, const char *operand_2) {
    if (strlen(operand_1) != 1 || operand_1[0] < '0' || operand_1[0] > '7') {
        ERROR("Expected bit position for operand 1 for BIT: Found this instead %s", operand_1);
        return;
    }

    unsigned char bit_position = operand_1[0] - '0';

    if (strlen(operand_2) == 1) {
        _BIT(bit_position, get_byte_reg_value(operand_2[0]));
    }
    else if (strcmp(operand_2, "(HL)") == 0) {
        libspectrum_byte bytetemp = readbyte(HL);

	    perform_contend_read_no_mreq(HL, 1);
	    _BIT_MEMPTR(bit_position, bytetemp);
    }
    else if (is_DDFD_op()) {
        if (strcmp(operand_2, "(REGISTER+dd)") == 0) {
            libspectrum_byte bytetemp = readbyte(MEMPTR_W);

            perform_contend_read_no_mreq(MEMPTR_W, 1);
            _BIT_MEMPTR(bit_position, bytetemp);
        } else {
            ERROR("Unexpected operand 2 for BIT: %s", operand_2);
        }
    }
    else {
        ERROR("Unexpected operand 2 for BIT: %s", operand_2);
    }
}

void op_CALL(const char *operand_1, const char *operand_2) {
    call_jp(CALL, operand_1, operand_2);
}

void op_CCF(void) {
    F = ( F & (FLAG_P | FLAG_Z | FLAG_S) ) |
        ( (F & FLAG_C) ? FLAG_H : FLAG_C ) |
        ( (settings_current.z80_is_cmos ? A : ((last_Q ^ F) | A)) & (FLAG_3 | FLAG_5) );
    Q = F;
}

void op_CP(const char *operand_1, const char *operand_2) {
    arithmetic_logical(CP, operand_1, operand_2);
}

void op_CPD(void) {
    cpi_cpir_cpd_cpdr(CPD);
}

void op_CPDR(void) {
    cpi_cpir_cpd_cpdr(CPDR);
}

void op_CPI(void) {
    cpi_cpir_cpd_cpdr(CPI);
}

void op_CPIR(void) {
    cpi_cpir_cpd_cpdr(CPIR);
}

void op_CPL(void) {
    A ^= 0xff;
    F = ( F & (FLAG_C | FLAG_P | FLAG_Z | FLAG_S) ) |
        ( A & (FLAG_3 | FLAG_5) ) | (FLAG_N | FLAG_H);
    Q = F;
}

void op_DAA(void) {
	libspectrum_byte add = 0;
    libspectrum_byte carry = (F & FLAG_C);

	if ((F & FLAG_H) || ((A & 0x0f) > 9)) {
        add = 6;
    }

	if (carry || (A > 0x99)) {
        add |= 0x60;
    }

	if (A > 0x99) {
        carry = FLAG_C;
    }

	if (F & FLAG_N) {
        _SUB(add);
	} else {
	    _ADD(add);
	}

	F = ( F & ~(FLAG_C | FLAG_P) ) | carry | parity_table[A];
	Q = F;
}

void op_DEC(const char *operand) {
    inc_dec(DEC, operand);
}

void op_DI(void) {
    IFF1 = 0;
    IFF2 = 0;
}

void op_DJNZ(const char *offset) {
    //  The offset parameter is not used
    perform_contend_read_no_mreq(IR, 1);
    B--;

    if (B) {
        _JR();
    } else {
        perform_contend_read(PC, 3);
        PC++;
    }
}

void op_EI(void) {
    /*
     *  Interrupts are not accepted immediately after an EI, but are
     *  accepted after the next instruction.
     */
    IFF1 = 1;
    IFF2 = 1;

    z80.interrupts_enabled_at = tstates;
    event_add(tstates + 1, z80_interrupt_event);
}

void op_EX(const char *operand_1, const char *operand_2) {
    if (strcmp(operand_1, "AF") == 0 && strcmp(operand_2, "AF'") == 0) {
        /*
         *  Tape saving trap: note this traps the EX AF,AF' at #04d0, not #04d1 as the PC has already been incremented.
         *  0x0076 is the Timex 2068 save routine in EXROM.
         */
        if (PC == (TAPE_SAVING_TRAP + 1) || PC == (TIMEX_2068_SAVE + 1)) {
	        if( tape_save_trap() == 0 ) {
                return;
            }
        }
        
        libspectrum_word wordtemp = AF;

        AF = AF_;
        AF_ = wordtemp;
    } else if (strcmp(operand_1, "(SP)") == 0) {
        libspectrum_byte bytetempl = readbyte(SP);
        libspectrum_byte bytetemph = readbyte(SP + 1);
        libspectrum_byte *reg_h;
        libspectrum_byte *reg_l;

        if (strcmp(operand_2, "HL") == 0) {
            reg_h = &H;
            reg_l = &L;
        } else if (is_DDFD_op() && strcmp(operand_2, "REGISTER") == 0) {
            reg_h = (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) ? &IXH  : &IYH;
            reg_l = (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) ? &IXL : &IYL;
        }
        else {
            ERROR("Unexpected operand 2 for EX with SP as operand 1: %s", operand_2);
            return;
        }

        perform_contend_read_no_mreq(SP + 1, 1);

        writebyte(SP + 1, *reg_h);
        writebyte(SP, *reg_l);

        perform_contend_write_no_mreq(SP, 1);
        perform_contend_write_no_mreq(SP, 1);

        MEMPTR_H = bytetemph;
        MEMPTR_L = bytetempl;

        *reg_h = bytetemph;
        *reg_l = bytetempl;
    } else if (strcmp(operand_1, "DE") == 0 && strcmp(operand_2, "HL") == 0) {
        libspectrum_word wordtemp = DE;

        DE = HL;
        HL = wordtemp;
    } else {
        ERROR("Unexpected operands for EX: %s, %s", operand_1, operand_2);
    }
}

void op_EXX(void) {
	libspectrum_word wordtemp;

	wordtemp = BC;
    BC = BC_;
    BC_ = wordtemp;

	wordtemp = DE;
    DE = DE_;
    DE_ = wordtemp;

	wordtemp = HL;
    HL = HL_;
    HL_ = wordtemp;
}

void op_HALT(void) {
    HALTED = 1;
    PC--;
}

/*
 * The IM instruction is used to set the interrupt mode to 0, 1 or 2 and is from the Extended Instruction Set (ED).
 */
void op_IM(const char *operand) {
    if (strlen(operand) != 1 || operand[0] < '0' || operand[0] > '2') {
        ERROR("Expected 0, 1 or 2 for operand 1 for IM: Found this instead %s", operand);
        return;
    }

    IMODE = operand[0] - '0';
}

void op_IN(const char *operand_1, const char *operand_2) {
    if (strcmp(operand_1, "A") == 0 && strcmp(operand_2, "(nn)") == 0) {
        libspectrum_word intemp = readbyte(PC++) + (A << 8);

        A = readport(intemp);
        MEMPTR_W = intemp + 1;  // Is this correct if (nn) was 0xff?
    }
    else if (strcmp(operand_1, "F") == 0 && strcmp(operand_2, "(C)") == 0) {
        libspectrum_byte bytetemp;

        _Z80_IN(&bytetemp, BC);  // Value is not used but address is for temporary storage
    }
    else if (strlen(operand_1) == 1 && strcmp(operand_2, "(C)") == 0) {
        _Z80_IN(get_byte_reg(operand_1[0]), BC);
    }
    else {
        ERROR("Unexpected operands for IN: %s, %s", operand_1, operand_2);
    }
}

void op_INC(const char *operand) {
    inc_dec(INC, operand);
}

void op_IND(void) {
    ini_inir_ind_indr(IND);
}

void op_INDR(void) {
    ini_inir_ind_indr(INDR);
}

void op_INI(void) {
    ini_inir_ind_indr(INI);
}

void op_INIR(void) {
    ini_inir_ind_indr(INIR);
}

void op_JP(const char *operand_1, const char *operand_2) {
    if (strcmp(operand_1, "HL") == 0) {
        PC = HL;  // Not indirect
    } else if (is_DDFD_op()) {
        PC = (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) ? IX : IY;
    } else {
        call_jp(JP, operand_1, operand_2);
    }
}

/*
 *  The original Perl code checks for no second operand (offset) and if so, it transfer the first operand (condition) to the second (offset) and
 *  blanks out the first (condition).
 * 
 *  This has been updated to just check for "offset" as the first operand.
 */
void op_JR(const char *operand_1, const char *operand_2) {
    if (strcmp(operand_1, "offset") == 0) {
        _JR();
    }
    else {
        if (is_flag_true(operand_1)) {
            _JR();
        } else {
            perform_contend_read(PC, 3);
            PC++;
        }
    }
}

/*
 *  The LD instruction is used to copy data from one location to another; it is the
 *  most frequently used instruction in the Z80 instruction set and has the greatest
 *  variance in operands to be catered for.
 */
void op_LD(const char *operand_1, const char *operand_2) {
    const char *dest = operand_1;
    const char *src = operand_2;

    if (is_byte_reg_from_operand(dest)) {
        //  This call encompasses DD and FD instructions
        ld_dest_byte(dest, src);
    } else if (is_word_reg_from_operand(dest)) {
        ld_dest_word(dest, src);
    } else if (is_indirect_word_reg(dest)) {
        ld_dest_indirect(dest, src);
    } else if (strcmp(dest, "(nnnn)") == 0) {
        ld_dest_indirect_from_PC(src);
    } else if (is_DDFD_op() && strcmp(dest, "(REGISTER+dd)") == 0) {
        ld_dest_DDFD_offset(src);
    } else {
        ERROR("Unexpected operands for LD: %s, %s", dest, src);
    }
}

void op_LDD(void) {
    ldi_ldd(LDD);
}

void op_LDDR(void) {
    ldir_lddr(LDDR);
}

void op_LDI(void) {
    ldi_ldd(LDI);
}

void op_LDIR(void) {
    ldir_lddr(LDIR);
}

void op_NEG(void) {
    libspectrum_byte bytetemp = A;

	A = 0;
	_SUB(bytetemp);
}

void op_NOP(void) {
    //  No operation
}

void op_OR(const char *operand_1, const char *operand_2) {
    arithmetic_logical(OR, operand_1, operand_2);
}

void op_OTDR(void) {
    otir_otdr(OTDR);
}

void op_OTIR(void) {
    otir_otdr(OTIR);
}

void op_OUT(const char *operand_1, const char *operand_2) {
    const char *port = operand_1;
    const char *reg = operand_2;

    if (strcmp(port, "(nn)") == 0 && strcmp(reg, "A") == 0) {
        libspectrum_byte nn = readbyte(PC++);
        libspectrum_word outtemp = nn | (A << 8);

        MEMPTR_H = A;
        MEMPTR_L = (nn + 1);

        writeport(outtemp, A);
    }
    else if (strcmp(port, "(C)") == 0 && strlen(reg) == 1) {
        if (reg[0] == '0' ) {
            writeport(BC, settings_current.z80_is_cmos ? 0xff : 0 );
        } else {
            writeport(BC, get_byte_reg_value(reg[0]));
        }

        MEMPTR_W = BC + 1;
    }
    else {
        ERROR("Unexpected operands for OUT: %s, %s", port, reg);
    }
}

void op_OUTD(void) {
    outi_outd(OUTD);
}

void op_OUTI(void) {
    outi_outd(OUTI);
}

void op_POP(const char *operand) {
    push_pop(POP, operand);
}

void op_PUSH(const char *operand) {
    perform_contend_read_no_mreq(IR, 1);
    push_pop(PUSH, operand);
}

void op_RES(const char *operand_1, const char *operand_2) {
    res_set(RES, operand_1, operand_2);
}

void op_RET(const char *operand) {
    if (operand == NULL || strlen(operand) == 0) {
        _RET();
    } else {
        perform_contend_read_no_mreq(IR, 1);

        if (strcmp(operand, "NZ") == 0) {
            if (PC == 0x056c || PC == 0x0112) {  // There is no indication of what these addresses represent
                if (tape_load_trap() == 0) {
                    return;
                }
            }
        }

        if (is_flag_true(operand)) {
            _RET();
        }
    }
}

void op_RETN(void) {
    IFF1=IFF2;
    _RET();

    z80_retn();
}

void op_RL(const char *operand) {
    rotate_shift(RL, operand);
}

void op_RLA(void) {
	libspectrum_byte bytetemp = A;

	A = (A << 1) | (F & FLAG_C);
	F = (F & (FLAG_P | FLAG_Z | FLAG_S)) |
	    (A & (FLAG_3 | FLAG_5)) | (bytetemp >> 7);
	Q = F;
}

void op_RLC(const char *operand) {
    rotate_shift(RLC, operand);
}

void op_RLCA(void) {
    A = (A << 1) | (A >> 7);
    F = (F & (FLAG_P | FLAG_Z | FLAG_S)) |
        (A & (FLAG_C | FLAG_3 | FLAG_5));
    Q = F;
}

void op_RLD(void) {
	libspectrum_byte bytetemp = readbyte(HL);

    perform_contend_read_no_mreq_iterations(HL, 4);
	writebyte(HL, (bytetemp << 4) | (A & 0x0f));

	A = (A & 0xf0) | (bytetemp >> 4);
	F = (F & FLAG_C) | sz53p_table[A];
	Q = F;

	MEMPTR_W = HL + 1;
}

void op_RR(const char *operand) {
    rotate_shift(RR, operand);
}

void op_RRA(void) {
	libspectrum_byte bytetemp = A;

	A = (A >> 1) | (F << 7);
	F = (F & (FLAG_P | FLAG_Z | FLAG_S)) |
	    (A & (FLAG_3 | FLAG_5)) | (bytetemp & FLAG_C);
	Q = F;
}

void op_RRC(const char *operand) {
    rotate_shift(RRC, operand);
}

void op_RRCA(void) {
    F = (F & (FLAG_P | FLAG_Z | FLAG_S)) |
        (A & FLAG_C);
    A = (A >> 1) | (A << 7);
    F |= (A & (FLAG_3 | FLAG_5));
    Q = F;
}

void op_RRD(void) {
	libspectrum_byte bytetemp = readbyte(HL);

    perform_contend_read_no_mreq_iterations(HL, 4);
	writebyte(HL, (A << 4) | (bytetemp >> 4));

	A = (A & 0xf0) | (bytetemp & 0x0f);
	F = (F & FLAG_C) | sz53p_table[A];
	Q = F;

	MEMPTR_W = HL + 1;
}

void op_RST(const char *operand) {
    libspectrum_byte hex_value = (libspectrum_byte)strtol(operand, NULL, 16);

    perform_contend_read_no_mreq(IR, 1);
    _RST(hex_value);
}

void op_SBC(const char *operand_1, const char *operand_2) {
    arithmetic_logical(SBC, operand_1, operand_2);
}

void op_SCF(void) {
    F = (F & ( FLAG_P | FLAG_Z | FLAG_S)) |
        ((settings_current.z80_is_cmos ? A : ((last_Q ^ F) | A)) & (FLAG_3 | FLAG_5)) |
        FLAG_C;
    Q = F;
}

void op_SET(const char *operand_1, const char *operand_2) {
    res_set(SET, operand_1, operand_2);
}

void op_SLA(const char *operand) {
    rotate_shift(SLA, operand);
}

void op_SLL(const char *operand) {
    rotate_shift(SLL, operand);
}

void op_SRA(const char *operand) {
    rotate_shift(SRA, operand);
}

void op_SRL(const char *operand) {
    rotate_shift(SRL, operand);
}

void op_SUB(const char *operand_1, const char *operand_2) {
    arithmetic_logical(SUB, operand_1, operand_2);
}

void op_XOR(const char *operand_1, const char *operand_2) {
    arithmetic_logical(XOR, operand_1, operand_2);
}

void op_SLTTRAP(void) {
    slt_trap(HL, A);
}

/*
 *  The shift instruction utilises the parameter to determine which op codes set to utilise.
 *  DD instructions use the IX register.
 *  FD instructions use the IY register.
 *  CB instructions represent a group of extended instructions that primarily deal with bit manipulation, shifting and rotating operations.
 *  ED instructions are used for the extended instructions that do not fit into the other categories.
 *
 *  DD or FD can also shift again to use CB instructions.
 */
void op_SHIFT(const char *value) {
    if (strcmp(value, "DDFDCB") == 0) {
        //  This is only called by the DD or FD instruction set to utilise the CB instruction set.
        libspectrum_word register_value = (current_op == CURRENT_OP_DD) ? IX : IY;
        Z80_OP op;

	    perform_contend_read(PC, 3);
	    MEMPTR_W = register_value + (libspectrum_signed_byte)readbyte_internal(PC);
	    PC++;
        perform_contend_read(PC, 3);

	    libspectrum_byte opcode_id = readbyte_internal(PC);
	    perform_contend_read_no_mreq(PC, 1);
        perform_contend_read_no_mreq(PC, 1);
        PC++;

        current_op = (current_op == CURRENT_OP_DD) ? CURRENT_OP_DDCB : CURRENT_OP_FDCB;
        op = z80_ops_set[OP_SET_DDFDCB].op_codes[opcode_id];

        DEBUG("PC:0x%04x, shifted id (%d):0x%02x, op:%s %s,%s", (PC - 1), (int)current_op, opcode_id, get_mnemonic_name(op.op), op.operand_1, op.operand_2);
        call_z80_op_func(op);
    } else {
	    perform_contend_read(PC, 4);

	    libspectrum_byte opcode_id = readbyte_internal(PC);
        Z80_OP op;

        PC++;
	    R++;

        if (strcmp(value, "DD") == 0) {
            current_op = CURRENT_OP_DD;
            op = z80_ops_set[OP_SET_DDFD].op_codes[opcode_id];

            if (op.op == NOP) {
                WARNING("DD instruction found with NOP opcode: id %02x", opcode_id);
            }
        } else if (strcmp(value, "FD") == 0) {
            current_op = CURRENT_OP_FD;
            op = z80_ops_set[OP_SET_DDFD].op_codes[opcode_id];

            if (op.op == NOP) {
                WARNING("FD instruction found with NOP opcode: id %02x", opcode_id);
            }
        } else if (strcmp(value, "CB") == 0) {
            current_op = CURRENT_OP_CB;
            op = z80_ops_set[OP_SET_CB].op_codes[opcode_id];
        } else if (strcmp(value, "ED") == 0) {
            current_op = CURRENT_OP_ED;
            op = z80_ops_set[OP_SET_ED].op_codes[opcode_id];
        } else {
            ERROR("Unexpected value found for op_SHIFT: %s", value);
            return;
        }

        DEBUG("PC:0x%04x, shifted id (%d):0x%02x, op:%s %s,%s", (PC - 1), (int)current_op, opcode_id, get_mnemonic_name(op.op), op.operand_1, op.operand_2);
        call_z80_op_func(op);

        //  The shift is complete, so reset the current_op to the base set.
        current_op = CURRENT_OP_BASE;
    }
}

/*
 *  The following functions are called by Op Codes functions and perform the necessary operations
 *  by calling the commands in `execute_z80_command.c`.
 * 
 *  This can be called for DD and FD instructions to set the IX or IY register high or low bytes;
 *  so the initial name for the register is checked to ensure it is a single character before being
 *  assigned to the src or dest fields.
 */

static void ld_dest_byte(const char *operand_1, const char *operand_2) {
    char dest = 'X';
    libspectrum_byte *dest_reg = get_byte_reg_from_operand(operand_1);
    const char *word_reg;

    if (strlen(operand_1) == 1) {
        dest = operand_1[0];
    }

    if (is_byte_reg_from_operand(operand_2)) {
        char src = 'Y';

        if (strlen(operand_2) == 1) {
            src = operand_2[0];
        }

        if (dest == 'R' || dest == 'I' || src == 'R' || src == 'I') {
            perform_contend_read_no_mreq(IR, 1);
        }

        if (dest == 'R' && src == 'A') {
            //  Keep the RZX instruction counter aligned
            rzx_instructions_offset += (R - A);

            R = A;
            R7 = A;
        } else if (dest == 'A' && src == 'R') {
            A = (R & LOWER_SEVEN_BITS_MASK) | (R7 & BIT_7);
        } else if (dest != src) {
            *dest_reg = *get_byte_reg_from_operand(operand_2);
        }

        if (dest == 'A' && (src == 'I' || src == 'R')) {
            F = (F & FLAG_C) |
                sz53_table[A] |
                (IFF2 ? FLAG_V : 0);
            Q = F;

            z80.iff2_read = 1;
            event_add(tstates, z80_nmos_iff2_event);
        }
    } else if (strcmp(operand_2, "nn") == 0) {
        *dest_reg = readbyte(PC++);
    } else if ((word_reg = get_indirect_word_reg_name(operand_2)) != NULL) {
        if (strcmp(word_reg, "BC") == 0 || strcmp(word_reg, "DE") == 0) {
            MEMPTR_W = get_word_reg_value(word_reg) + 1;
        }
        
        *dest_reg = readbyte(get_word_reg_value(word_reg));
    } else if (strcmp(operand_2, "(nnnn)") == 0) {
        MEMPTR_L = readbyte(PC++);
        MEMPTR_H = readbyte(PC++);
        A = readbyte(MEMPTR_W++);
    } else if (is_DDFD_op()) {
        if (strcmp(operand_2, "(REGISTER+dd)") == 0) {
            if (strlen(operand_1) == 1) {
                *get_byte_reg_from_operand(operand_1) = get_DDFD_byte_reg_value_from_operand(operand_2);
            } else {
                ERROR("Unexpected (REGISTER+dd) operand 1 source byte register found for LD: %s", operand_1);
            }
        } else {
            ERROR("Unexpected DDFD operand 2 source word register found for LD: %s", operand_2);
        }
    } else {
        ERROR("Unexpected operand 2 found for LD: %s", operand_2);
    }
}

static void ld_dest_word(const char *operand_1, const char *operand_2) {
    const char *dest = operand_1;
    const char *src = operand_2;
    regpair dest_word_union;

    dest_word_union.w = *get_word_reg_from_operand(dest);

    if (strcmp(src, "nnnn") == 0) {
        dest_word_union.b.l = readbyte(PC++);
        dest_word_union.b.h = readbyte(PC++);

        //  Write the updated word to the destination register
        *get_word_reg_from_operand(dest) = dest_word_union.w;
    } else if (strcmp(src, "(nnnn)") == 0) {
        _LD16_RRNN(&dest_word_union.b.l, &dest_word_union.b.h);

        //  Write the updated word to the destination register
        *get_word_reg_from_operand(dest) = dest_word_union.w;
    } else if (strcmp(src, "HL") == 0) {
        perform_contend_read_no_mreq(IR, 1);
        perform_contend_read_no_mreq(IR, 1);

        SP = get_word_reg_value(src);
    } else if (is_DDFD_op()) {
        if (strcmp(src, "REGISTER") == 0) {
            perform_contend_read_no_mreq(IR, 1);
            perform_contend_read_no_mreq(IR, 1);

            SP = get_DDFD_word_reg_value();
        } else {
            ERROR("Unexpected DDFD operand 2 source word register found for LD: %s", src);
        }
	} else {
        ERROR("Unexpected operand 2 source word register found for LD: %s", src);
    }
}

/*
 *  The indirect dest is always a word register.
 */
static void ld_dest_indirect(const char *operand_1, const char *operand_2) {
    const char *dest;
    const char *src = operand_2;

    dest = get_indirect_word_reg_name(operand_1);

    if (strlen(dest) != 2) {
        ERROR("Unexpected dest for indirect LD: %s", dest);
        return;
    }

    if (strlen(src) == 1) {
        if (strcmp(dest, "BC") == 0 || strcmp(dest, "DE") == 0) {
            MEMPTR_L = (libspectrum_byte)(get_word_reg_value(dest)) + 1;
            MEMPTR_H = A;
        }

        writebyte(get_word_reg_value(dest), get_byte_reg_value(src[0]));
    } else if (strcmp(src, "nn") == 0) {
        writebyte(get_word_reg_value(dest), readbyte(PC++));
    } else {
        ERROR("Unexpected source for indirect LD: %s", src);
    }
}

static void ld_dest_indirect_from_PC(const char *operand) {
    const char *src = operand;

    if (strcmp(src, "A") == 0) {
        libspectrum_word wordtemp = readbyte(PC++);

        wordtemp |= readbyte(PC++) << 8;
        MEMPTR_L = wordtemp + 1;
        MEMPTR_H = A;
        
        writebyte(wordtemp, A);
    }
    else if (is_word_reg_from_operand(src)) {
        regpair src_word_union;

        src_word_union.w = *get_word_reg_from_operand(src);
        _LD16_NNRR(src_word_union.b.l, src_word_union.b.h);
    }
}

static void ld_dest_DDFD_offset(const char *operand) {
    const char *src = operand;

    if (strlen(src) == 1) {
        (void)get_DDFD_offset_value();
        writebyte(MEMPTR_W, get_byte_reg_value(src[0]));
    } else if (strcmp(src, "nn") == 0) {
        libspectrum_byte offset;
        libspectrum_byte value;

        offset = readbyte(PC++);
        value = readbyte(PC);

        perform_contend_read_no_mreq(PC, 1);
        perform_contend_read_no_mreq(PC, 1);
        PC++;

        MEMPTR_W = get_DDFD_word_reg_value() + (libspectrum_signed_byte)offset;
        writebyte(MEMPTR_W, value);
    } else {
        ERROR("Unexpected src for LD (REGISTER+DD) dest: %s", src);
    }
}

/*
 *  This can be called by ADC, ADD, AND, CP, OR, SBC, SUB, XOR.
 */
static void arithmetic_logical(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    char *op_1 = strdup(operand_1);
    char *op_2 = (operand_2 == NULL) ? NULL : strdup(operand_2);

    /*
     *  In Z80 assembly, if only operand_1 is provided then the code assumes that the
     *  operation uses the accumulator register A.
     */
    if (operand_2 == NULL || strlen(operand_2) == 0) {
        op_2 = strdup(op_1);

        strcpy(op_1, "A");
    }

    /*
     *  All the operations utilise the accumulator register A as the first operand for
     *  single byte instructions.
     */
    if (strlen(op_1) == 1 && op_1[0] != 'A') {
        ERROR("Unexpected single register operand 1 found, expected 'A': %s", op_1);

        free(op_1);
        free(op_2);
        return;
    }

    if (is_byte_reg_from_operand(op_1)) {
        arithmetic_logical_byte(op, op_2);
    } else if (is_word_reg_from_operand(op_1)) {
        arithmetic_logical_word(op, op_1, op_2);
    } else {
        ERROR("Unexpected operand 1 length found for %s: %s", get_mnemonic_name(op), op_1);
    }

    free(op_1);
    free(op_2);
}

static void arithmetic_logical_byte(Z80_MNEMONIC op, const char *operand_2) {
    libspectrum_byte operand_2_value = 0;

    if (is_DDFD_op()) {
        operand_2_value = get_DDFD_byte_reg_value_from_operand(operand_2);
    }
    else if (strlen(operand_2) == 1) {
        operand_2_value = get_byte_reg_value(operand_2[0]);
    }
    else {
        if (strcmp(operand_2, "(HL)") == 0) {
            operand_2_value = readbyte(HL);
        } else {
            //  The "nn" operand entails that the byte value is read from the PC address.
            if (strlen(operand_2) > 0 && strcmp(operand_2, "nn") != 0) {
                WARNING("Unused operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
            }

            operand_2_value = readbyte(PC++);
        }
    }

    switch(op) {
        case ADC:
            _ADC(operand_2_value);
            break;
        case ADD:
            _ADD(operand_2_value);
            break;
        case AND:
            _AND(operand_2_value);
            break;
        case CP:
            _CP(operand_2_value);
            break;
        case OR:
            _OR(operand_2_value);
            break;
        case SBC:
            _SBC(operand_2_value);
            break;
        case SUB:
            _SUB(operand_2_value);
            break;
        case XOR:
            _XOR(operand_2_value);
            break;
        default:
            ERROR("Unexpected operation found with register operand for %s: %s", get_mnemonic_name(op), operand_2_value);
    }
}

/*
 *  For a DDFD word operation, either of the arguments may be REGISTER.
 *  In each case, the first operand is the address of a register and the second operand is a value.
 */
static void arithmetic_logical_word(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    perform_contend_read_no_mreq_iterations(IR, 7);

    if (op == ADD) {
        libspectrum_word *reg_1;
        libspectrum_word reg_2_value = 0;

        if (is_DDFD_op() && strcmp(operand_1, "REGISTER") == 0) {
            reg_1 = get_DDFD_word_reg();
        } else {
            if (strlen(operand_1) == 2) {
                reg_1 = get_word_reg(operand_1);
            } else {
                ERROR("Unexpected operand 1 found for %s: %s", get_mnemonic_name(op), operand_1);
                return;
            }
        }

        if (is_DDFD_op() && strcmp(operand_2, "REGISTER") == 0) {
            reg_2_value = get_DDFD_word_reg_value();
        } else {
            if (strlen(operand_2) == 2) {
                reg_2_value = get_word_reg_value(operand_2);
            } else {
                ERROR("Unexpected operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
                return;
            }
        }

        _ADD16(reg_1, reg_2_value);
    } else {
        if (strcmp(operand_1, "HL") == 0) {
            libspectrum_word operand_2_value = get_word_reg_value(operand_2);

            switch(op) {
                case ADC:
                    _ADC16(operand_2_value);
                    break;
                case SBC:
                    _SBC16(operand_2_value);
                    break;
                default:
                    ERROR("Unexpected operation found with 16-bit register operand for %s: %s", get_mnemonic_name(op), operand_2);
            }
        } else {
            ERROR("Expected operand 1 to be HL for %s: Found %s", get_mnemonic_name(op), operand_1);
        }
    }
}

/*
 *  This can be called by CALL, JP.
 */
static void call_jp(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    const char *condition = operand_1;
    const char *offset = operand_2;

    MEMPTR_L = readbyte(PC++);
    MEMPTR_H = readbyte(PC);

    if (offset == NULL || strlen(offset) == 0 || is_flag_true(condition)) {
        switch(op) {
            case CALL:
                _CALL();
                break;
            case JP:
                _JP();
                break;
            default:
                ERROR("Unexpected operation called call_jp: %s", get_mnemonic_name(op));
        }
    } else {
        PC++;
    }
}

/*
 *  This can be called by CPI, CPD, CPIR, CPDR.
 */
static void cpi_cpir_cpd_cpdr(Z80_MNEMONIC op) {
    libspectrum_byte value = readbyte(HL);
    libspectrum_byte bytetemp = A - value;
    libspectrum_byte lookup;
    int modifier = (op == CPIR) ? 1 : -1;

    lookup = ( (A & FLAG_3) >> 3 ) |
        ( (value & FLAG_3) >> 2 ) |
        ( (bytetemp & FLAG_3) >> 1 );

    for (int i = 0; i < 5; i++) {
        perform_contend_read_no_mreq(HL, 1);
    }

    if (op == CPI || op == CPD) {
        HL += modifier;
    }

	BC--;
	F = ( F & FLAG_C ) |
        ( BC ? (FLAG_V | FLAG_N) : FLAG_N ) |
	    halfcarry_sub_table[lookup] |
        ( bytetemp ? 0 : FLAG_Z ) |
	    ( bytetemp & FLAG_S );

	if (F & FLAG_H) {
        bytetemp--;
    }

	F |= ( bytetemp & FLAG_3 ) | ( (bytetemp & BIT_1) ? FLAG_5 : 0 );
	Q = F;

    if (op == CPI || op == CPD) {
        MEMPTR_W += modifier;
    } else {
        if( ( F & ( FLAG_V | FLAG_Z ) ) == FLAG_V ) {
            for (int i = 0; i < 5; i++) {
                perform_contend_read_no_mreq(HL, 1);
            }

            PC -= 2;
            MEMPTR_W = PC + 1;
        } else {
            MEMPTR_W += modifier;
        }

        HL += modifier;
    }
}

/*
 *  This can be called by INC, DEC.
 */
static void inc_dec(Z80_MNEMONIC op, const char *operand) {
    int modifier = (op == INC) ? 1 : -1;

    if (strlen(operand) == 1) {
        libspectrum_byte *reg = get_byte_reg(operand[0]);

        (op == INC) ? _INC(reg) : _DEC(reg);
    } else if (strlen(operand) == 2) {
        perform_contend_read_no_mreq(IR, 1);
        perform_contend_read_no_mreq(IR, 1);

        (*get_word_reg(operand)) += modifier;
    } else if (strcmp(operand, "(HL)") == 0) {
        libspectrum_byte bytetemp = readbyte(HL);

	    perform_contend_read_no_mreq(HL, 1);

        (op == INC) ? _INC(&bytetemp) : _DEC(&bytetemp);
	    writebyte(HL, bytetemp);
    } else if (is_DDFD_op()) {
        if (strcmp(operand, "REGISTER") == 0) {
            libspectrum_word *reg = (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) ? &IX : &IY;

            perform_contend_read_no_mreq( IR, 1 );
	        perform_contend_read_no_mreq( IR, 1 );

            (*reg) += modifier;
        } else if (strcmp(operand, "(REGISTER+dd)") == 0) {
            libspectrum_byte value = get_DDFD_offset_value();

            perform_contend_read_no_mreq(MEMPTR_W, 1);
            (op == INC) ? _INC(&value) : _DEC(&value);

        	writebyte(MEMPTR_W, value);
        } else {
            libspectrum_byte *reg = get_DDFD_byte_reg_from_operand(operand);

            (op == INC) ? _INC(reg) : _DEC(reg);
        }
    } else {
        ERROR("Unexpected operand found for %s: %s", get_mnemonic_name(op), operand);
    }
}

/*
 * This function can be called by INI, IND, INIR, INDR.
 */
static void ini_inir_ind_indr(Z80_MNEMONIC op) {
    int modifier = (op == INI || op == INIR) ? 1 : -1;

	libspectrum_byte initemp;
    libspectrum_byte initemp2;

	perform_contend_read_no_mreq(IR, 1);
	initemp = readport(BC);
	writebyte(HL, initemp);

    MEMPTR_W = BC + modifier;
	B--;

    if (op == INI || op == IND) {
        HL += modifier;
    }

    initemp2 = initemp + C + modifier;

	F = ( (initemp & 0x80) ? FLAG_N : 0 ) |
        ( (initemp2 < initemp) ? (FLAG_H | FLAG_C) : 0 ) |
        ( (parity_table[(initemp2 & 0x07) ^ B]) ? FLAG_P : 0 ) |
        sz53_table[B];
	Q = F;

    if (op == INIR || op == INDR) {
        if (B) {
            for (int i = 0; i < 5; i++) {
                perform_contend_read_no_mreq(HL, 1);
            }

            PC -= 2;
        }

        HL += modifier;
    }
}

/*
 * This function can be called by LDI, LDD.
 */
static void ldi_ldd(Z80_MNEMONIC op) {
    int modifier = (op == LDI) ? 1 : -1;
    libspectrum_byte bytetemp = readbyte(HL);

	BC--;
	writebyte(DE, bytetemp);
	perform_contend_write_no_mreq(DE, 1);
    perform_contend_write_no_mreq(DE, 1);

	DE += modifier;
    HL += modifier;
	bytetemp += A;

	F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) |
        ( BC ? FLAG_V : 0 ) |
	    ( (bytetemp & FLAG_3 ) | ( (bytetemp & BIT_1)) ? FLAG_5 : 0 );
	Q = F;
}

/*
 * This function can be called by LDIR, LDDR.
 */
static void ldir_lddr(Z80_MNEMONIC op) {
    int modifier = (op == LDIR) ? 1 : -1;
	libspectrum_byte bytetemp;
    
    bytetemp = readbyte(HL);
	writebyte(DE, bytetemp);

	perform_contend_write_no_mreq(DE, 1);
    perform_contend_write_no_mreq(DE, 1);

	BC--;
	bytetemp += A;

	F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) |
        ( BC ? FLAG_V : 0 ) |
	    ( bytetemp & FLAG_3 ) |
        ( (bytetemp & BIT_1) ? FLAG_5 : 0 );
	Q = F;

    DEBUG("LDIR/LDDR: BC:%d, HL:0x%04x, DE:0x%04x, F:0x%02x", BC, HL, DE, F);

	if (BC) {
        for (int i = 0; i < 5; i++) {
            perform_contend_write_no_mreq(DE, 1);
        }

        PC -= 2;
	    MEMPTR_W = PC + 1;
	}

    HL += modifier;
    DE += modifier;
}

/*
 * This function can be called by OTIR, OTDR.
 */
static void otir_otdr(Z80_MNEMONIC op) {
    int modifier = (op == OTIR) ? 1 : -1;
	libspectrum_byte outitemp;
    libspectrum_byte outitemp2;

	perform_contend_read_no_mreq(IR, 1);
	outitemp = readbyte(HL);
	B--;    // This does happen first, despite what the specs say
	MEMPTR_W = BC + modifier;
	writeport(BC, outitemp);

	HL += modifier;
    outitemp2 = outitemp + L;

	F = ( (outitemp & 0x80) ? FLAG_N : 0 ) |
        ( (outitemp2 < outitemp) ? (FLAG_H | FLAG_C) : 0 ) |
        ( (parity_table[(outitemp2 & LOWER_THREE_BITS_MASK) ^ B]) ? FLAG_P : 0 ) |
        sz53_table[B];
	Q = F;

	if (B) {
        for (int i = 0; i < 5; i++) {
            perform_contend_read_no_mreq(BC, 1);
        }

        PC -= 2;
    }
}

/*
 * This function can be called by OUTI, OUTD.
 */
static void outi_outd(Z80_MNEMONIC op) {
    int modifier = (op == OUTI) ? 1 : -1;
	libspectrum_byte outitemp;
    libspectrum_byte outitemp2;

	perform_contend_read_no_mreq(IR, 1);
	outitemp = readbyte(HL);
	B--;    // This does happen first, despite what the specs say
	MEMPTR_W = BC + modifier;
	writeport(BC, outitemp);

	HL += modifier;
    outitemp2 = outitemp + L;

	F = ( (outitemp & 0x80) ? FLAG_N : 0 ) |
        ( (outitemp2 < outitemp) ? (FLAG_H | FLAG_C) : 0 ) |
        ( (parity_table[(outitemp2 & LOWER_THREE_BITS_MASK) ^ B]) ? FLAG_P : 0 ) |
        sz53_table[B];
	Q = F;
}

/*
 * This function can be called by PUSH, POP.
 */
static void push_pop(Z80_MNEMONIC op, const char *operand) {
    if (is_DDFD_op()) {
        if (strcmp(operand, "REGISTER") == 0) {
            if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
                (op == PUSH) ? _PUSH16(IXL, IXH) : _POP16(&IXL, &IXH);
            } else {
                (op == PUSH) ? _PUSH16(IYL, IYH) : _POP16(&IYL, &IYH);
            }
        } else {
            ERROR("Unexpected DDFD register found: %s", operand);
        }
    } else if (strlen(operand) == 2) {
        libspectrum_byte *reg_high = get_byte_reg(operand[0]);
        libspectrum_byte *reg_low = get_byte_reg(operand[1]);

        (op == PUSH) ? _PUSH16(*reg_low, *reg_high) : _POP16(reg_low, reg_high);
    } else {
        ERROR("Unexpected operand found for %s: %s", get_mnemonic_name(op), operand);
    }
}

/*
 * This function can be called by RES, SET.
 *
 * This instruction is called from the CB instruction set.
 * The first operand is a bit position from 0 to 7.
 */
static void res_set(Z80_MNEMONIC op, const char *operand_1, const char *operand_2) {
    if (strlen(operand_1) != 1 || operand_1[0] < '0' || operand_1[0] > '7') {
        ERROR("Expected bit position for operand 1 for %s: Found this instead %s", get_mnemonic_name(op), operand_1);
        return;
    }

    unsigned char bit_position = operand_1[0] - '0';
    unsigned char bit_mask = res_set_hexmask(op, bit_position);

    if (strlen(operand_2) == 1) {
        libspectrum_byte *reg = get_byte_reg(operand_2[0]);

        if (op == RES) {
            *reg &= bit_mask;
        } else {
            *reg |= bit_mask;
        }
    }
    else if (strcmp(operand_2, "(HL)") == 0) {
        libspectrum_byte bytetemp = readbyte(HL);

	    perform_contend_read_no_mreq(HL, 1);

        if (op == RES) {
            writebyte(HL, bytetemp & bit_mask);
        } else {
            writebyte(HL, bytetemp | bit_mask);
        }
    }
    else if (is_DDFD_op()) {
        if (strcmp(operand_2, "(REGISTER+dd)") == 0) {
            libspectrum_byte bytetemp = readbyte(MEMPTR_W);

	        perform_contend_read_no_mreq(MEMPTR_W, 1);

            if (op == RES) {
    	        writebyte(MEMPTR_W, bytetemp & bit_mask);
            } else {
    	        writebyte(MEMPTR_W, bytetemp | bit_mask);
            }
        }
        else {
            ERROR("Unexpected DDFD operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
        }
    }
    else {
        ERROR("Unexpected operand 2 found for %s: %s", get_mnemonic_name(op), operand_2);
    }
}

/*
 *  This function returns the hex mask for the given bit position.
 */
static unsigned char res_set_hexmask(Z80_MNEMONIC op, unsigned char bit_position) {
    unsigned char mask = 1 << bit_position;

    if (op == RES) {
        mask = ~mask;
    }

    return mask;
}

/*
 *  This function can be called by RL, RR, SLA, SRA, SRL, RLC, RRC.
 */
static void rotate_shift(Z80_MNEMONIC op, const char *operand) {
    if (strlen(operand) == 1) {
        libspectrum_byte *reg = get_byte_reg(operand[0]);

        call_rotate_shift_op(op, reg);
    } else if (strcmp(operand, "(HL)") == 0) {
        call_rotate_shift_offset_op(op, HL);
    }
    else if (is_DDFD_op()) {
        if (strcmp(operand, "(REGISTER+dd)") == 0) {
            call_rotate_shift_offset_op(op, MEMPTR_W);
        }
        else {
            ERROR("Unexpected DDFD operand found for %s: %s", get_mnemonic_name(op), operand);
        }
    }
    else {
        ERROR("Unexpected operand found for %s: %s", get_mnemonic_name(op), operand);
    }
}

/*
 *  Utilise a value at a given address to call the appropriate rotate/shift operation.
 */
static void call_rotate_shift_offset_op(Z80_MNEMONIC op, libspectrum_word address) {
    libspectrum_byte bytetemp = readbyte(address);

    perform_contend_read_no_mreq(address, 1);
    call_rotate_shift_op(op, &bytetemp);

    writebyte(address, bytetemp);
}

/*
 *  Call the appropriate rotate/shift operation based on the given operation in execute_z80_command.c.
 */
static void call_rotate_shift_op(Z80_MNEMONIC op, libspectrum_byte *reg) {
    switch(op) {
        case RL:
            _RL(reg);
            break;
        case RR:
            _RR(reg);
            break;
        case SLA:
            _SLA(reg);
            break;
        case SRA:
            _SRA(reg);
            break;
        case SLL:
            _SLL(reg);
            break;
        case SRL:
            _SRL(reg);
            break;
        case RLC:
            _RLC(reg);
            break;
        case RRC:
            _RRC(reg);
            break;
        default:
            ERROR("Unexpected operation found for rotate_shift: %s", get_mnemonic_name(op));
    }
}

static bool is_byte_reg_from_operand(const char *operand) {
    bool is_byte_reg = false;

    if (strlen(operand) == 1) {
        is_byte_reg = true;
    } else if (is_DDFD_op()) {
        if (strcmp(operand, "REGISTERL") == 0 || strcmp(operand, "REGISTERH") == 0) {
            is_byte_reg = true;
        }
    }

    return is_byte_reg;
}

static libspectrum_byte *get_byte_reg_from_operand(const char *operand) {
    libspectrum_byte *single_byte_reg = NULL;

    if (strlen(operand) == 1) {
        single_byte_reg = get_byte_reg(operand[0]);
    } else if (is_DDFD_op()) {
        single_byte_reg = get_DDFD_byte_reg_from_operand(operand);
    }

    return single_byte_reg;
}

/*
 *  This will also return true for indirect word registers.
 */
static bool is_word_reg_from_operand(const char *operand) {
    bool is_word_reg = false;

    if (strlen(operand) == 2) {
        is_word_reg = true;
    } else if (is_DDFD_op()) {
        if (strcmp(operand, "REGISTER") == 0) {
            is_word_reg = true;
        }
    }

    return is_word_reg;
}

/*
 *  This will also the value for an indirect word registers.
 */
static libspectrum_word *get_word_reg_from_operand(const char *operand) {
    libspectrum_word *word_reg = NULL;

    if (strlen(operand) == 2) {
        word_reg = get_word_reg(operand);
    } else if (is_DDFD_op()) {
        word_reg = get_DDFD_word_reg();
    }

    return word_reg;
}

static bool is_DDFD_op(void) {
    return (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB ||
            current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB);
}

static libspectrum_byte *get_DDFD_byte_reg_from_operand(const char *operand) {
    libspectrum_byte *reg = NULL;

    if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
        if (strcmp(operand, "REGISTERL") == 0) {
            reg = &IXL;
        } else if (strcmp(operand, "REGISTERH") == 0) {
            reg = &IXH;
        } else {
            ERROR("Unexpected DD byte register operand found: %s", operand);
        }
    } else if (current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB) {
        if (strcmp(operand, "REGISTERL") == 0) {
            reg = &IYL;
        } else if (strcmp(operand, "REGISTERH") == 0) {
            reg = &IYH;
        } else {
            ERROR("Unexpected FD byte register operand found: %s", operand);
        }
    } else {
        ERROR("Unexpected current_op found: %d", current_op);
    }

    return reg;
}

static libspectrum_byte get_DDFD_byte_reg_value_from_operand(const char *operand) {
    libspectrum_byte value = 0;

    if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
        if (strcmp(operand, "REGISTERL") == 0) {
            value = IXL;
        } else if (strcmp(operand, "REGISTERH") == 0) {
            value = IXH;
        } else if (strcmp(operand, "(REGISTER+dd)") == 0) {
            value = get_DDFD_offset_value();
        } else {
            ERROR("Unexpected DD operand found: %s", operand);
        }
    } else if (current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB) {
        if (strcmp(operand, "REGISTERL") == 0) {
            value = IYL;
        } else if (strcmp(operand, "REGISTERH") == 0) {
            value = IYH;
        } else if (strcmp(operand, "(REGISTER+dd)") == 0) {
            value = get_DDFD_offset_value();
        } else {
            ERROR("Unexpected FD operand found: %s", operand);
        }
    } else {
        ERROR("Unexpected current_op found: %d", current_op);
    }

    return value;
}

static libspectrum_byte get_DDFD_offset_value(void) {
    libspectrum_byte offset = readbyte(PC);

    perform_contend_read_no_mreq_iterations(PC, 5);

    PC++;
	MEMPTR_W = get_DDFD_word_reg_value() + (libspectrum_signed_byte)offset;
	return readbyte(MEMPTR_W);
}

static libspectrum_word *get_DDFD_word_reg(void) {
    libspectrum_word *value = NULL;

    if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
        value = &IX;
    } else if (current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB) {
        value = &IY;
    } else {
        ERROR("Unexpected current_op found: %d", current_op);
    }

    return value;
}

static libspectrum_word get_DDFD_word_reg_value(void) {
    libspectrum_word value = 0;

    if (current_op == CURRENT_OP_DD || current_op == CURRENT_OP_DDCB) {
        value = IX;
    } else if (current_op == CURRENT_OP_FD || current_op == CURRENT_OP_FDCB) {
        value = IY;
    } else {
        ERROR("Unexpected current_op found: %d", current_op);
    }

    return value;
}

static bool is_flag_true(const char *condition) {
    FLAG_MAPPING flag_mapping = get_flag_mapping(condition);

    if ((flag_mapping.is_not && !(F & flag_mapping.flag)) ||
        (!flag_mapping.is_not && (F & flag_mapping.flag))) {
        return true;
    }

    return false;
}

static FLAG_MAPPING get_flag_mapping(const char *condition) {
    FLAG_MAPPING found_flag_mapping = { 0 };

    for (int i = 0; flag_lookup[i].condition != NULL; i++) {
        if (strcmp(flag_lookup[i].condition, condition) == 0) {
            found_flag_mapping = flag_lookup[i];
        }
    }
    
    return found_flag_mapping;
}

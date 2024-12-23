#ifndef MNEMONICS_H
#define MNEMONICS_H

#define UNKNOWN_MNEMONIC  -1

typedef enum {
    NOP,    // No operation
    LD,     // Load data between registers or between memory and registers
    INC,    // Increment register or memory location by 1
    DEC,    // Decrement register or memory location by 1
    RLCA,   // Rotate left circular accumulator
    ADD,    // Add register or memory location to accumulator
    SUB,    // Subtract register or memory location from accumulator
    AND,    // Logical AND between register or memory location and accumulator
    OR,     // Logical OR between register or memory location and accumulator
    XOR,    // Logical XOR between register or memory location and accumulator
    CP,     // Compare register or memory location with accumulator
    JP,     // Jump to address
    JR,     // Jump relative to address
    CALL,   // Call subroutine
    RET,    // Return from subroutine
    PUSH,   // Push register pair onto stack
    POP,    // Pop register pair from stack
    RLA,    // Rotate left accumulator
    RRA,    // Rotate right accumulator
    DAA,    // Decimal adjust accumulator
    CPL,    // Complement accumulator (logical NOT)
    SCF,    // Set carry flag
    CCF,    // Complement carry flag
    HALT,   // Halt CPU until interrupt occurs
    DI,     // Disable interrupts
    EI,     // Enable interrupts
    RL,     // Rotate left through carry
    RR,     // Rotate right through carry
    SLA,    // Shift left arithmetic
    SRA,    // Shift right arithmetic
    SRL,    // Shift right logical
    RLC,    // Rotate left circular
    RRC,    // Rotate right circular
    BIT,    // Test bit in register or memory location
    RES,    // Reset bit in register or memory location
    SET,    // Set bit in register or memory location
    EX,     // Exchange registers
    RRCA,   // Rotate right circular accumulator
    DJNZ,   // Decrement B and jump if not zero
    ADC,    // Add with carry to accumulator
    SBC,    // Subtract with carry from accumulator
    RST,    // Restart (call to fixed address)
    OUT,    // Output to port
    EXX,    // Exchange alternate register set
    IN,     // Input from port
    LDI,    // Load and increment
    CPI,    // Compare and increment
    INI,    // Input and increment
    OUTI,   // Output and increment
    LDD,    // Load and decrement
    CPD,    // Compare and decrement
    IND,    // Input and decrement
    OUTD,   // Output and decrement
    LDIR,   // Load, increment, and repeat
    CPIR,   // Compare, increment, and repeat
    INIR,   // Input, increment, and repeat
    OTIR,   // Output, increment, and repeat
    LDDR,   // Load, decrement, and repeat
    CPDR,   // Compare, decrement, and repeat
    INDR,   // Input, decrement, and repeat
    OTDR,   // Output, decrement, and repeat
    Z80_MNEMONIC_COUNT // This should be the last entry
} Z80_MNEMONIC;

const char *get_mnemonic_name(Z80_MNEMONIC mnemonic);
Z80_MNEMONIC get_mnemonic_enum(const char *mnemonic);

#endif

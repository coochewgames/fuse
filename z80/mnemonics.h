#ifndef MNEMONICS_H
#define MNEMONICS_H

#define UNKNOWN_MNEMONIC  -1

typedef enum {
    NOP,    // No operation
    LD,     // Load
    INC,    // Increment
    DEC,    // Decrement
    RLCA,   // Rotate left circular accumulator
    ADD,    // Add
    SUB,    // Subtract
    AND,    // Logical AND
    OR,     // Logical OR
    XOR,    // Logical XOR
    CP,     // Compare
    JP,     // Jump
    JR,     // Jump relative
    CALL,   // Call subroutine
    RET,    // Return from subroutine
    PUSH,   // Push onto stack
    POP,    // Pop from stack
    RLA,    // Rotate left accumulator
    RRA,    // Rotate right accumulator
    DAA,    // Decimal adjust accumulator
    CPL,    // Complement accumulator
    SCF,    // Set carry flag
    CCF,    // Complement carry flag
    HALT,   // Halt
    DI,     // Disable interrupts
    EI,     // Enable interrupts
    RL,     // Rotate left
    RR,     // Rotate right
    SLA,    // Shift left arithmetic
    SRA,    // Shift right arithmetic
    SRL,    // Shift right logical
    RLC,    // Rotate left circular
    RRC,    // Rotate right circular
    BIT,    // Test bit
    RES,    // Reset bit
    SET     // Set bit
} Z80_MNEMONIC;


const char *getMnemonicName(Z80_MNEMONIC mnemonic);
Z80_MNEMONIC getMnemonicEnum(const char *mnemonic);

#endif

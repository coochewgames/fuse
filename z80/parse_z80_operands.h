#ifndef PARSE_Z80_OPCODE_H
#define PARSE_Z80_OPCODE_H

#include <stdbool.h>
#include <libspectrum.h>


typedef struct {
    bool is_indirect;
    char reg[3];                // To store register pair (e.g., "HL", "IX", "IY")
    libspectrum_word address;   // To store a literal address value
    int offset;                 // To store the offset value
} ADDRESS_OPERAND;


libspectrum_byte get_DD_value(void);
libspectrum_byte get_byte_value(char *operand);
libspectrum_byte get_reg_byte_value(char reg);
libspectrum_word get_reg_word_value(const char *reg);
libspectrum_word get_indirect_reg_word_value(const char *reg);

ADDRESS_OPERAND parse_address_operand(const char *operand);
void perform_contend_read_no_mreq_iterations(libspectrum_word address, int iterations);

#endif

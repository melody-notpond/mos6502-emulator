//
// MOS6502 Emulator
// addressing.h: Header file for addressing.c.
//
// Created by jenra.
// Created on June 15 2020.
//

#ifndef ADDRESSING_H
#define ADDRESSING_H

#include "m6502.h"

// The jump table for all implemented addressing modes except absolute indirect and zero page + y offset.
extern const addr_fn addressing_modes[3][8];

bool m65_addr_impl(m6502_t* cpu);
bool m65_addr_zp_x(m6502_t* cpu);
bool m65_addr_zp_y(m6502_t* cpu);
bool m65_addr_abs_x(m6502_t* cpu);
bool m65_addr_abs_y(m6502_t* cpu);

#endif /* ADDRESSING_H */

//
// MOS6502 Emulator
// instructions.h: Header file for instructions.c.
// 
// Created by jenra.
// Created on June 15 2020.
// 

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "m6502.h"

// The jump table for all implemented (regular) instructions.
extern const instr_fn instructions[3][8];

// The jump table for instructions that end with an 8.
extern const instr_fn instr_08_f8[16];

// The jump table for instructions that end with a and are in the upper half of the byte.
extern const instr_fn instr_8a_fa[8];

// The jump table for opcodes 00, 20, 40, and 60.
extern const instr_fn instr_00_60[4];

// Instructions to be exported
bool m65_instr_brk(m6502_t* cpu);
bool m65_instr_bra(m6502_t* cpu);
bool m65_instr_jpa(m6502_t* cpu);
bool m65_instr_jpi(m6502_t* cpu);
bool m65_instr_ldx(m6502_t* cpu);
bool m65_instr_stx(m6502_t* cpu);

#endif /* INSTRUCTIONS_H */

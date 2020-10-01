//
// MOS6502 Emulator
// instructions.c: Implements all instructions.
//
// Created by jenra.
// Created on June 15 2020.
//

#include <stdio.h>


#include <stdlib.h>

#include "instructions.h"

// Adds or subtracts the accumulator with carry.
// Implemented opcodes:
// - 69 - adc #immediate
// - 65 - adc $zero page
// - 75 - adc $zero page, x
// - 6D - adc $absolute
// - 7D - adc $absolute, x
// - 79 - adc $absolute, y
// - 61 - adc ($zero page, x)
// - 71 - adc ($zero page), y
//
// - E9 - sbc #immediate
// - E5 - sbc $zero page
// - F5 - sbc $zero page, x
// - ED - sbc $absolute
// - FD - sbc $absolute, x
// - F9 - sbc $absolute, y
// - E1 - sbc ($zero page, x)
// - F1 - sbc ($zero page), y
#define m65_instr___c(mem, inv)																						\
bool m65_instr_##mem (m6502_t* cpu)																					\
{																													\
	switch (cpu->ipc)																								\
	{																												\
		case 0:																										\
			/* cycle 1 - get value */																				\
			return false;																							\
		case 1:																										\
			/* cycle 2 - add or subtract accumulator and fetch */													\
			if (cpu->flags & 0x08)																					\
			{																										\
				/* Decimal mode */																					\
				puts("decimal mode uhoh");																			\
			} else																									\
			{																										\
				/* Binary mode */																					\
				cpu->alu.b = inv cpu->pins.data;																	\
				cpu->alu.c = cpu->a + cpu->alu.b + (cpu->flags & 1);												\
			}																										\
																													\
			/* Update flags */																						\
			cpu->flags = (cpu->flags & 0x3C)																		\
					   | (cpu->alu.c & 0x80)																		\
					   | ((cpu->a & 0x80) == (cpu->alu.b & 0x80) && (cpu->a & 0x80) != (cpu->alu.c & 0x80)) << 6	\
					   | (cpu->alu.c == 0) << 1																		\
					   | (cpu->a > cpu->alu.c || cpu->alu.b > cpu->alu.c);											\
			cpu->a = cpu->alu.c;																					\
		default:																									\
			return true;																							\
	}																												\
}

m65_instr___c(adc,  )
m65_instr___c(sbc, ~)

#undef m65_instr___c

// Branches to an address relative to the program counter based on a condition.
// Length: 2 bytes / 1 word
// Time: 2 cycles + 1 if branching + 1 if page boundary crossed
// Implemented opcodes:
// - 10 - bpl rel
// - 30 - bmi rel
// - 50 - bvc rel
// - 70 - bvs rel
// - 90 - bcc rel
// - B0 - bcs rel
// - D0 - bne rel
// - F0 - beq rel
bool m65_instr_bra(m6502_t* cpu)
{
	//							  N		  V		  C		  Z
	static uint8_t flags[4] = {1 << 7, 1 << 6, 1 << 0, 1 << 1};

	switch (cpu->ipc)
	{
		// cycle 1 - read the program counter offset and test the flag; dont resume instruction if test failed
		case 0:
			// Read offset
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc++;

			// Test the flag
			uint8_t flag = flags[(cpu->ir & 0xC0) >> 6];
			if ((bool)(cpu->flags & flag) != (bool) (cpu->ir & 0x20))
				cpu->ipc = 2;
			return false;
		
		// cycle 2 - add the program counter offset to the low byte
		case 1:
			cpu->addr_buf = cpu->pc;
			cpu->pc += (int8_t) cpu->pins.data;
			return false;

		// cycle 2.5 - increment or decrement high byte if needed
		// (This doesn't actually do anything but the 6502 needs it)
		case 2:
			if ((cpu->addr_buf & 0x0100) != (cpu->pc & 0x0100))
			{
				return false;
			} else cpu->ipc++;

		// cycle 3 - fetch next instruction
		case 3:
		default:
			return true;
	}
}

// Triggers a nonmaskable software interrupt.
// Length: 2 bytes / 1 word
// Time: 7 cycles
// Implemented opcode:
// - 00 - brk
bool m65_instr_brk(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - increment program counter
		case 0:
			if (cpu->int_brk)
				cpu->pc++;
			return false;

		// cycle 2 - push high byte of program counter
		case 1:
			cpu->pins.rw = cpu->int_rw;
			cpu->pins.addr = 0x0100 | cpu->s--;
			cpu->pins.data = (cpu->pc & 0xff00) >> 8;
			return false;

		// cycle 3 - push low byte of program counter
		case 2:
			cpu->pins.rw = cpu->int_rw;
			cpu->pins.addr = 0x0100 | cpu->s--;
			cpu->pins.data = cpu->pc & 0xff;
			return false;

		// cycle 4 - push processor flags
		case 3:
			cpu->pins.rw = cpu->int_rw;
			cpu->pins.addr = 0x0100 | cpu->s--;
			cpu->pins.data = cpu->flags;

			// Adjust break flag in stack
			if (cpu->int_brk)
				cpu->pins.data |= 0x10;
			else cpu->pins.data &= 0xEF;
			return false;

		// cycle 5 - read low byte of program counter
		case 4:
			cpu->pins.addr = cpu->int_vec;

			// Adjust interrupt flag post push
			if (cpu->int_dsi)
				cpu->flags |= 0x04;
			else cpu->flags &= 0xFD;
			return false;

		// cycle 6 - read high byte of program counter
		case 5:
			cpu->addr_buf = cpu->pins.data;
			cpu->pins.addr++;
			return false;

		// cycle 7 - reset interrupt config and fetch
		case 6:
			cpu->int_rw = WRITE;
			cpu->int_brk = true;
			cpu->int_dsi = false;
			cpu->int_vec = 0xFFFE;
			cpu->pc = cpu->pins.data << 8 | cpu->addr_buf;

		default:
			return true;
	}
}

// Increments or decrements a value in memory.
// Implemented opcodes:
// - E6 - inc $zero page
// - F6 - inc $zero page, x
// - EE - inc $absolute
// - FE - inc $absolute, x
// - C6 - dec $zero page
// - D6 - dec $zero page, x
// - CE - dec $absolute
// - DE - dec $absolute, x
#define m65_instr_idm(mem, op)																	\
bool m65_instr_##mem(m6502_t* cpu)																\
{																								\
	switch (cpu->ipc)																			\
	{																							\
		case 0:																					\
			/* cycle 1 - get value */															\
			return false;																		\
		case 1:																					\
			/* cycle 2 - increment or decrement the value */									\
			cpu->alu.c = cpu->pins.data op 1;													\
			cpu->flags = (cpu->flags & 0x7D) | (cpu->alu.c & 0x80) | (cpu->alu.c == 0) << 1;	\
			return false;																		\
		case 2:																					\
			/* cycle 3 - store to memory */														\
			cpu->pins.rw = WRITE;																\
			cpu->pins.data = cpu->alu.c;														\
			return false;																		\
		case 3:																					\
			/* cycle 4 - fetch */																\
		default:																				\
			return true;																		\
	}																								\
}

m65_instr_idm(inc, +)
m65_instr_idm(dec, -)

#undef m65_instr_idm

// Increments or decrements a register.
// Implemented opcodes:
// - E8 - inx
// - CA - dex
// - C8 - iny
// - 88 - dey
#define m65_instr_idr(mnem, reg, op)															\
bool m65_instr_##mnem (m6502_t* cpu)															\
{																								\
	switch (cpu->ipc)																			\
	{																							\
		/* cycle 1 - do nothing */																\
		case 0:																					\
			return false;																		\
																								\
		/* cycle 2 - increment or decrement the register */										\
		case 1:																					\
			cpu->reg op;																		\
			cpu->flags = (cpu->flags & 0x7D) | (cpu->reg & 0x80) << 7 | (cpu->reg == 0) << 1;	\
		default:																				\
			return true;																		\
	}																							\
}

m65_instr_idr(inx, x, ++)
m65_instr_idr(dex, x, --)
m65_instr_idr(iny, y, ++)
m65_instr_idr(dey, y, --)

#undef m65_instr_idr

// Jumps to an address.
// Length: 3 bytes / 1.5 words
// Time: 3 cycles
// Implemented opcode:
// - 4C - jmp $absolute
bool m65_instr_jpa(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		case 0:
			goto ipc_0;
		case 1:
			goto ipc_1;
		case 2:
			goto ipc_2;
		default:
			goto ipc_ret;
	}

	// cycle 1 - read low byte
ipc_0:
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc++;
	return false;

	// cycle 2 - read high byte
ipc_1:
	cpu->addr_buf = cpu->pins.data;
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc;
	return false;

	// cycle 3 - set program counter (ie, jump)
ipc_2:
	cpu->pc = cpu->pins.data << 8 | cpu->addr_buf;

ipc_ret:
	return true;
}

// Jumps to an address.
// Length: 3 bytes / 1.5 words
// Time: 5 cycles
// Implemented opcode:
// - 6C - jmp ($absolute)
bool m65_instr_jpi(m6502_t* cpu)
{
	switch(cpu->ipc)
	{
		case 0:
			goto ipc_0;
		case 1:
			goto ipc_1;
		case 2:
			goto ipc_2;
		case 3:
			goto ipc_3;
		case 4:
			goto ipc_4;
		default:
			goto ipc_ret;
	}

	// cycle 1 - read low byte of address
ipc_0:
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc++;
	return false;

	// cycle 2 - read high byte of address
ipc_1:
	cpu->addr_buf = cpu->pins.data;
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc++;
	return false;

	// cycle 3 - read low byte of location
ipc_2:
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pins.data << 8 | cpu->addr_buf;
	return false;

	// cycle 4 - read high byte of location
ipc_3:
	cpu->addr_buf = cpu->pins.data;
	cpu->pins.rw = READ;

	// This instruction doesn't update the high byte when crossing a page boundary.
	if ((cpu->pins.addr & 0xff) == 0xff)
		 cpu->pins.addr &= 0xff00;
	else cpu->pins.addr++;
	return false;

	// cycle 5 - set program counter (ie, jump to location)
ipc_4:
	cpu->pc = cpu->pins.data << 8 | cpu->addr_buf;

ipc_ret:
	return true;
}

// Calls a subroutine
// Length: 3 bytes / 1.5 words
// Time: 6 cycles
// Implemented opcode:
// - 20 - jsr $absolute
bool m65_instr_jsr(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		case 0:
			goto ipc_0;
		case 1:
			goto ipc_1;
		case 2:
			goto ipc_2;
		case 3:
			goto ipc_3;
		case 4:
			goto ipc_4;
		case 5:
			goto ipc_5;
		default:
			goto ipc_ret;
	}

	// cycle 1 - read low byte of address
ipc_0:
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc++;
	return false;

	// cycle 2 - read high byte of address
ipc_1:
	cpu->addr_buf = cpu->pins.data;
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc;
	return false;

	// cycle 3 - push high byte of the program counter to the stack
ipc_2:
	cpu->addr_buf |= cpu->pins.data << 8;
	cpu->pins.rw = WRITE;
	cpu->pins.addr = 0x0100 | cpu->s;
	cpu->pins.data = (cpu->pc & 0xff00) >> 8;
	return false;

	// decrement stack pointer
ipc_3:
	cpu->s--;
	return false;

	// push low byte of the program counter to stack
ipc_4:
	cpu->pins.rw = WRITE;
	cpu->pins.addr = 0x0100 | cpu->s;
	cpu->pins.data = cpu->pc & 0xff;
	return false;

	// decrement stack pointer and fetch
ipc_5:
	cpu->s--;
	cpu->pc = cpu->addr_buf;
ipc_ret:
	return true;
}

// Loads a value into a register.
// Implemented opcodes are:
// - A9 - lda #immediate
// - A5 - lda $zero page
// - B5 - lda $zero page, X
// - AD - lda $absolute
// - BD - lda $absolute, X
// - B9 - lda $absolute, Y
// - A1 - lda ($zero page, X)
// - B1 - lda ($zero page), Y
//
// - A2 - ldx #immediate
// - A6 - ldx $zero page
// - B6 - ldx $zero page, Y
// - AE - ldx $absolute
// - BE - ldx $absolute, Y
//
// - A0 - ldy #immediate
// - A4 - ldy $zero page
// - B4 - ldy $zero page, X
// - AC - ldy $absolute
// - BC - ldy $absolute, X
#define m65_instr_ld_(reg)																\
bool m65_instr_ld##reg (m6502_t* cpu)													\
{																						\
	switch (cpu->ipc)																	\
	{																					\
		case 0:																			\
			goto ipc_0;																	\
		case 1:																			\
			goto ipc_1;																	\
		default:																		\
			goto ipc_ret;																\
	}																					\
																						\
	/* cycle 1 - read value	*/ 															\
ipc_0:																					\
	cpu->pins.rw = READ;																\
	return false;																		\
																						\
	/* cycle 2 - store in register, update flags, and fetch next instruction */			\
ipc_1:																					\
	cpu->reg = cpu->pins.data;															\
	cpu->flags = (cpu->flags & 0x7D) | (cpu->reg & 0x80) << 7 | (cpu->reg == 0) << 1;	\
																						\
ipc_ret:																				\
	return true;																		\
}

m65_instr_ld_(a)
m65_instr_ld_(x)
m65_instr_ld_(y)

#undef m65_instr_ld_

// Does nothing, skipping the next byte in memory.
// Length: 2 bytes / 1 word
// Time: 2 cycles
// Implemented opcode:
// - EA - nop
bool m65_instr_nop(m6502_t* cpu)
{
	if (cpu->ipc == 0)
	{
		cpu->pc++;
		return false;
	}

	return true;
}

// Pushes a register onto the stack.
// Length: 1 byte
// Time: 3 cycles
// Implemented opcodes:
// - 48 - pha
// - 08 - php
#define m65_instr_ph_(mnem, reg)							\
bool m65_instr_ph##mnem (m6502_t* cpu)						\
{															\
	switch (cpu->ipc)										\
	{														\
		case 0:												\
			goto ipc_0;										\
		case 1:												\
			goto ipc_1;										\
		default:											\
			goto ipc_ret;									\
	}														\
															\
	/* cycle 1 - put accumulator in $0100+stack pointer */	\
ipc_0:														\
	cpu->pins.rw = WRITE;									\
	cpu->pins.addr = 0x0100 | cpu->s;						\
	cpu->pins.data = cpu->reg;								\
	return false;											\
															\
	/* cycle 2 - decrement stack pointer and fetch */		\
ipc_1:														\
	cpu->s--;												\
ipc_ret:													\
	return true;											\
}

m65_instr_ph_(a, a)
m65_instr_ph_(p, flags)

#undef m65_instr_ph_

// Pops (pulls?) a register from the stack.
// Length: 1 byte
// Time: 4 cycles
// Implemented opcodes:
// - 68 - pla
// - 28 - plp
#define m65_instr_pl_(mnem, reg)							\
bool m65_instr_pl##mnem (m6502_t* cpu)						\
{															\
	switch (cpu->ipc)										\
	{														\
		/* cycle 1 - increment stack pointer */				\
		case 0:												\
			cpu->s++;										\
			return false;									\
															\
		/* cycle 2 - load value on the top of the stack */	\
		case 1:												\
			cpu->pins.rw = READ;							\
			cpu->pins.addr = 0x0100 | cpu->s;				\
			return false;									\
															\
		/* cycle 3 - store tos in register and fetch */		\
		case 2:												\
			cpu->reg = cpu->pins.data;						\
		default:											\
			return true;									\
	}														\
}

m65_instr_pl_(a, a)
m65_instr_pl_(p, flags)

#undef m65_instr_pl_

// Returns from an interrupt.
// Length: 1 byte
// Time: 6 cycles
// Implemented opcode:
// - 40 - rti
bool m65_instr_rti(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - load processor flags
		case 0:
			cpu->pins.rw = READ;
			cpu->pins.addr = 0x0100 | ++cpu->s;
			return false;

		// cycle 2 - increment stack pointer
		case 1:
			cpu->flags = cpu->pins.data;
			cpu->s++;
			return false;

		// cycle 3 - load low byte of pc
		case 2:
			cpu->pins.rw = READ;
			cpu->pins.addr = 0x0100 | cpu->s;
			return false;

		// cycle 4 - increment stack pointer
		case 3:
			cpu->addr_buf = cpu->pins.data;
			cpu->s++;
			return false;

		// cycle 5 - load high byte of pc
		case 4:
			cpu->pins.rw = READ;
			cpu->pins.addr = 0x0100 | cpu->s;
			return false;

		// cycle 6 - fetch
		case 5:
			cpu->pc = cpu->pins.data << 8 | cpu->addr_buf;
		default:
			return true;
	}
}

// Returns from a subroutine.
// Length: 1 byte
// Time: 6 cycles
// Implemented opcode:
// - 60 - rts
bool m65_instr_rts(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - do nothing (wasted cycle for implicit addressing)
		case 0:
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc;
			return false;

		// cycle 2 - increment stack pointer
		case 1:
			cpu->s++;
			return false;

		// cycle 3 - load low byte of pc
		case 2:
			cpu->pins.rw = READ;
			cpu->pins.addr = 0x0100 | cpu->s;
			return false;

		// cycle 4 - increment stack pointer
		case 3:
			cpu->addr_buf = cpu->pins.data;
			cpu->s++;
			return false;

		// cycle 5 - load high byte of pc
		case 4:
			cpu->pins.rw = READ;
			cpu->pins.addr = 0x0100 | cpu->s;
			return false;

		// cycle 6 - fetch
		case 5:
			cpu->pc = (cpu->pins.data << 8 | cpu->addr_buf) + 1;
		default:
			return true;
	}
}

// Stores the accumulator in memory.
// Note: The extra page boundary cycles for sta is always executed in the real 6502, but I'm too lazy to implement it here xD.
// Implemented opcodes are:
// - 85 - sta $zero page
// - 95 - sta $zero page, X
// - 8D - sta $absolute
// - 9D - sta $absolute, X
// - 99 - sta $absolute, Y
// - 81 - sta ($zero page, X)
// - 91 - sta ($zero page), Y
//
// - 86 - stx $zero page
// - 96 - stx $zero page, Y
// - 8E - stx $absolute
//
// - 84 - sty $zero page
// - 94 - sty $zero page, X
// - 8C - sty $absolute
#define m65_instr_st_(reg)							\
bool m65_instr_st##reg (m6502_t* cpu)				\
{													\
	switch (cpu->ipc)								\
	{												\
		/* cycle 1 - write register to memory */	\
		case 0:										\
			cpu->pins.rw = WRITE;					\
			cpu->pins.data = cpu->reg;				\
			return false;							\
													\
		/* cycle 2 - fetch */						\
		case 1:										\
		default:									\
			return true;							\
													\
	}												\
}

m65_instr_st_(a)
m65_instr_st_(x)
m65_instr_st_(y)

#undef m65_instr_st_

// Sets or clears a processor state flag.
// Implemented opcodes are:
// - 18 - clc 0001
// - 38 - sec 0011
// - 58 - cli 0101
// - 78 - sei 0111
// - b8 - clv 1011
// - d8 - cld 1101
// - f8 - sed 1111
bool m65_instr_scf(m6502_t* cpu)
{
	//							  C		  I		  V		  D
	static uint8_t flags[4] = {1 << 0, 1 << 2, 1 << 6, 1 << 3};

	// cycle 1 - set or clear the flag and fetch
	if (cpu->ipc == 0)
	{
		// Get the flag
		uint8_t flag = flags[(cpu->ir & 0xC0) >> 6];

		// Set or clear the flag (for overflow, it's always clear)
		if (flag != flags[2] && (cpu->ir & 0x20))
			cpu->flags |= flag;
		else
			cpu->flags &= ~flag;
	}

	return true;
}

// Transfers one register into another.
// Implemented opcodes are:
// - AA - tax
// - 8a - txa
// - A8 - tay
// - 98 - tya
// - 9A - txs
// - BA - tsx
#define m65_instr_t__(rf, rt)																\
bool m65_instr_t##rf##rt (m6502_t* cpu)														\
{																							\
	switch (cpu->ipc)																		\
	{																						\
		/* cycle 1 - read the next byte */													\
		case 0:																				\
			return false;																	\
																							\
		/* cycle 2 - move rf to rt */														\
		case 1:																				\
			cpu->rt = cpu->rf;																\
			cpu->flags = (cpu->flags & 0x7D) | (cpu->rt & 0x80) << 7 | (cpu->rt == 0) << 1;	\
		default:																			\
			return true;																	\
	}																						\
}

m65_instr_t__(a, x);
m65_instr_t__(x, a);
m65_instr_t__(a, y);
m65_instr_t__(y, a);
m65_instr_t__(s, x);
m65_instr_t__(x, s);

#undef m65_instr_t__

// The jump table for all implemented regular instructions.
const instr_fn instructions[3][8] = {
	{
		NULL		 , NULL			, m65_instr_jpa, m65_instr_jpi,
		m65_instr_sty, m65_instr_ldy, NULL		   , NULL
	},
	{
		NULL		 , NULL			, NULL		   , m65_instr_adc,
		m65_instr_sta, m65_instr_lda, NULL		   , m65_instr_sbc
	},
	{
		NULL		 , NULL			, NULL		   , NULL,
		m65_instr_stx, m65_instr_ldx, m65_instr_dec, m65_instr_inc
	}
};

// The jump table for instructions that end with 8.
const instr_fn instr_08_f8[16] = {
	m65_instr_php, m65_instr_scf, m65_instr_plp, m65_instr_scf,
	m65_instr_pha, m65_instr_scf, m65_instr_pla, m65_instr_scf,
	m65_instr_dey, m65_instr_tya, m65_instr_tay, m65_instr_scf,
	m65_instr_iny, m65_instr_scf, m65_instr_inx, m65_instr_scf
};

// The jump table for instructions in the upper half of the byte that end with a.
const instr_fn instr_8a_fa[8] = {
	m65_instr_txa, m65_instr_txs, m65_instr_tax, m65_instr_tsx,
	m65_instr_dex, NULL			, m65_instr_nop, NULL
};

// The jump table for opcodes 00, 20, 40, and 60.
const instr_fn instr_00_60[4] = {
	m65_instr_brk, m65_instr_jsr, m65_instr_rti, m65_instr_rts
};

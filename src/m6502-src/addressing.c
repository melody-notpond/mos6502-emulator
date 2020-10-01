//
// MOS6502 Emulator
// addressing.c: Implements all addressing modes.
// 
// Created by jenra.
// Created on June 15 2020.
// 

#include <stdlib.h>

#include "addressing.h"

//
// cycles:
// first few cycles: addressing mode
// next cycle: do the operation and fetch next instruction
// or instead:
//  - do operation and store
//  - fetch next instruction
//

// Implicit addressing - does not use memory but still reads from it.
// Length: 1 byte
// Time: 2 cycles
bool m65_addr_impl(m6502_t* cpu)
{
	if (cpu->ipc == 0)
	{
		// cycle 1 - do nothing
		cpu->pins.rw = READ;
		cpu->pins.addr = cpu->pc;
		return false;

		// cycle 2 - operation and fetch
	} else return true;
}

bool m65_addr_acc(m6502_t* cpu)
{
	// cycle 1 - do nothing
	cpu->pins.rw = READ;

	// cycle 2 - operation and fetch
	return true;
}

// Immediate addressing - loads a value right after the opcode.
// Length: 2 bytes / 1 word
// Time: 2 cycles
bool m65_addr_imm(m6502_t* cpu)
{
	// cycle 1 - load immediate
	if (cpu->ipc == 0)
	{
		cpu->pins.rw = READ;
		cpu->pins.addr = cpu->pc++;
	}

	// cycle 2 - operation and fetch
	return true;
}

// Zero Page addressing - loads a value located at an address in the zero page.
// Length: 2 bytes / 1 word
// Time: 3 cycles
bool m65_addr_zp(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - load address
		case 0:
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc++;
			return false;

		// cycle 2 - addressing (rw set by operation)
		case 1:
			cpu->pins.addr = cpu->pins.data;

		// cycle 3 - operation and fetch
		default:
			return true;
	}
}

// Absolute addressing - loads a value located at any address.
// Length: 3 bytes / 1.5 words
// Time: 4 cycles
bool m65_addr_abs(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - load low byte of address
		case 0:
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc++;
			return false;

		// cycle 2 - load high byte of address
		case 1:
			cpu->addr_buf = cpu->pins.data;
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc++;
			return false;

		// cycle 3 - addressing (rw set by operation)
		case 2:
			cpu->pins.addr = cpu->pins.data << 8 | cpu->addr_buf;

		// cycle 4 - operation and fetch
		default:
			return true;
	}
}

// Zero Page, register offset addressing: Adds the value of an index register to the supplied zero page address and loads the value from the calculated address.
// Length: 2 bytes / 1 word
// Time: 4 cycles
#define m65_addr_zp_(register)											\
bool m65_addr_zp_##register (m6502_t* cpu)								\
{																		\
	switch (cpu->ipc)													\
	{																	\
		/* cycle 1 - load address */									\
		case 0:															\
			cpu->pins.rw = READ;										\
			cpu->pins.addr = cpu->pc++;									\
			return false;												\
																		\
		/* cycle 2 - zp+register */										\
		case 1:															\
			cpu->addr_buf = (uint8_t) cpu->pins.data + cpu->register;	\
			return false;												\
																		\
		/* cycle 3 - addressing (rw set by operation) */				\
		case 2:															\
			cpu->pins.addr = cpu->addr_buf;								\
																		\
		/* cycle 4 - operation and fetch */								\
		default:														\
			return true;												\
	}															\
}

m65_addr_zp_(x);
m65_addr_zp_(y);

#undef m65_addr_zp_

// Absolute, register offset addressing: Adds the value of an index register to the supplied address and loads the value from the calculated address.
// Length: 3 bytes / 1.5 words
// Time: 4 cycles + 1 if page boundary crossed
#define m65_addr_abs_(register)										\
bool m65_addr_abs_##register (m6502_t* cpu)							\
{																	\
	switch (cpu->ipc)												\
	{																\
		/* cycle 1 - load low byte of address */					\
		case 0:														\
			cpu->pins.rw = READ;									\
			cpu->pins.addr = cpu->pc++;								\
			return false;											\
																	\
		/* cycle 2 - lb+reg, load high byte of address */			\
		case 1:														\
			cpu->addr_buf = cpu->pins.data + cpu->register;			\
			cpu->pins.rw = READ;									\
			cpu->pins.addr = cpu->pc++;								\
			return false;											\
																	\
		/* cycle 2.5 - increment high byte if needed */				\
		case 2:														\
			if (cpu->addr_buf & 0x100)								\
			{														\
				cpu->addr_buf &= 0xff;								\
				cpu->pins.data++;									\
				return false;										\
			} cpu->ipc++;											\
																	\
		/* cycle 3 - addressing (rw set by operation) */			\
		case 3:														\
			cpu->pins.addr = cpu->pins.data << 8 | cpu->addr_buf;	\
																	\
		/* cycle 4 - operation and fetch */							\
		default:													\
			return true;											\
	}																\
}

m65_addr_abs_(x);
m65_addr_abs_(y);

#undef m65_addr_abs_

// Indirect X addressing: adds the x register to the zero page address, loads an address from the calculated zero page address, and loads a value from the fetched address.
// Length: 2 bytes / 1 word
// Time: 6 cycles
bool m65_addr_ind_zp_x(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - load address
		case 0:
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc++;
			return false;

		// cycle 2 - zp+x
		case 1:
			cpu->addr_buf = cpu->pins.data + cpu->x;
			return false;

		// cycle 3 - load low byte at address
		case 2:
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->addr_buf;
			return false;

		// cycle 4 - load high byte at address
		case 3:
			cpu->addr_buf = cpu->pins.data;
			cpu->pins.rw = READ;
			cpu->pins.addr++;
			return false;

		// cycle 5 - addressing (rw set by operation)
		case 4:
			cpu->pins.addr = cpu->pins.data << 8 | cpu->addr_buf;

		// cycle 6 - operation and fetch
		default:
			return true;
	}
}

// Indirect Y addressing - loads an address from zero page, adds the y register to it, and loads a value from the calculated address.
// Length: 2 bytes / 1 word
// Time 5 cycles + 1 if page boundary crossed
bool m65_addr_ind_zp_y(m6502_t* cpu)
{
	switch (cpu->ipc)
	{
		// cycle 1 - load address
		case 0:
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->pc++;
			return false;

		// cycle 2 - load low byte at address
		case 1:
			cpu->addr_buf = cpu->pins.data;
			cpu->pins.rw = READ;
			cpu->pins.addr = cpu->addr_buf;
			return false;

		// cycle 3 - lb+y, load high byte at address + 1
		case 2:
			cpu->addr_buf = cpu->pins.data + cpu->y;
			cpu->pins.rw = READ;
			cpu->pins.addr++;
			return false;

		// cycle 3.5 - increment high byte if necessary
		case 3:
			if (cpu->addr_buf & 0x100)
			{
				cpu->addr_buf &= 0xff;
				cpu->pins.data++;
				return false;
			} else cpu->ipc++;

		// cycle 4 - addressing (rw set by operation)
		case 4:
			cpu->pins.addr = cpu->pins.data << 8 | cpu->addr_buf;

		// cycle 5 - operation and fetch
		default:
			return true;
	}
}

// The jump table for all implemented addressing modes except zero page y offset.
const addr_fn addressing_modes[3][8] = {
	{m65_addr_imm, m65_addr_zp, NULL, m65_addr_abs, NULL, m65_addr_zp_x, NULL, m65_addr_abs_x},
	{m65_addr_ind_zp_x, m65_addr_zp, m65_addr_imm, m65_addr_abs, m65_addr_ind_zp_y, m65_addr_zp_x, m65_addr_abs_y, m65_addr_abs_x},
	{m65_addr_imm, m65_addr_zp, m65_addr_acc, m65_addr_abs, NULL, m65_addr_zp_x, NULL, m65_addr_abs_x}
};

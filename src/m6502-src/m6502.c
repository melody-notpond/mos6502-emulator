//
// MOS6502 Emulator
// m6502.c: Implements the 6502 processor.
//
// Created by jenra.
// Created on June 8 2020.
//

#include <stdio.h>


#include <stdlib.h>

#include "addressing.h"
#include "instructions.h"
#include "m6502.h"

// init_6502(m6502_t*) -> void
// Initialises a 6502 processor.
void init_6502(m6502_t* cpu)
{
	cpu->a = 0;
	cpu->x = 0;
	cpu->y = 0;
	cpu->s = 0x02;
	//			   NV-BDIZC
	cpu->flags = 0b00110110;
	cpu->pc = 0;

	cpu->alu.a = 0;
	cpu->alu.b = 0;
	cpu->alu.c = 0;

	cpu->instr = NULL;
	cpu->addr_mode = NULL;

	cpu->ir = 0;
	cpu->ipc = 0;

	cpu->handle_interrupt = false;
	cpu->int_rw = WRITE;
	cpu->int_brk = true;
	cpu->int_dsi = false;
	cpu->int_vec = 0xFFFE;

	cpu->addr_buf = 0;

	cpu->pins.addr = 0;
	cpu->pins.data = 0;
	cpu->pins.rw = READ;
}

// m65_fetch(m6502_t*) -> void
// Fetches the next opcode. This is done before executing any code and on the last cycle of an instruction.
void m65_fetch(m6502_t* cpu)
{
	cpu->pins.rw = READ;
	cpu->pins.addr = cpu->pc++;
}

// m65_decode(m6502_t*) -> void
// Decodes the next opcode.
void m65_decode(m6502_t* cpu)
{
	// Transfer data from the input pins into the ir
	cpu->ir = cpu->pins.data;

	// Branches
	if ((cpu->ir & 0x1f) == 0b00010000)
	{
		cpu->instr = m65_instr_bra;
		cpu->addr_mode = NULL;

	// Stack related jumps
	} else if ((cpu->ir & 0x9f) == 0x00)
	{
		cpu->instr = instr_00_60[cpu->ir >> 5];
		cpu->addr_mode = NULL;

	// Stuff that ends with 8
	} else if ((cpu->ir & 0x0f) == 0x08)
	{
		cpu->instr = instr_08_f8[(cpu->ir & 0xf0) >> 4];
		cpu->addr_mode = m65_addr_impl;

	// Stuff that ends with a in the upper half of the byte
	} else if ((cpu->ir & 0x8f) == 0x8a)
	{
		instr_fn instr = instr_8a_fa[((cpu->ir & 0xf0) >> 4) - 0x8];

		if (instr != NULL)
		{
			cpu->instr = instr;
			cpu->addr_mode = m65_addr_impl;
		}

	// Literally everything else
	} else
	{
		// Get the instruction and addressing mode
		uint8_t sel = cpu->ir & 0b11;
		addr_fn addr_mode = addressing_modes[sel][cpu->ir >> 2 & 0b111];
		instr_fn instr = instructions[sel][cpu->ir >> 5];

		// Check that the instruction isn't null (unimplemented or illegal opcode)
		if (instr != NULL)
		{
			// Set the instruction function
			cpu->instr = instr;

			// Clear addressing mode function for jmp
			if (instr == m65_instr_jpa || instr == m65_instr_jpi)
				cpu->addr_mode = NULL;

			// Adjust addressing modes that use the X register for ldx and stx
			else if (addr_mode == m65_addr_zp_x && (instr == m65_instr_ldx || instr == m65_instr_stx))
				 cpu->addr_mode = m65_addr_zp_y;
			else if (addr_mode == m65_addr_abs_x && instr == m65_instr_ldx)
				 cpu->addr_mode = m65_addr_abs_y;
			else cpu->addr_mode = addr_mode;
		}
	}

	// Fetch next instruction if not found
	// if (cpu->instr == NULL)
	// 	m65_fetch(cpu);
}

// m65_cycle(m6502_t*) -> void
// Executes one cycle of a 6502 processor.
void m65_cycle(m6502_t* cpu)
{
	// Disable the bus
	cpu->pins.rw = READ;

	// Decode the opcode if not done already
	if (cpu->instr == NULL)
	{
		// deal with interrupts
		if (cpu->handle_interrupt)
		{
			puts("interrupt request handler activated");
			cpu->handle_interrupt = false;
			cpu->ir = 0;
			cpu->instr = m65_instr_brk;
			cpu->addr_mode = NULL;
			cpu->pc--;
		} else m65_decode(cpu);
	}

	// Execute the addressing mode code
	if (cpu->addr_mode != NULL)
	{
		if (cpu->addr_mode(cpu))
		{
			// Once done, clear the addressing mode and IPC
			cpu->addr_mode = NULL;
			cpu->ipc = 0;
			puts("end of addressing");

		// Otherwise increment the IPC
		} else cpu->ipc++;
	}

	// Execute the instruction specific code
	if (cpu->addr_mode == NULL && cpu->instr != NULL)
	{
		if (cpu->instr(cpu))
		{
			// Once done, clear the instruction function and fetch the next opcode
			puts("fetching next opcode");
			cpu->instr = NULL;
			cpu->ipc = 0;
			m65_fetch(cpu);
			puts("end of instruction");

		// Otherwise increment the IPC
		} else cpu->ipc++;
	}
}

// m65_nmi(m6502_t*) -> void
// Triggers a nonmaskable interrupt. Its interrupt vector is located at 0xFFFA-0xFFFB.
void m65_nmi(m6502_t* cpu)
{
	// Set up the interrupt
	puts("nonmaskable interrupt pending");
	cpu->handle_interrupt = true;
	cpu->int_brk = false;
	cpu->int_dsi = true;
	cpu->int_vec = 0xFFFA;
}

// m65_res(m6502_t*) -> void
// Triggers a reset interrupt. Its interrupt vector is located at 0xFFFC-0xFFFD.
void m65_res(m6502_t* cpu)
{
	// Set up the interrupt
	puts("reset pending");
	cpu->handle_interrupt = true;
	cpu->int_rw = READ;
	cpu->int_brk = false;
	cpu->int_dsi = true;
	cpu->int_vec = 0xFFFC;
}

// m65_irq(m6502_t*) -> void
// Triggers an interrupt request if interrupts are not disabled. Its interrupt vector is located at 0xFFFE-0xFFFF.
void m65_irq(m6502_t* cpu)
{
	// Don't do the interrupt if disabled
	if (cpu->flags & 0x04)
		return;

	// Set up the interrupt
	puts("interrupt request pending");
	cpu->handle_interrupt = true;
	cpu->int_brk = false;
	cpu->int_dsi = true;
}

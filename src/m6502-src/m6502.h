//
// MOS6502 Emulator
// m6502.h: Header file for m6502.c.
//
// Created by jenra.
// Created on June 8 2020.
//

#ifndef M6502_H
#define M6502_H

#include <stdbool.h>
#include <inttypes.h>

#define READ 1
#define WRITE 0

typedef struct s_m6502 m6502_t;

// (m6502_t*) -> bool
// Represents an addressing mode function. Returns true if addressing for the instruction is done.
typedef bool (*addr_fn )(m6502_t*);

// (m6502_t*) -> bool
// Represents an instruction function. Returns true if the instruction is done executing and the cpu is ready to fetch.
typedef bool (*instr_fn)(m6502_t*);

// Represents the pins via which outside components may interact with the processor.
typedef struct
{
	// 16 bit addressing output from ABL and ABH.
	uint16_t addr;

	// 8 bit data input and output read from and stored to memory.
	uint8_t data;

	// If true, the processor is reading; if false, the processor is writing
	bool rw;
} m65_pins_t;

// Represents the state of a 6502 processor.
struct s_m6502
{
	// The main registers (accumulator, indexing, and stack pointer).
	uint8_t a, x, y, s;

	// The flags in the current execution state. The byte is formatted as:
	// NV-B DIZC
	// 7654 3210
	// N - Negative flag
	// V - oVerflow flag
	// (bit 5 is unused)
	// B - Brk flag (on when interrupt was called by BRK)
	// D - binary coded Decimal operations
	// I - disable Interrupt flag
	// Z - Zero flag
	// C - Carry flag
	uint8_t flags;

	// The program counter.
	uint16_t pc;

	// Represents the ALU of the processor.
	struct
	{
		// a and b are the inputs, c is the output;
		uint8_t a, b, c;
	} alu;

	// The current instruction being executed.
	instr_fn instr;

	// The addressing mode of the current instruction.
	addr_fn addr_mode;

	// The instruction register holds the opcode of the currently executing instruction.
	uint8_t ir;

	// The instruction program counter holds the current
	uint8_t ipc;

	// Whether the processor has to handle a hardware interrupt or executes normally.
	bool handle_interrupt;

	// False if the currently handled interrupt writes the program counter and status to the stack.
	bool int_rw;

	// Whether the interrupt enables or disables the break flag.
	bool int_brk;

	// Whether the interrupt disables or enables future interrupts.
	bool int_dsi;

	// The vector for the currently handled interrupt.
	uint16_t int_vec;

	// The buffers for the addressing pins.
	uint16_t addr_buf;

	// The pins of the processor.
	m65_pins_t pins;
};

// init_6502(m6502_t*) -> void
// Initialises a 6502 processor.
void init_6502(m6502_t* cpu);

// m65_cycle(m6502_t*) -> void
// Executes one cycle of a 6502 processor.
void m65_cycle(m6502_t* cpu);

// m65_nmi(m6502_t*) -> void
// Triggers a nonmaskable interrupt. Its interrupt vector is located at 0xFFFA-0xFFFB.
void m65_nmi(m6502_t* cpu);

// m65_res(m6502_t*) -> void
// Triggers a reset interrupt. Its interrupt vector is located at 0xFFFC-0xFFFD.
void m65_res(m6502_t* cpu);

// m65_irq(m6502_t*) -> void
// Triggers an interrupt request if interrupts are not disabled. Its interrupt vector is located at 0xFFFE-0xFFFF.
void m65_irq(m6502_t* cpu);

#endif /* M6502_H */

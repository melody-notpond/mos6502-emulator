//
// MOS6502 Emulator
// main.c: Runs the command line interface for the emulator.
//
// Created by jenra.
// Created on June 8 2020.
//

#include <stdio.h>

#include "m6502-src/m6502.h"

int main()
{
	// Init cpu
	m6502_t cpu;
	init_6502(&cpu);

	// Init memory
	uint8_t memory[1 << 16] = { 0 };
	memory[0x1234] = 0x40;

	memory[0x8000] = 0xa9;
	memory[0x8001] = 0x01;
	memory[0x8002] = 0x69;
	memory[0x8003] = 0x7f;

	memory[0xfffc] = 0x00;
	memory[0xfffd] = 0x80;

	// Reset the CPU to prepare it for execution
	m65_res(&cpu);

	// Do a few cycles of operation
	for (int i = 0; i < 11; i++)
	{
		// Read
		if (cpu.pins.rw == READ)
			cpu.pins.data = memory[cpu.pins.addr];

		// Write
		else if (cpu.pins.rw == WRITE)
			memory[cpu.pins.addr] = cpu.pins.data;

		// Debug info
		if (i > 0)
		{
			printf("addr in pins: 0x%04x\n", cpu.pins.addr);
			printf("data in pins: 0x%02x\n", cpu.pins.data);
		}
		printf("\ncycle #%i:\n", i);

		// Processor cycle
		m65_cycle(&cpu);

		// Debug info
		printf("flags: 0x%02x\n", cpu.flags);
		printf("accumulator: 0x%02x\n", cpu.a);
		printf("x register: 0x%02x\n", cpu.x);
		printf("y register: 0x%02x\n", cpu.y);
		printf("stack pointer: 0x01%02x\n", cpu.s);
		printf("program counter: 0x%04x\n", cpu.pc);
		printf("pin mode: %s\n", (cpu.pins.rw == READ ? "read" : "write"));
	}

	printf("Accumulator: 0x%02x\n", cpu.a);
	printf("Flags: 0x%02x\n", cpu.flags);
	printf("memory 0x42: 0x%02x\n", memory[0x42]);

	return 0;
}

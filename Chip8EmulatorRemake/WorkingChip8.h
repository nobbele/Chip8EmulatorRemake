#pragma once

#include <cinttypes>
#include <SDL.h>
#include "Chip8.h"

template<typename T>
constexpr T get_nibble(const T num, const unsigned int mask, const unsigned int nibble_shift)
{
	return (num & mask) >> (4 * nibble_shift);
}

struct WorkingChip8
{
	bool redraw = false;
	bool exit = false;
	bool halted = false;

	Chip8 *const chip;
	WorkingChip8(Chip8 *const chip);

	uint16_t *current_stack_value_ptr();
	void push_stack(uint16_t value);
	uint16_t pop_stack();

	bool execute(uint16_t inst);

	unsigned long cycle_count = 0;
	void run_cycle();

	void draw(SDL_Renderer *renderer);
};
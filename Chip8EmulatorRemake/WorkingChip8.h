#pragma once

#include <cinttypes>
#include <SDL.h>
#include "Chip8.h"

template<typename RT, typename T>
constexpr RT get_nibble(const T num, const unsigned int mask, const unsigned int nibble_shift)
{
	return static_cast<RT>((num & mask) >> (4 * nibble_shift));
}

struct WorkingChip8
{
	bool redraw = false;
	bool halted = false;
	bool waiting_for_input = false;

	Chip8 *const chip;
	WorkingChip8(Chip8 *const chip);

	void load_program(const uint8_t *const data, const size_t data_size);
	void reset();

	uint16_t *current_stack_value_ptr();
	void push_stack(const uint16_t value);
	uint16_t pop_stack();

	bool execute(const uint16_t inst);

	unsigned long cycle_count = 0;
	void run_cycle();

	void draw(SDL_Renderer *const renderer, const unsigned int offsetX = 0, const unsigned int offsetY = 0, const int pixel_scale = 0);
};
#include "WorkingChip8.h"
#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <ctime>

WorkingChip8::WorkingChip8(Chip8 *const chip)
	: chip(chip)
{
	halted = false;
	redraw = false;

	cycle_count = 0;
}

void WorkingChip8::load_program(uint8_t *data, size_t data_size)
{
	printf("Loading %zu bytes\n", data_size);
	for (int i = 0; i < data_size; i++) {
		chip->memory.data[i + 512] = data[i];
	}
}

void WorkingChip8::reset()
{
	halted = false;
	waiting_for_input = false;
	cycle_count = 0;

	chip->registers.PC = 0x200;
	chip->registers.I = 0;
	chip->registers.SP = 0;

	for (unsigned int i = 0; i < chip->registers.Vregister_count; i++) {
		chip->registers.V[i] = 0;
	}

	for (unsigned int i = 0; i < chip->stack.size; i++) {
		chip->stack.data[i] = 0;
	}

	for (unsigned int i = 0; i < chip->screen.width * chip->screen.height; i++) {
		chip->screen.data[i] = 0;
	}

	chip->registers.DT = 0;
	chip->registers.ST = 0;
}

uint16_t *WorkingChip8::current_stack_value_ptr()
{
	return chip->stack.data + chip->registers.SP;
}

void WorkingChip8::push_stack(uint16_t value)
{
	chip->registers.SP++;
	*current_stack_value_ptr() = value;
}

uint16_t WorkingChip8::pop_stack()
{
	int n = *current_stack_value_ptr();
	chip->registers.SP--;
	return n;
}

bool WorkingChip8::execute(uint16_t inst)
{
	switch (get_nibble<uint8_t>(inst, 0xF000, 3)) {
		case 0x0:
		{
			switch (inst & 0x00FF) {
				case 0xE0:
				{
					for (unsigned int i = 0; i < chip->screen.width * chip->screen.height; i++) 
					{
						chip->screen.data[i] = 0;
					}
				} break;
				case 0xEE:
				{
					chip->registers.PC = pop_stack();
				} break;
				case 0x00: // Halt in null memory
				case 0xFD:
				{
					halted = true;
				} break;
				default: return false;
			}
		} break;
		case 0x1:
		{
			uint16_t addr = inst & 0x0FFF;
			chip->registers.PC = addr - 2;
		} break;
		case 0x2:
		{
			uint16_t addr = inst & 0x0FFF;
			push_stack(chip->registers.PC);
			chip->registers.PC = addr;
		} break;
		case 0x3:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint16_t value = inst & 0x00FF;
			if (chip->registers.V[register_index] == value) chip->registers.PC += 2;
		} break;
		case 0x4:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint16_t value = inst & 0x00FF;
			if (chip->registers.V[register_index] != value) chip->registers.PC += 2;
		} break;
		case 0x5:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint8_t other_register_index = get_nibble<uint8_t>(inst, 0x00F0, 1);
			if (chip->registers.V[register_index] == chip->registers.V[other_register_index]) chip->registers.PC += 2;
		} break;
		case 0x6:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint8_t value = inst & 0x00FF;
			chip->registers.V[register_index] = value;
		} break;
		case 0x7:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint16_t value = inst & 0x00FF;
			chip->registers.V[register_index] += value;
		} break;
		case 0x8:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint8_t other_register_index = get_nibble<uint8_t>(inst, 0x00F0, 1);
			switch (inst & 0x000F) {
				case 0x0:
				{
					chip->registers.V[register_index] = chip->registers.V[other_register_index];
				} break;
				case 0x1:
				{
					chip->registers.V[register_index] |= chip->registers.V[other_register_index];
				} break;
				case 0x2:
				{
					chip->registers.V[register_index] &= chip->registers.V[other_register_index];
				} break;
				case 0x3:
				{
					chip->registers.V[register_index] ^= chip->registers.V[other_register_index];
				} break;
				case 0x4:
				{
					uint16_t res = (uint16_t)chip->registers.V[register_index] + chip->registers.V[other_register_index];
					chip->registers.VF = res > 0xFF ? 1 : 0;
					chip->registers.V[register_index] = (uint8_t)(res & 0xFFFF);
				} break;
				case 0x5:
				{
					uint8_t Vx = chip->registers.V[register_index];
					uint8_t Vy = chip->registers.V[other_register_index];
					if (Vy > Vx) chip->registers.VF;
					chip->registers.V[register_index] = Vx - Vy;
				} break;
				case 0x6:
				{
					chip->registers.VF = (chip->registers.V[register_index] & 1) != 0 ? 1 : 0;
					chip->registers.V[register_index] >>= 1;
				} break;
				case 0x7:
				{
					uint8_t Vx = chip->registers.V[register_index];
					uint8_t Vy = chip->registers.V[other_register_index];
					if (Vy > Vx) chip->registers.VF;
					chip->registers.V[register_index] = Vy - Vx;
				} break;
				case 0xE:
				{
					(chip->registers.VF = chip->registers.V[register_index] & (1 << 7)) != 0 ? 1 : 0;
					chip->registers.V[register_index] <<= 1;
				} break;
				default: return false;
			}
		} break;
		case 0x9:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint8_t other_register_index = get_nibble<uint8_t>(inst, 0x00F0, 1);
			if (chip->registers.V[register_index] != chip->registers.V[other_register_index]) chip->registers.PC++;
		} break;
		case 0xA:
		{
			uint16_t val = inst & 0x0FFF;
			chip->registers.I = val;
		} break;
		case 0xB:
		{
			uint16_t addr = inst & 0x0FFF;
			chip->registers.PC = chip->registers.V0 + addr;
		} break;
		case 0xC:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			uint16_t value = inst & 0x00FF;
			int n = rand() % 256;
			chip->registers.V[register_index] += n & value;
		} break;
		case 0xD:
		{
			uint8_t pos_x = chip->registers.V[get_nibble<uint8_t>(inst, 0x0F00, 2)];
			uint8_t pos_y = chip->registers.V[get_nibble<uint8_t>(inst, 0x00F0, 1)];
			uint8_t n = inst & 0x000F;
			chip->registers.VF = 0;
			for (int y = 0; y < n; y++)
			{
				uint8_t pixel = chip->memory.data[chip->registers.I + y];
				for (int x = 0; x < 8; x++)
				{
					if ((pixel & (0x80 >> x)) != 0) 
					{
						unsigned int index = ((pos_x + x) % chip->screen.width) + (unsigned int)(((pos_y + y) % chip->screen.height) * chip->screen.width);
						if (chip->screen.data[index] == 1)
							chip->registers.VF = 1;
						chip->screen.data[index] ^= 1;
					}
				}
			}
			redraw = true;
		} break;
		case 0xE:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			switch (inst & 0x00FF) {
				case 0x9E:
				{
					uint8_t key = chip->registers.V[register_index];
					if (chip->keyboard.data[key]) chip->registers.PC += 2;
				} break;
				case 0xA1:
				{
					uint8_t key = chip->registers.V[register_index];
					if (!chip->keyboard.data[key]) chip->registers.PC += 2;
				} break;
				default: return false;
			}
		}
		case 0xF:
		{
			uint8_t register_index = get_nibble<uint8_t>(inst, 0x0F00, 2);
			switch (inst & 0x00FF) {
				case 0x07:
				{
					chip->registers.V[register_index] = chip->registers.DT;
				} break;
				case 0x0A:
				{
					if (!waiting_for_input) {
						waiting_for_input = true;
					}
					for (int i = 0; i < 16; i++) 
					{
						if (chip->keyboard.data[i]) 
						{
							chip->registers.V[register_index] = i;
							waiting_for_input = false;
							break;
						}
					}
				}
				case 0x15:
				{
					chip->registers.DT = chip->registers.V[register_index];
				} break;
				case 0x18:
				{
					chip->registers.ST = chip->registers.V[register_index];
				} break;
				case 0x1E:
				{
					chip->registers.I += chip->registers.V[register_index];
				} break;
				case 0x29:
				{
					uint8_t key = chip->registers.V[register_index];
					chip->registers.I = 0x50 + key * 5;
				} break;
				case 0x33:
				{
					chip->memory.data[chip->registers.I] = chip->registers.V[register_index] / 100;
					chip->memory.data[chip->registers.I + 1] = chip->registers.V[register_index] / 10 % 100;
					chip->memory.data[chip->registers.I + 2] = chip->registers.V[register_index] % 10;
				} break;
				case 0x55:
				{
					for (int i = 0; i <= register_index ; i++) {
						chip->memory.data[chip->registers.I + i] = chip->registers.V[i];
					}
				} break;
				case 0x65:
				{
					for (int i = 0; i <= register_index; i++) {
						chip->registers.V[i] = chip->memory.data[chip->registers.I + i];
					}
				} break;
				default: return false;
			}
		} break;
		default: return false;
	}
	return true;
}

void WorkingChip8::run_cycle()
{
	uint16_t inst = ((uint16_t)chip->memory.data[chip->registers.PC] << 8) | chip->memory.data[chip->registers.PC + 1];
	if (!execute(inst)) 
	{
		printf("Unknown opcode %04x cycle %d line %d PC %04x\n", inst, cycle_count, (chip->registers.PC - 0x200) / 2, chip->registers.PC);
	}
	else
	{
		printf("%04x line %d PC %04x\n", inst, (chip->registers.PC - 0x200) / 2, chip->registers.PC);
	}

	if (waiting_for_input) return;
	for (int i = 0; i < 16; i++) {
		chip->keyboard.data[i] = false;
	}

	chip->registers.PC += 2;
	if (chip->registers.PC < 0x200) printf("Detected possible OOB code execution\n");

	cycle_count++;
}

void WorkingChip8::draw(SDL_Renderer *renderer, unsigned int offsetX, unsigned int offsetY, int pixel_scale)
{
	SDL_Rect *rects = new SDL_Rect[chip->screen.width * chip->screen.width];
	for (unsigned int x = 0; x < chip->screen.width; x++) 
	{
		for (unsigned int y = 0; y < chip->screen.height; y++) 
		{
			size_t index = x + (y * chip->screen.width);
			if (chip->screen.data[index])
			{
				rects[index] = { (int)(x * pixel_scale + offsetX), (int)(y * pixel_scale + offsetY), pixel_scale, pixel_scale };
			}
		}
	}
	SDL_SetRenderDrawColor(renderer, 0x66, 0xFF, 0x66, 0xFF);
	SDL_RenderFillRects(renderer, rects, (int)(chip->screen.width * chip->screen.width));
	delete rects;
}
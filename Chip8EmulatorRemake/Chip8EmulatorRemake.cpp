#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <ctime>
#include <SDL.h>

unsigned char chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

struct Chip8
{
	struct Screen
	{
		const size_t width;
		const size_t height;
		uint8_t *const data;
		Screen(const size_t width, const size_t height)
			: width(width), height(height), data(new uint8_t[width * height])
		{}
	} screen;
	struct Memory
	{
		const size_t size;
		uint8_t *const data;
		Memory(const size_t size)
			: size(size), data(new uint8_t[size])
		{}
	} memory;
	struct Stack
	{
		const size_t size;
		uint16_t *const data;
		Stack(const size_t size)
			: size(size), data(new uint16_t[size])
		{}
	} stack;
	struct Registers
	{
		static const size_t Vregister_count = 16;
		//General purpose registers inside a union for 2 ways to access them
		union
		{
			struct
			{
				uint8_t V0;
				uint8_t V1;
				uint8_t V2;
				uint8_t V3;
				uint8_t V4;
				uint8_t V5;
				uint8_t V6;
				uint8_t V7;
				uint8_t V8;
				uint8_t V9;
				uint8_t VA;
				uint8_t VB;
				uint8_t VC;
				uint8_t VD;
				uint8_t VE;
				uint8_t VF;
			};
			uint8_t V[Vregister_count];
		};
		uint16_t I;
		//Sound Registers
		uint8_t DT;
		uint8_t ST;
		//Special Registers
		uint16_t PC;
		uint8_t SP;
	} registers;
	struct Keyboard
	{
		static const size_t size = 16;
		bool data[size];
	} keyboard;
	Chip8(const size_t memory_size, const size_t stack_size, const size_t screen_width, const size_t screen_height)
		: memory(Memory(memory_size)), screen(Screen(screen_width, screen_height)), stack(Stack(stack_size))
	{
		registers.PC = 0x200;
		registers.I = 0;
		registers.SP = 0;

		for (int i = 0; i < registers.Vregister_count; i++) 
		{
			registers.V[i] = 0;
		}

		for (int i = 0; i < stack.size; i++) 
		{
			stack.data[i] = 0;
		}

		for (int i = 0; i < memory_size; ++i) 
		{
			memory.data[i] = 0;
		}
		for (int i = 0; i < 80; ++i) 
		{
			memory.data[0x50 + i] = chip8_fontset[i];
		}

		for (int i = 0; i < screen_width * screen_height; i++) 
		{
			screen.data[i] = 0;
		}

		registers.DT = 0;
		registers.ST = 0;
	}
};

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
	WorkingChip8(Chip8 *const chip)
		: chip(chip)
	{}

	uint16_t *current_stack_value_ptr()
	{
		return chip->stack.data + chip->registers.SP;
	}

	void push_stack(uint16_t value)
	{
		chip->registers.SP++;
		*current_stack_value_ptr() = value;
	}

	uint16_t pop_stack()
	{
		int n = *current_stack_value_ptr();
		chip->registers.SP--;
		return n;
	}

	bool execute(uint16_t inst)
	{
		switch (get_nibble(inst, 0xF000, 3)) 
		{
			case 0x0:
			{
				switch (inst & 0x00FF) 
				{
					case 0xE0:
					{
						for (int i = 0; i < chip->screen.width * chip->screen.height; i++) {
							chip->screen.data[i] = 0;
						}
					} break;
					case 0xEE:
					{
						chip->registers.PC = pop_stack();
					} break;
					case 0xFD:
					{
						exit = true;
					}
				}
			} break;
			case 0x1:
			{
				uint16_t addr = inst & 0x0FFF;
				chip->registers.PC = addr;
			} break;
			case 0x2:
			{
				uint16_t addr = inst & 0x0FFF;
				push_stack(chip->registers.PC);
				chip->registers.PC = addr;
			} break;
			case 0x3:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint16_t value = inst & 0x00FF;
				if (chip->registers.V[register_index] == value) chip->registers.PC++;
			} break;
			case 0x4:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint16_t value = inst & 0x00FF;
				if (chip->registers.V[register_index] != value) chip->registers.PC++;
			} break;
			case 0x5:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint8_t other_register_index = get_nibble(inst, 0x00F0, 1);
				if (chip->registers.V[register_index] == chip->registers.V[other_register_index]) chip->registers.PC++;
			} break;
			case 0x6:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint16_t value = inst & 0x00FF;
				chip->registers.V[register_index] = value;
			} break;
			case 0x7:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint16_t value = inst & 0x00FF;
				chip->registers.V[register_index] += value;
			} break;
			case 0x8:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint8_t other_register_index = get_nibble(inst, 0x00F0, 1);
				switch(inst & 0x000F)
				{
					case 0:
					{
						chip->registers.V[register_index] = chip->registers.V[other_register_index];
					} break;
					case 1:
					{
						chip->registers.V[register_index] |= chip->registers.V[other_register_index];
					} break;
					case 2:
					{
						chip->registers.V[register_index] &= chip->registers.V[other_register_index];
					} break;
					case 3:
					{
						chip->registers.V[register_index] ^= chip->registers.V[other_register_index];
					} break;
					case 4:
					{
						uint16_t res = (uint16_t)chip->registers.V[register_index] + chip->registers.V[other_register_index];
						chip->registers.VF = res > 0xFF ? 1 : 0;
						chip->registers.V[register_index] = (uint8_t)(res & 0xFFFF);
					} break;
					case 5:
					{
						uint8_t Vx = chip->registers.V[register_index];
						uint8_t Vy = chip->registers.V[other_register_index];
						if (Vy > Vx) chip->registers.VF;
						chip->registers.V[register_index] = Vx - Vy;
					} break;
					case 6:
					{
						chip->registers.VF = chip->registers.V[register_index] & 1 != 0 ? 1 : 0;
						chip->registers.V[register_index] >>= 1;
					} break;
					case 7:
					{
						uint8_t Vx = chip->registers.V[register_index];
						uint8_t Vy = chip->registers.V[other_register_index];
						if (Vy > Vx) chip->registers.VF;
						chip->registers.V[register_index] = Vy - Vx;
					} break;
					case 0xE:
					{
						chip->registers.VF = chip->registers.V[register_index] & (1 << 7) != 0 ? 1 : 0;
						chip->registers.V[register_index] <<= 1;
					} break;
				}
			} break;
			case 0x9:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint8_t other_register_index = get_nibble(inst, 0x00F0, 1);
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
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				uint16_t value = inst & 0x00FF;
				chip->registers.V[register_index] += (rand() % 256) & value;
			} break;
			case 0xD:
			{
				uint8_t x = get_nibble(inst, 0x0F00, 2);
				uint8_t y = get_nibble(inst, 0x00F0, 1);
				uint8_t n = inst & 0x000F;
				chip->registers.VF = 0;
				for (int y = 0; y < n; y++) {
					uint8_t pixel = chip->memory.data[chip->registers.I + y];
					for (int x = 0; x < 8; x++) {
						if ((pixel & (0x80 >> x)) != 0) {
							int index = x + (y * chip->screen.width);
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
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
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
				}
			}
			case 0xF:
			{
				uint8_t register_index = get_nibble(inst, 0x0F00, 2);
				switch (inst & 0x00FF) {
					case 0x07:
					{
						chip->registers.V[register_index] = chip->registers.DT;
					} break;
					case 0x0A:
					{
						chip->registers.V[register_index] = /*Wait for input*/0;
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
						chip->memory.data[chip->registers.I + 1] = chip->registers.V[register_index] % 10;
						chip->memory.data[chip->registers.I + 2] = chip->registers.V[register_index] % 10;
					} break;
					case 0x55:
					{
						for (int i = 0; i < register_index; i++)
						{
							chip->memory.data[chip->registers.I + i] = chip->registers.V[i];
						}
					} break;
					case 0x65:
					{
						for (int i = 0; i < register_index; i++) {
							chip->registers.V[i] = chip->memory.data[chip->registers.I + i];
						}
					} break;
				}
			}
			default: return false;
		}
		return true;
	}

	unsigned long cycle_count = 0;

	void run_cycle()
	{
		uint16_t inst = ((uint16_t)chip->memory.data[chip->registers.PC] << 8) | chip->memory.data[chip->registers.PC + 1];
		if (!execute(inst)) 
		{
			printf("Unknown opcode %x c%d\n", inst, cycle_count);
		}
		chip->registers.PC += 2;
		if (chip->registers.PC < 0x200) printf("Detected possible OOB code execution\n");
		cycle_count++;
	}
};

const int pixel_scale = 10;

#undef main
int main()
{
	srand(time(NULL));

	Chip8 chip8(4096, 16, 64, 32);
	WorkingChip8 workingChip8(&chip8);

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		chip8.screen.width * pixel_scale, chip8.screen.height * pixel_scale,
		SDL_WINDOW_SHOWN
	);
	if (!window) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 2;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		printf("Renderers could not be created! SDL_Error: %s\n", SDL_GetError());
		return 3;
	}

	SDL_RenderSetLogicalSize(renderer, chip8.screen.width, chip8.screen.height);

	load:
	uint8_t program[] = { 0x60,0x02, 0xF0,0x29, 0xD5,0x55, 0x00,0xFD };
	for (int i = 0; i < sizeof(program) / sizeof(*program); i++)
	{
		chip8.memory.data[i + 512] = program[i];
	}
	while (true) 
	{
		if (workingChip8.halted) 
		{

		}
		else {
			workingChip8.run_cycle();

			if (workingChip8.exit) {
				workingChip8.exit = false;
				workingChip8.halted = true;
				printf("Halted\n");
				continue;
			}

			if (workingChip8.redraw) {
				SDL_RenderClear(renderer);

				for (int x = 0; x < chip8.screen.width; x++) {
					for (int y = 0; y < chip8.screen.height; y++) {
						if (chip8.screen.data[x + (y * chip8.screen.width)]) 
						{
							SDL_SetRenderDrawColor(renderer, 0x66, 0xFF, 0x66, 0xFF);
							printf("Drawing %d, %d\n", x, y);
						}
						SDL_Rect pixel = SDL_Rect({ x, y, 1, 1 });
						SDL_RenderFillRect(renderer, &pixel);
						SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
					}
				}

				SDL_RenderPresent(renderer);
			}
		}

		SDL_Delay(1 / 60);
	}

	return 0;
}
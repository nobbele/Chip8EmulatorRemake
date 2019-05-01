#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <ctime>
#include <SDL.h>
#include "WorkingChip8.h"

const int pixel_scale = 10;

// SDL pls
#undef main

int main()
{
	srand((unsigned int)time(nullptr));

	Chip8 chip8(4096, 16, 64, 32);
	WorkingChip8 workingChip8(&chip8);

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) 
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		chip8.screen.width * pixel_scale, chip8.screen.height * pixel_scale,
		SDL_WINDOW_SHOWN
	);
	if (!window) 
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 2;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) 
	{
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
		else 
		{
			workingChip8.run_cycle();

			if (workingChip8.exit) 
			{
				workingChip8.exit = false;
				workingChip8.halted = true;
				printf("Halted\n");
				continue;
			}

			SDL_RenderClear(renderer);

			if (workingChip8.redraw) 
			{
				workingChip8.draw(renderer);
			}

			SDL_RenderPresent(renderer);
		}

		SDL_Delay(1 / 60);
	}


	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
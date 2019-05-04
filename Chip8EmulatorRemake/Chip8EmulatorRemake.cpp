#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <ctime>
#include <cerrno>
#include <SDL.h>
#include <SDL_ttf.h>
#include <functional>
// SDL pls
#undef main
#include "WorkingChip8.h"
#include "filedialog.h"

const int pixel_scale = 10;
const int gui_height = 20;
int window_width = 0;
int window_height = 0;

const unsigned int menu_item_count = 3;
const int menu_item_spacing = 2;

double time_scale = 1;

TTF_Font *arial;

SDL_Texture *get_string_texture(SDL_Renderer *renderer, const char *const str, SDL_Color text_color)
{
	SDL_Surface *message_surface = TTF_RenderText_Solid(arial, str, text_color);
	SDL_Texture *message_texture = SDL_CreateTextureFromSurface(renderer, message_surface);
	SDL_FreeSurface(message_surface);
	return message_texture;
}

inline int get_item_width()
{
	return window_width / menu_item_count - menu_item_spacing;
}

struct menu_item
{
	const char *title;
	std::function<void()> on_click;

	inline SDL_Rect get_rect(const int index, const int spacing, const int item_width)
	{
		return { ((spacing / 2) + (spacing + item_width) * index), 0, item_width, gui_height };
	}

	inline SDL_Rect get_screen_rect(unsigned int index)
	{
		return get_rect(index, menu_item_spacing, get_item_width());
	}
};

menu_item menu_items[menu_item_count] = { { "Load ROM" }, { "Reset" }, { "Halt" } };

void draw_menu(SDL_Renderer *renderer)
{
	SDL_Rect menu = { 0, 0, window_width, gui_height };
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderFillRect(renderer, &menu);

	for (unsigned int i = 0; i < menu_item_count; i++) 
	{
		SDL_Rect border = menu_items[i].get_screen_rect(i);
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderDrawRect(renderer, &border);

		SDL_Color text_color = { 0, 0, 0, 0 };

		SDL_Surface *message_surface = TTF_RenderText_Solid(arial, menu_items[i].title, text_color);
		SDL_Texture *message_texture = SDL_CreateTextureFromSurface(renderer, message_surface);
		SDL_FreeSurface(message_surface);

		int message_width;
		int message_height;
		TTF_SizeText(arial, menu_items[i].title, &message_width, &message_height);
		SDL_Rect text_rect = { border.x + (border.w - message_width) / 2, border.y, message_width, message_height };

		SDL_RenderCopy(renderer, message_texture, nullptr, &text_rect);
		SDL_DestroyTexture(message_texture);

	}
}

int main()
{
	srand(static_cast<unsigned int>(time(nullptr)));

	Chip8 chip8(4096, 16, 64, 32);
	WorkingChip8 workingChip8(&chip8);

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) 
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	window_width = static_cast<int>(chip8.screen.width * pixel_scale);
	window_height = static_cast<int>(chip8.screen.height * pixel_scale + gui_height);

	SDL_Window* window = SDL_CreateWindow("CHIP-8 Emulator",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		window_width, window_height,
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

	SDL_RenderSetLogicalSize(renderer, window_width, window_height);

	if (TTF_Init() < 0) {
		printf("SDL_ttf could not initialize! SDL_Error: %s\n", TTF_GetError());
		return -1;
	};
	arial = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 18);

	const char *const halt_text = "Halted";
	SDL_Texture *halt_text_texture = get_string_texture(renderer, halt_text, { 0xFF, 0xFF, 0xFF, 0xFF });
	int halt_text_width;
	int halt_text_height;
	TTF_SizeText(arial, halt_text, &halt_text_width, &halt_text_height);

	const uint8_t *program = new uint8_t[8]{ 0x60,0x02, 0xF0,0x29, 0xD5,0x55, 0x00,0xFD };
	size_t program_size = 8;

	// Load ROM
	menu_items[0].on_click = [&workingChip8, &program, &program_size]()
	{
		// TODO
		wchar_t path[FILEDIALOGBUFFERSIZE];
		open_file_dialog(path);
		printf("Loading %S\n", path);

		FILE *rom_file;
		errno_t err = _wfopen_s(&rom_file, path, L"rb");
		if (err != 0)
		{
			size_t errmsglen = 95; // 94 char + null is the longest message
			char *errmsg = new char[errmsglen];
			strerror_s(errmsg, errmsglen, err);
			printf("Couldn't Open ROM '%s'", errmsg);
			delete errmsg;
			return;
		}

		fseek(rom_file, 0, SEEK_END);
		program_size = ftell(rom_file);
		fseek(rom_file, 0, SEEK_SET);

		delete program;
		uint8_t *buf = new uint8_t[program_size];
		fread(buf, sizeof(uint8_t), program_size, rom_file);
		program = buf;

		workingChip8.reset();
		workingChip8.load_program(program, program_size);

		fclose(rom_file);
	}; 
	// Reset
	menu_items[1].on_click = [&workingChip8, &program, &program_size]()
	{
		workingChip8.reset();
		workingChip8.load_program(program, program_size);
	};
	// Halt
	menu_items[2].on_click = [&workingChip8]()
	{
		workingChip8.halted ^= 1;
	};

	while (true) 
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) 
		{
			switch (event.type) 
			{
				case SDL_EventType::SDL_MOUSEBUTTONDOWN:
				{
					SDL_Point point;
					SDL_GetMouseState(&point.x, &point.y);
					for (unsigned int i = 0; i < menu_item_count; i++) {
						SDL_Rect rect = menu_items[i].get_screen_rect(i);
						if (SDL_PointInRect(&point, &rect)) 
						{
							if (menu_items[i].on_click)
								menu_items[i].on_click();
						}
					}
				} break;
				case SDL_EventType::SDL_QUIT:
				{
					goto exit;
				} break;
			}
		}

		SDL_RenderClear(renderer);

		if (workingChip8.halted) 
		{
			SDL_Rect text_rect = { (window_width - halt_text_width) / 2, (window_height - halt_text_height) / 2, halt_text_width, halt_text_height };
			SDL_RenderCopy(renderer, halt_text_texture, nullptr, &text_rect);
		}
		else
		{
			workingChip8.run_cycle();
		}

		if (workingChip8.redraw) {
			workingChip8.draw(renderer, 0, gui_height, pixel_scale);
		}

		draw_menu(renderer);

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderPresent(renderer);

		SDL_Delay(static_cast<unsigned int>((1.0f / 60 * 1000) / time_scale));
	}
	exit:

	delete program;
	SDL_DestroyTexture(halt_text_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
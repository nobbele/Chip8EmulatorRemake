#pragma once

#include <cinttypes>

struct Chip8
{
	uint8_t *fontset;
	struct Screen
	{
		const size_t size;
		const size_t width;
		const size_t height;
		uint8_t *const data;
		Screen(const size_t width, const size_t height)
			: width(width), height(height), size(width * height), data(new uint8_t[size])
		{}
		~Screen()
		{
			delete data;
		}
	} screen;
	struct Memory
	{
		const size_t size;
		uint8_t *const data;
		Memory(const size_t size)
			: size(size), data(new uint8_t[size])
		{}
		~Memory()
		{
			delete data;
		}
	} memory;
	struct Stack
	{
		const size_t size;
		uint16_t *const data;
		Stack(const size_t size)
			: size(size), data(new uint16_t[size])
		{}
		~Stack()
		{
			delete data;
		}
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
		: memory(Memory(memory_size)), screen(Screen(screen_width, screen_height)), stack(Stack(stack_size)), 
	    fontset(new uint8_t[80] {
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
	    })
	{}

	~Chip8()
	{
		delete fontset;
	}
};
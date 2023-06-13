#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <conio.h>
#include <windows.h>
#include "Header.h"
#include <SDL.h>

/*GLOBAL VARIABLES*/
unsigned short stack[16];// the stack
unsigned short sp; //stack pointer
unsigned short opcode; //35 opcodes 2 bytes
unsigned char memory[4096]; //4k total memory
unsigned char V[16]; //Registers V0-VE, 16th is carry flag
unsigned short I; // index register
unsigned short pc; // program counter
unsigned char gfx[64][32];//64*32 display - ON/OFF
unsigned char delay_timer; //TIME REGISETR
unsigned char sound_timer;//TIME REGISETR
unsigned char key[16]; // 0X0 to 0XF 
long romSize;// size of rom
char font[80] = //Fonset Hexdemcial
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


/*FUNCTIONS*/

void reset()
{
	pc = 0x200;	//512	
	opcode = 0;			
	I = 0;			
	sp = 0;			
	delay_timer = 0;
	sound_timer = 0;
	
	for (int x = 0; x < 64; ++x)
	{
		for (int y = 0; y < 32; ++y)
		{
			gfx[x][y] = 0x0;
		}
	}

	for (int i = 0; i < 16; ++i) {
		stack[i] = 0;
		V[i] = 0;
		key[i] = V[i];
	}
		for (int i = 0; i < 4096; ++i)
			memory[i] = 0;

		for (int i = 0; i < 80; ++i)
			memory[i] = font[i];

	}

void render(unsigned char gfx[64][32], SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 50, 255);
	SDL_RenderClear(renderer);
	for (int y = 0; y < 32; ++y)
	{
		for (int x = 0; x < 64; ++x)
		{
			if (gfx[x][y] == 1) {
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				SDL_RenderDrawPointF(renderer,x, y);
				
			}
		}
	
	}
	SDL_Delay(10);
	SDL_RenderPresent(renderer);
	
	
	
}







bool loadROM(const char* filename)
{
	reset();
	unsigned char* memPtr = &memory;
	printf("Loading: %s\n", filename);

	FILE* pFile = fopen(filename, "rb");
	if (pFile == NULL)
	{
		fputs("File error", stderr);
		return false;
	}

	fseek(pFile, 0, SEEK_END);
	romSize = ftell(pFile);
	rewind(pFile);

	char* buffer = (char*)malloc(sizeof(char) * romSize);
	
	size_t result = fread(buffer, 1, romSize, pFile);
	if (result != romSize)
	{
		fputs("Reading error", stderr);
		return false;
	}

	if (3584 > romSize) //memory - location of pc
	{
		for (int i = 0; i < romSize; ++i) {
			memPtr[i + 512] = buffer[i];
		}
	}
	else
		printf("Error: ROM too big for memory");

	
	fclose(pFile);
	free(buffer);
	return true;
}


//OPCODE SOURCE: https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
void emulateCycle(SDL_Renderer* renderer)
{
	// Fetch opcode
	opcode = memory[pc + 0];
	opcode <<= 8;
	opcode |= memory[pc + 1];

	// Process opcode
	switch (opcode & 0xF000)
	{
	case 0x0000:
		switch (opcode & 0x000F) //
		{
		case 0x0000: // 0x00E0
			for (int x = 0; x < 64; ++x)
			{
				for (int y = 0; y < 32; ++y)
				{
					gfx[x][y] = 0x0;
				}
			}
			render(gfx,renderer);
			pc += 2;
			break;

		case 0x000E: // 0x00EE
			--sp;			
			pc = stack[sp];	
			pc += 2;		
			break;

		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
		break;

	case 0x1000: // 0x1NNN
		pc = opcode & 0x0FFF;
		break;

	case 0x2000: // 0x2NNN
		stack[sp] = pc;			
		++sp;					
		pc = opcode & 0x0FFF;	
		break;

	case 0x3000: // 0x3XNN
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;

	case 0x4000: // 0x4XNN
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;

	case 0x5000: // 0x5XY0
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;

	case 0x6000: // 0x6XNN
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		pc += 2;
		break;

	case 0x7000: // 0x7XNN
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		pc += 2;
		break;

	case 0x8000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x8XY0
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0001: // 0x8XY1
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0002: // 0x8XY2
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0003: // 0x8XY3
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0004: // 0x8XY4
			if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 1; 
			else
				V[0xF] = 0;
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0005: // 0x8XY5
			if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
				V[0xF] = 0; 
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0006: // 0x8XY6
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
			V[(opcode & 0x0F00) >> 8] >>= 1;
			pc += 2;
			break;

		case 0x0007: // 0x8XY7
			if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])	
				V[0xF] = 0; 
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x000E: // 0x8XYE
			V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
			V[(opcode & 0x0F00) >> 8] <<= 1;
			pc += 2;
			break;

		default:
			printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
		}
		break;

	case 0x9000: // 0x9XY0
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;

	case 0xA000: // ANNN
		I = opcode & 0x0FFF;
		pc += 2;
		break;

	case 0xB000: // BNNN: Jumps to the address NNN plus V0
		pc = (opcode & 0x0FFF) + V[0];
		break;

	case 0xC000: // CXNN
		V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
		pc += 2;
		break;

	case 0xD000: // DXYN  THIS ONE
		
	{
		unsigned short x = V[(opcode & 0x0F00) >> 8];
		unsigned short y = V[(opcode & 0x00F0) >> 4];
		unsigned short height = opcode & 0x000F;
		unsigned short pixel;

		V[0xF] = 0;
		for (int yline = 0; yline < height; yline++)
		{
			pixel = memory[I + yline];
			for (int xline = 0; xline < 8; xline++)
			{
				if ((pixel & (0x80 >> xline)) != 0)
				{
					if (gfx[x+xline][(y + yline)] == 1)
					{
						V[0xF] = 1;
					}
					gfx[x + xline][(y + yline)] ^= 1;
				}
			}
		}

		render(gfx,renderer);
		pc += 2;
	}
	break;

	case 0xE000:
		switch (opcode & 0x00FF)
		{
		case 0x009E: // EX9E'
			printf("opcode [0xE000]: 0x%X\n", opcode);
			if (key[V[(opcode & 0x0F00) >> 8]] != 0)
				pc += 4;
			else
				pc += 2;
			break;

		case 0x00A1: // EXA1
			printf("opcode [0xE000]: 0x%X   is key %d pressed?  \n", opcode,V[(opcode & 0x0F00) >> 8]);
			
			if (key[V[(opcode & 0x0F00) >> 8]] == 0)
				pc += 4;
			else
				pc += 2;
			break;

		default:
			printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
		}
		break;

	case 0xF000:
		switch (opcode & 0x00FF)
		{
		case 0x0007: // FX07
			printf("opcode [0xF000]: 0x%X\n", opcode);
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			pc += 2;
			break;

		case 0x000A: // FX0A
		{
			printf("opcode [0xF000]: 0x%X\n", opcode);
			bool keyPress = false;

		

			for (int i = 0; i < 16; ++i)
			{
				if (key[i] != 0)
				{
					V[(opcode & 0x0F00) >> 8] = i;
					keyPress = true;
				}
			}

			
			if (!keyPress)
				return;

			pc += 2;
		}
		break;

		case 0x0015: // FX15
			delay_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0018: // FX18
			sound_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x001E: // FX1E
			if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)	
				V[0xF] = 1;
			else
				V[0xF] = 0;
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0029: 
			I = V[(opcode & 0x0F00) >> 8] * 0x5;
			pc += 2;
			break;

		case 0x0033: // FX33
			memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
			memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
			pc += 2;
			break;

		case 0x0055: // FX55	
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				memory[I + i] = V[i];

			
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;

		case 0x0065: // FX65			
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				V[i] = memory[I + i];

			
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;

		default:
			printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
		}
		break;

	default:
		printf("Unknown opcode: 0x%X\n", opcode);
	}

	
	if (delay_timer > 0)// update delay timer
		--delay_timer;

	if (sound_timer > 0)// update sound timer
	{
		if (sound_timer == 1)
		--sound_timer;
	}
}


/*MAIN*/
int main(int argc, char** argv)
{
	/// window
	SDL_Event e;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(640, 320, 0, &window, &renderer);
	SDL_RenderSetScale(renderer, 10, 10);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	

	char romName[100]="C:/Users/Camden/Downloads/Pong (1 player).ch8";
	printf("Type Rom: ");
	//scanf("%s", romName);
	const char* romPTR = &romName;
	loadROM(romPTR);
	
	/// window
	render(gfx,renderer);

  
	
    while (pc < (romSize + 0x200))
    {
        emulateCycle(renderer);
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				switch (e.key.keysym.sym) {
				case SDLK_0:
					key[0] = 1;
					break;
				case SDLK_1:
					key[1] = 1;
					break;
				case SDLK_2:
					key[2] = 1;
					break;
				case SDLK_3:
					key[3] = 1;
					break;
				case SDLK_4:
					key[4] = 1;
					break;
				case SDLK_5:
					key[5] = 1;
					break;
				case SDLK_6:
					key[6] = 1;
					break;
				case SDLK_7:
					key[7] = 1;
					break;
				case SDLK_8:
					key[8] = 1;
					break;
				case SDLK_9:
					key[9] = 1;
					break;
				case SDLK_q:
					key[10] = 1;
					break;
				case SDLK_w:
					key[11] = 1;
					break;
				case SDLK_e:
					key[12] = 1;
					break;
				case SDLK_r:
					key[13] = 1;
					break;
				case SDLK_t:
					key[14] = 1;
					break;
				case SDLK_y:
					key[15] = 1;
					break;
				default:
					break;
				}
			}
			else if (e.type == SDL_KEYUP) {
				switch (e.key.keysym.sym) {
				case SDLK_0:
					key[0] = 1;
					break;
				case SDLK_1:
					key[1] = 0;
					break;
				case SDLK_2:
					key[2] = 0;
					break;
				case SDLK_3:
					key[3] = 0;
					break;
				case SDLK_4:
					key[4] = 0;
					break;
				case SDLK_5:
					key[5] = 0;
					break;
				case SDLK_6:
					key[6] = 0;
					break;
				case SDLK_7:
					key[7] = 0;
					break;
				case SDLK_8:
					key[8] = 0;
					break;
				case SDLK_9:
					key[9] = 0;
					break;
				case SDLK_q:
					key[10] = 0;
					break;
				case SDLK_w:
					key[11] = 0;
					break;
				case SDLK_e:
					key[12] = 0;
					break;
				case SDLK_r:
					key[13] = 0;
					break;
				case SDLK_t:
					key[14] = 0;
					break;
				case SDLK_y:
					key[15] = 0;
					break;
				default:
					break;
				}
			}

			
		}

    }
	
    return 0;
}



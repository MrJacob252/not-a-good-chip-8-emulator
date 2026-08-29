#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdint.h>

// ************************************************************
// Defines

#define STEP_RATE_IN_MS 16.6667f // 60Hz ((1/60) * 1000)
#define SDL_WINDOW_WIDTH  64
#define SDL_WINDOW_HEIGHT 32

#define MEMORY_SIZE_IN_KIB 4
#define MEMORY_SIZE (MEMORY_SIZE_IN_KIB * 1024)
#define STACK_SIZE 5

#define FONT_START_ADDR 0x050

// ************************************************************
// Typedef

typedef enum
{
    CELL_OFF = 0U,
    CELL_ON  = 1U,
} ScreenCell;

typedef struct
{
    uint8_t V0; uint8_t V1; uint8_t V2; uint8_t V3;    
    uint8_t V4; uint8_t V5; uint8_t V6; uint8_t V7;
    uint8_t V8; uint8_t V9; uint8_t VA; uint8_t VB;
    uint8_t VC; uint8_t VD; uint8_t VE; uint8_t VF;
} Registers;

// ************************************************************
// Global variables

uint16_t PC = 0;
uint16_t I = 0;
uint16_t stack[STACK_SIZE] = {0};
uint8_t delayTimer = 0;
uint8_t soundTimer = 0;
Registers registers = {0};
uint8_t memory[MEMORY_SIZE/8] = {0}; // 4 KiB comprised of 8-bit numbers

uint8_t font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// ************************************************************

void FontInit(uint16_t startAddr, uint16_t length, uint8_t *data)
{
    // Probably worth adding some handling against writing outside of the memory
    uint16_t i;
    for (i = 0; i < length; i++)
    {
        memory[startAddr + length] = data[i];
    }
}

// ************************************************************

int main(void)
{
    printf("Hello World\n");
    printf("uchar: %d\n", sizeof(uint8_t));
    printf("ushor: %d\n", sizeof(uint16_t));
    printf("uint: %d\n", sizeof(unsigned int));
    printf("ulong: %d\n", sizeof(unsigned long));
    return 0;
}

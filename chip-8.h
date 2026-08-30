#ifndef CHIP8H
#define CHIP8H
// ************************************************************
// Libraries

#define SDL_MAIN_USE_CALLBACKS 1 // Use callbacks insted of main
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_stdinc.h>

// ************************************************************
// Defines

#define CPU_CLOCK_IN_MHZ 1
#define CPU_CLOCK_IN_MS ((1 / (CPU_CLOCK_IN_MHZ * 1000000))) * 1000 // TODO: Could be reworked to be better

#define SCREEN_REFRESH_RATE_IN_MS 16.6667f // 60Hz ((1/60) * 1000)
#define PIXEL_SIZE 24 // Size of the individual pixels of the chip-8 screen
#define SCREEN_WIDTH_IN_PX  64
#define SCREEN_HEIGHT_IN_PX 32
#define SCREEN_MATRIX_SIZE (SCREEN_HEIGHT_IN_PX * SCREEN_WIDTH_IN_PX)
#define SCREEN_MAX_PX_STATES 2 // On / Off

#define SDL_WINDOW_WIDTH (SCREEN_WIDTH_IN_PX * PIXEL_SIZE)
#define SDL_WINDOW_HEIGHT (SCREEN_HEIGHT_IN_PX * PIXEL_SIZE)

#define MEMORY_SIZE_IN_KIB 4
#define MEMORY_SIZE (MEMORY_SIZE_IN_KIB * 1024)
#define STACK_SIZE 16

#define NUM_REGS 16

#define NUM_PX_STATES 2

#define FONT_START_ADDR 0x050
#define ROM_START 0x200

#define ROM_DIRECTORY "./ROMS/"



// ************************************************************
// Enums

typedef enum
{
    PX_OFF = 0U,
    PX_ON  = 1U,
} ScreenPxState;

typedef enum 
{
    V0 = 0U,
    V1 = 1U,
    V2 = 2U,
    V3 = 3U,
    V4 = 4U,
    V5 = 5U,
    V6 = 6U,
    V7 = 7U,
    V8 = 8U,
    V9 = 9U,
    VA = 10U,
    VB = 11U,
    VC = 12U,
    VD = 13U,
    VE = 14U,
    VF = 15U,
} Reg;

// ************************************************************
// Typedef

typedef struct
{
    Uint16 PC;
    Uint16 I;
    Uint16 SP; // Stack pointer
    Uint16 stack[STACK_SIZE];
    Uint8 delayTimer;
    Uint8 soundTimer;
    Uint8 registers[NUM_REGS];
    Uint8 memory [MEMORY_SIZE];
    Uint8 screen[SCREEN_MATRIX_SIZE];
} MachineState;

typedef struct 
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    MachineState machineState;
    Uint64 lastTick;
} AppState;

// ************************************************************
// Global Variables

const Uint8 font[] = {
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
// Prototypes

void FontInit(MachineState *machine, Uint16 startAddr, Uint16 length, const Uint8 *data);
void MachineInit(MachineState *machine);
ScreenPxState GetPixelValue(const MachineState *machine, Uint32 x, Uint32 y);
void SetPixelPosition(SDL_FRect *r, Uint32 x, Uint32 y);

// ************************************************************
#endif
#define SDL_MAIN_USE_CALLBACKS 1 // Use callbacks insted of main
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_stdinc.h>

// ************************************************************
// Defines

#define STEP_RATE_IN_MS 16.6667f // 60Hz ((1/60) * 1000)
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
// Typedef

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
} Regs;

// ************************************************************
// Global variables

// Uint16 PC = 0;
// Uint16 I = 0;
// Uint16 stack[STACK_SIZE] = {0};
// Uint8 delayTimer = 0;
// Uint8 soundTimer = 0;
// Uint8 registers[NUM_REGS] = {0};
// Uint8 memory[MEMORY_SIZE/8] = {0}; // 4 KiB comprised of 8-bit numbers

Uint8 font[] = {
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

typedef struct
{
    Uint16 PC;
    Uint16 I;
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

void FontInit(MachineState *machine, Uint16 startAddr, Uint16 length, Uint8 *data)
{
    // Probably worth adding some handling against writing outside of the memory
    Uint16 i;
    
    for (i = 0; i < length; i++)
    {
        machine->memory[startAddr + i] = data[i];
    }
}

// Initialize the emulator
void MachineInit(MachineState *machine)
{
    SDL_zerop(machine);
    machine->PC = ROM_START;
    FontInit(machine, FONT_START_ADDR, sizeof(font), font);
}

// Get value of specific pixel on the screen
ScreenPxState GetPixelValue(const MachineState *machine, Uint16 x, Uint16 y)
{
    Uint16 location = (y * SCREEN_WIDTH_IN_PX) + x;
    return (ScreenPxState)(machine->screen[location] & 1);
}

// Set the position of the pixel-to-be-drawn
void SetPixelPosition(SDL_FRect *r, Uint16 x, Uint16 y)
{
    r->x = (float)(x * PIXEL_SIZE);
    r->y = (float)(y * PIXEL_SIZE);
}

// ************************************************************


// ************************************************************
// SDL Callbacks

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    SDL_AppResult result = SDL_APP_CONTINUE;

    // TODO Metadata

    // Inittialize the SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) && (result == SDL_APP_CONTINUE))
    {
        SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
        result = SDL_APP_FAILURE;
    }

    // Allocate the appstate struct
    AppState *state = (AppState *)SDL_calloc(1, sizeof(AppState));
    if ((!state) && (result == SDL_APP_CONTINUE))
    {
        result = SDL_APP_FAILURE;
    }
    *appstate = state;

    // Create window
    if (!SDL_CreateWindowAndRenderer("CHIP-8", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer) 
        && (result == SDL_APP_CONTINUE))
    {
        result = SDL_APP_FAILURE;
    }
    // To enable scaling of the window while keeping the desired resolution
    SDL_SetRenderLogicalPresentation(state->renderer, SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Initialize the emulator
    MachineInit(&state->machineState);

    // TODO: Move to function?
    // TODO: Get filename from the arguments
    // char romName[] = "/home/jacob/Code/not-a-good-chip-8-emulator/chip8-test-suite/bin/2-ibm-logo.ch8";
    char *romName = "2-ibm-logo.ch8";
    char *romDir = ROM_DIRECTORY;
    size_t pathSize = SDL_strlen(romName) + SDL_strlen(romDir) + 1; // +1 for NULL terminator
    char path[pathSize];
    SDL_zeroa(path);
    SDL_strlcat(path, romDir, pathSize);
    pathSize = SDL_strlcat(path, romName, pathSize);
    // SDL_Log("%s\n", path);
    // SDL_Log("%d\n", pathSize);
    
    // Load File using SDL
    // TODO: Maybe check that the ROM fits into the memory
    size_t fileBufferSize;
    Uint8 *fileBuffer = (Uint8 *)SDL_LoadFile(path, &fileBufferSize);
    if (!fileBuffer && (result == SDL_APP_CONTINUE))
    {
        SDL_Log("Failed to load ROM file: %s", SDL_GetError());
        result = SDL_APP_FAILURE;
    }
    else // Load the ROM into the memory
    {
        SDL_memcpy(&state->machineState.memory[ROM_START], fileBuffer, fileBufferSize);
    }
    // Clear the file buffer
    SDL_free(fileBuffer);

    // Tics
    state->lastTick = SDL_GetTicks();

    return result;
}

SDL_AppResult SDL_AppIterate(void *appsate)
{
    SDL_AppResult result = SDL_APP_CONTINUE;

    AppState *state = (AppState *)appsate;
    MachineState *machine = &state->machineState;
    const Uint64 now = SDL_GetTicks();
    
    SDL_FRect r;
    Uint32 x, y;
    ScreenPxState pxState;

    // run game logic if we're at or past the time to run it.
    // if we're _really_ behind the time to run it, run it
    // several times.
    while ((now - state->lastTick) >= STEP_RATE_IN_MS)
    {
        // TODO emulator step
        state->lastTick += STEP_RATE_IN_MS; // To run it multiple times we're reaally behind
    }

    // == Draw the screen == 
    r.w = r.h = PIXEL_SIZE;
    // Set the drawing color to black and clear the screen
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state->renderer);

    for (y = 0; y < SCREEN_HEIGHT_IN_PX; y++)
    {
        for (x = 0; x < SCREEN_WIDTH_IN_PX; x++)
        {
            pxState = GetPixelValue(machine, x, y);
            if (pxState == PX_ON) // Draw white rectangle where ON pixel should be
            {
                SetPixelPosition(&r, x, y);
                SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(state->renderer, &r);
            }
        }
    }

    SDL_RenderPresent(state->renderer);
    return result;
}

SDL_AppResult SDL_AppEvent(void *appsate, SDL_Event *event)
{
    SDL_AppResult result = SDL_APP_CONTINUE;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            result = SDL_APP_SUCCESS;
            break;

        default:
            break;
    }

    return result;
}

void SDL_AppQuit(void *appsate, SDL_AppResult result)
{
    // Clear appsate
    if (appsate != NULL)
    {
        AppState *state = (AppState *)appsate;
        SDL_DestroyRenderer(state->renderer);
        SDL_DestroyWindow(state->window);
        SDL_free(state);
    }
}
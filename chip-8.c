#ifndef CHIP8C
#define CHIP8C

#include "chip-8.h"

// ************************************************************

void FontInit(MachineState *machine, Uint16 startAddr, Uint16 length, const Uint8 *data)
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
ScreenPxState GetPixelValue(const MachineState *machine, Uint32 x, Uint32 y)
{
    Uint32 location = (y * SCREEN_WIDTH_IN_PX) + x;
    return (ScreenPxState)(machine->screen[location] & 1);
}

// Set the position of the pixel-to-be-drawn
void SetPixelPosition(SDL_FRect *r, Uint32 x, Uint32 y)
{
    r->x = (float)(x * PIXEL_SIZE);
    r->y = (float)(y * PIXEL_SIZE);
}

void RefreshScreen(AppState *appstate)
{
    MachineState *machine = &appstate->machineState;
    SDL_FRect r;
    Uint32 x, y;
    ScreenPxState pxState;

    r.w = r.h = PIXEL_SIZE;
    // Set the drawing color to black and clear the screen
    SDL_SetRenderDrawColor(appstate->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(appstate->renderer);

    for (y = 0; y < SCREEN_HEIGHT_IN_PX; y++)
    {
        for (x = 0; x < SCREEN_WIDTH_IN_PX; x++)
        {
            pxState = GetPixelValue(machine, x, y);
            if (pxState == PX_ON) // Draw white rectangle where ON pixel should be
            {
                SetPixelPosition(&r, x, y);
                SDL_SetRenderDrawColor(appstate->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(appstate->renderer, &r);
            }
        }
    }

    SDL_RenderPresent(appstate->renderer);
}

// ************************************************************
// Fetch, Decode, Execute

Uint16 FetchInstruction(MachineState *machine)
{
    Uint16 opCode;
    opCode = ((Uint16)machine->memory[machine->PC] << 8) | (Uint16)machine->memory[(machine->PC + 1)];
    machine->PC += 2;
    SDL_Log("%x\n", opCode);
    return opCode;
}

void OP_DXYN(MachineState *machine, Uint16 x, Uint16 y, Uint16 n)
{
    Uint8 vx, vy, colData, px, flipped;
    Uint16 col, row, screenIndex;

    // Location can wrap
    // Sprites should clip
    vx = (machine->registers[x] % SCREEN_WIDTH_IN_PX);
    vy = (machine->registers[y] % SCREEN_HEIGHT_IN_PX);

    machine->registers[VF] = 0;

    for (row = 0; row < n; row++)
    {
        if ((row + vy) >= SCREEN_HEIGHT_IN_PX)
        {
            break; // Clip the sprite
        }

        colData = machine->memory[(machine->I + row)];

        for (col = 0; (col < 8); col++)
        {
            if ((col + vx) >= SCREEN_WIDTH_IN_PX)
            {
                break; // Clip the sprite
            }

            screenIndex = ((row + vy) * SCREEN_WIDTH_IN_PX) + (col + vx);
            px = (colData >> (7 - col)) & 0x01U;

            if (px)
            {
                if (machine->screen[screenIndex] == PX_ON)
                {
                    machine->registers[VF] = 1;
                }
                machine->screen[screenIndex] ^= PX_ON;
            }
        }
    }
    return;
}
/*
- [ ] 0NNN -> Skip
- [x] 00E0
- [x] 00EE
- [x] 1NNN
- [x] 2NNN
- [x] 3XNN
- [x] 4XNN
- [x] 5XY0
- [x] 6XNN
- [x] 7XNN
- [ ] 8XY0
- [ ] 8XY1
- [ ] 8XY2
- [ ] 8XY3
- [ ] 8XY4
- [ ] 8XY5
- [ ] 8XY6
- [ ] 8XY7
- [ ] 8XYE
- [x] 9XY0
- [x] ANNN
- [x] BNNN -> Is ambiguous and can have different behavior based on emulator
- [ ] CXNN
- [x] DXYN
- [ ] EX9E
- [ ] EXA1
- [ ] FX07
- [ ] FX0A
- [ ] FX15
- [ ] FX18
- [ ] FX1E
- [ ] FX29
- [ ] FX33
- [ ] FX55
- [ ] FX65
*/
// This will be mess...
void DecodeExecute (MachineState *machine, Uint16 opcode)
{
    Uint16 nibble, nnn, nn, n, x, y;
    nibble = (opcode & 0xF000U) >> 12U;
    nnn = (opcode & 0x0FFFU);
    nn  = (opcode & 0x00FFU);
    n   = (opcode & 0x000FU);
    x   = (opcode & 0x0F00U) >> 8U;
    y   = (opcode & 0x00F0U) >> 4U;

    switch (nibble)
    {
        case 0x0:
            switch (nnn)
            {
                case 0x0E0: // Clear screeen
                    SDL_zeroa(machine->screen);
                    break;
                case 0x0EE: 
                    machine->SP--;
                    machine->PC = machine->SP;
                    break;
                default: // 0x0NNN instruction skipped
                    break;
            }
            break;
        case 0x1: // Jump
            machine->PC = nnn;
            break;
        case 0x2: // Call subroutine
            machine->stack[machine->SP] = machine->PC;
            machine->SP++;
            machine->PC = nnn;
            break;
        case 0x3: // Skip if VX == NN
            if (machine->registers[x] == (Uint8)nn)
            {
                machine->PC += 2;
            }
            break;
        case 0x4: // Skip if VX != NN
            if (machine->registers[x] != (Uint8)nn)
            {
                machine->PC += 2;
            }
            break;
        case 0x5: // Skip if VX == VY
            if (machine->registers[x] == machine->registers[y])
            {
                machine->PC += 2;
            }
            break;
        case 0x6: // Set VX to NN
            machine->registers[x] = nn;
            break;
        case 0x7: // Add NN to VX (without carry)
            machine->registers[x] += nn;
            break;
        case 0x9: // Skip if VX != VY
            if (machine->registers[x] != machine->registers[y])
            {
                machine->PC += 2;
            }
            break;
        case 0xA: // Set I to the address NNN
            machine->I = nnn;
            break;
        case 0xB: // Jump to NNN + V0
            machine->PC = nnn + machine->registers[V0];
            break;
        case 0xD: // Draw on screen
            OP_DXYN(machine, x, y, n);
            break;
        default:
            break;
    }
}

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
    // char *romName = "2-ibm-logo.ch8";
    char *romName = "1-chip8-logo.ch8";
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
    
    // run game logic if we're at or past the time to run it.
    // if we're _really_ behind the time to run it, run it
    // several times.
    while ((now - state->lastTick) >= CPU_CLOCK_IN_MHZ)
    {
        Uint16 opCode = FetchInstruction(machine);
        DecodeExecute(machine, opCode);
        state->lastTick += CPU_CLOCK_IN_MHZ; // To run it multiple times we're reaally behind
        RefreshScreen(state); // TODO: Should be capped at 60Hz I think
    }


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

#endif
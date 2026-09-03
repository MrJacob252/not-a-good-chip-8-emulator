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

void OP_FX0A(MachineState *machine, Uint16 x)
{
    Uint16 k;
    static Uint8 keyPressedIndex = UINT8_MAX;
    bool keyPressed = false;

    // Wait for key to be pressed
    if (keyPressedIndex == UINT8_MAX)
    {
        for (k = 0; k < KEYPAD_SIZE; k++)
        {
            if (machine->keypad[k] == KEY_PRESSED)
            {
                keyPressedIndex = k;
                break;
            }
        }
    }

    // Wait for the pressed key to be released
    if (keyPressedIndex != UINT8_MAX)
    {
        if (machine->keypad[keyPressedIndex] == KEY_NOT_PRESSED)
        {
            machine->registers[x] = (keyPressedIndex & 0xF);
            keyPressedIndex = UINT8_MAX;
            keyPressed = true;
        }
    }

    if (!keyPressed)
    {
        machine->PC -= 2;
    }

    return;
}

SDL_AppResult LoadROM (MachineState *machine, int argc, char **argv)
{
    char *romName;
    char *romDir = ROM_DIRECTORY;
    size_t pathSize, fileBufferSize;
    Uint8 *fileBuffer;
    
    SDL_AppResult result = SDL_APP_CONTINUE;

    if (argc != 2)
    {
        // -1 becaue there will be alway 1 argument (the program path)
        SDL_Log("Exactly one command line argument should be give!\nCurrent value: %d\n", (argc - 1));
        result = SDL_APP_FAILURE;
    }

    if (result == SDL_APP_CONTINUE)
    {
        romName = argv[1];

        size_t pathSize = SDL_strlen(romName) + SDL_strlen(romDir) + 1; // +1 for NULL terminator
        char path[pathSize];
        SDL_zeroa(path);
        // Combine the paths
        SDL_strlcat(path, romDir, pathSize); // Add dir to the path
        pathSize = SDL_strlcat(path, romName, pathSize); // Add ROM name to the path

        fileBuffer = SDL_LoadFile(path, &fileBufferSize);
        if (!fileBuffer)
        {
            SDL_Log("Failed to load ROM file: %s", SDL_GetError());
            result = SDL_APP_FAILURE;
        }
        else
        {
            // Load the ROM into memory
            SDL_memcpy(&machine->memory[ROM_START], fileBuffer, fileBufferSize);
        }

        SDL_free(fileBuffer);
    }

    return result;
}
/*
- [ ] [ ] 0NNN -> Skip
- [x] [ ] 00E0
- [x] [ ] 00EE
- [x] [x] 1NNN
- [x] [ ] 2NNN
- [x] [ ] 3XNN
- [x] [ ] 4XNN
- [x] [ ] 5XY0
- [x] [x] 6XNN
- [x] [ ] 7XNN
- [x] [ ] 8XY0
- [x] [ ] 8XY1
- [x] [ ] 8XY2
- [x] [ ] 8XY3
- [x] [ ] 8XY4
- [x] [ ] 8XY5
- [x] [ ] 8XY6
- [x] [ ] 8XY7
- [x] [ ] 8XYE
- [x] [ ] 9XY0
- [x] [x] ANNN
- [x] [ ] BNNN -> Is ambiguous and can have different behavior based on emulator
- [x] [ ] CXNN
- [x] [ ] DXYN
- [x] [ ] EX9E
- [x] [ ] EXA1
- [x] [ ] FX07
- [x] [ ] FX0A
- [x] [ ] FX15
- [x] [ ] FX18
- [x] [ ] FX1E
- [x] [ ] FX29
- [x] [ ] FX33
- [x] [x] FX55
- [x] [x] FX65
*/
// This will be mess...
void DecodeExecute (MachineState *machine, Uint16 opcode)
{
    Uint16 nibble, nnn, nn, n, x, y;
    Uint8 value, oldValue, carry, key;
    Sint32 rnd;
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
                    machine->PC = machine->stack[machine->SP];
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
        case 0x8:
            switch (n)
            {
                case 0x0: // Set VX to value of VY
                    machine->registers[x] = machine->registers[y];
                    break;
                case 0x1: // VX = VX or VY
                    machine->registers[x] |= machine->registers[y];
                    break;
                case 0x2: // VX = VX and VY
                    machine->registers[x] &= machine->registers[y];
                    break;
                case 0x3: // VX = VX xor VY
                    machine->registers[x] ^= machine->registers[y];
                    break;
                case 0x4: // VX = VX + VY with carry
                    oldValue = machine->registers[x];
                    machine->registers[x] += machine->registers[y];
                    machine->registers[VF] = (machine->registers[x] < oldValue) ? 1 : 0;
                    break;
                case 0x5: // VX = VX - VY with carry
                    carry = (machine->registers[x] >= machine->registers[y]);
                    machine->registers[x] -= machine->registers[y];
                    machine->registers[VF] = carry;
                    break;
                case 0x6: // VX >> 1 with carry
                    carry = machine->registers[x] & 0x1;
                    machine->registers[x] >>= 1;
                    machine->registers[VF] = carry;
                    break;
                case 0x7: // VX = VY - VX with carry
                    carry = (machine->registers[y] >= machine->registers[x]);
                    machine->registers[x] = machine->registers[y] - machine->registers[x];
                    machine->registers[VF] = carry;
                    break;
                case 0xE: // VX << 1 with carry
                    carry = (machine->registers[x] & 0x80) >> 7;
                    machine->registers[x] <<= 1;
                    machine->registers[VF] = carry;
                    break;
                default:
                    break;
            }
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
        case 0xC: // Generate random number
            rnd = SDL_rand(UINT8_MAX);
            machine->registers[x] = ((Uint8)rnd) & nn;
            break;
        case 0xD: // Draw on screen
            OP_DXYN(machine, x, y, n);
            break;
        case 0xE:
            switch (nn)
            {
                case 0x9E: // Skip if pressed
                    key = (machine->registers[x] & 0xFU);
                    if (machine->keypad[key] == KEY_PRESSED)
                    {
                        machine->PC += 2;
                    }
                    break;
                case 0xA1: // Skip if not pressed
                    key = (machine->registers[x] & 0xFU);
                    if (machine->keypad[key] == KEY_NOT_PRESSED)
                    {
                        machine->PC += 2;
                    }
                    break;
            }
            break;
        case 0xF:
            switch (nn)
            {
                case 0x07: // Set VX to the value of delay timer
                    machine->registers[x] = machine->delayTimer;
                    break;
                case 0x0A: 
                        OP_FX0A(machine, x);
                    break;
                case 0x15: // Set delay timer to VX
                    machine->delayTimer = machine->registers[x];
                    break;
                case 0x18: // Set sound timer to VX
                    machine->soundTimer = machine->registers[x];
                    break;
                case 0x1E: // I = VX + I, no carry
                    machine->I += machine->registers[x];
                    break;
                case 0x29: // Set the I to address of font character in the lowest nibble of VX
                    // Take the index of the character from the lowest nibble and multiply by 5
                    // because each character is represented by 5 bytes in memory
                    machine->I = FONT_START_ADDR + (5 * (machine->registers[x] & 0xF));
                    break;
                case 0x33:
                    value = machine->registers[x];
                    machine->memory[(machine->I)]     = (value  / 100); 
                    machine->memory[(machine->I + 1)] = (value %= 100) / 10; 
                    machine->memory[(machine->I + 2)] = (value  / 10); 
                    break;
                case 0x55: // Reg dump
                    SDL_memcpy(&machine->memory[machine->I], machine->registers, (x + 1));
                    break;
                case 0x65: // Reg load
                    SDL_memcpy(machine->registers, &machine->memory[machine->I], (x + 1));
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return;
}

SDL_AppResult HandleKeyEventDown(MachineState *machine, SDL_Scancode keyCode, SDL_EventType eventType)
{
    SDL_AppResult result = SDL_APP_CONTINUE;

    /*
    |     Key value = Mapped value  |
    | ----- | ----- | ----- | ----- | 
    | 1 = 1 | 2 = 2 | 3 = 3 | C = 4 |
    | 4 = Q | 5 = W | 6 = E | D = R |
    | 7 = A | 8 = S | 9 = D | E = F |
    | A = Z | 0 = X | B = C | F = V |
    */

    Uint8 keyValue;
    if (eventType == SDL_EVENT_KEY_DOWN)
    {
        keyValue = KEY_PRESSED;
    }
    else
    {
        keyValue = KEY_NOT_PRESSED;
    }
    
    switch (keyCode)
    {
        // Quit
        case SDL_SCANCODE_ESCAPE:
            result = SDL_APP_SUCCESS;
            break; 
        // Handle the keyboard input
        case SDL_SCANCODE_1:
            machine->keypad[0x1U] = keyValue;
            break;
        case SDL_SCANCODE_2:
            machine->keypad[0x2U] = keyValue;
            break;
        case SDL_SCANCODE_3:
            machine->keypad[0x3U] = keyValue;
            break;
        case SDL_SCANCODE_4:
            machine->keypad[0xCU] = keyValue;
            break;
        case SDL_SCANCODE_Q:
            machine->keypad[0x4U] = keyValue;
            break;
        case SDL_SCANCODE_W:
            machine->keypad[0x5U] = keyValue;
            break;
        case SDL_SCANCODE_E:
            machine->keypad[0x6U] = keyValue;
            break;
        case SDL_SCANCODE_R:
            machine->keypad[0xDU] = keyValue;
            break;
        case SDL_SCANCODE_A:
            machine->keypad[0x7U] = keyValue;
            break;
        case SDL_SCANCODE_S:
            machine->keypad[0x8U] = keyValue;
            break;
        case SDL_SCANCODE_D:
            machine->keypad[0x9U] = keyValue;
            break;
        case SDL_SCANCODE_F:
            machine->keypad[0xEU] = keyValue;
            break;
        case SDL_SCANCODE_Z:
            machine->keypad[0xAU] = keyValue;
            break;
        case SDL_SCANCODE_X:
            machine->keypad[0x0U] = keyValue;
            break;
        case SDL_SCANCODE_C:
            machine->keypad[0xBU] = keyValue;
            break;
        case SDL_SCANCODE_V:
            machine->keypad[0xFU] = keyValue;
            break;
        default:
            break;
    }

    return result;
}

void GenerateMoreAudio(AudioState *audioState)
{
    Uint32 n_samples = 256;
    Uint32 i;
    float samples[n_samples];
    float time;
    for (i = 0; i < n_samples; i++)
    {
        time = (float)audioState->phaseIndex / SAMPLE_RATE;
        samples[i] = SDL_sinf(2.0f * SDL_PI_F * TONE_FREQUENCY * time);
        audioState->phaseIndex++;
    }
    
    // Push samples to the queue
    SDL_PutAudioStreamData(audioState->stream, samples, sizeof(samples));

    return;
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

    // Initialize the audio
    SDL_AudioSpec audioSpec = {
        .format = SDL_AUDIO_F32,
        .channels = 1, // Mono
        .freq = SAMPLE_RATE,
    };

    state->audioState.stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &audioSpec,
        NULL,
        NULL
    );

    if ((!state->audioState.stream) && result == SDL_APP_CONTINUE)
    {
        SDL_Log("Couldn't initialize SDL audio stream: %s\n", SDL_GetError());
        result = SDL_APP_FAILURE;
    }

    if (result == SDL_APP_CONTINUE)
    {
        // Initialize the random seed
        Uint64 seed;
        SDL_srand(seed);
        
        // Initialize the emulator
        MachineInit(&state->machineState);
    }

    result = LoadROM(&state->machineState, argc, argv);

    // Tics
    if (result == SDL_APP_CONTINUE)
    {
        state->lastTick = SDL_GetTicks();
    }

    return result;
}

SDL_AppResult SDL_AppIterate(void *appsate)
{
    SDL_AppResult result = SDL_APP_CONTINUE;

    AppState *state = (AppState *)appsate;
    MachineState *machine = &state->machineState;
    AudioState *audioState = &state->audioState;
    const Uint64 now = SDL_GetTicks();
    
    // run game logic if we're at or past the time to run it.
    // if we're _really_ behind the time to run it, run it
    // several times.
    // while ((now - state->lastTick) >= CPU_CLOCK_IN_MS)
    if ((now - state->lastTick) >= CPU_CLOCK_IN_MS)
    {
        Uint16 opCode = FetchInstruction(machine);
        DecodeExecute(machine, opCode);
        
        // Timers (TODO: Update at the 60Hz rate or not?)
        // -- Hopefully this does it??
        if ((machine->delayTimer > 0) && ((now - state->lastTickDelay) >= DELAY_REFRESH_RATE_IN_MS))
        {
            machine->delayTimer--;
        }
        if ((machine->soundTimer > 0) && ((now - state->lastTickDelay) >= DELAY_REFRESH_RATE_IN_MS))
        {
            machine->soundTimer--;
            
            // Add more samples to the queue
            if (SDL_GetAudioStreamQueued(audioState->stream) < MIN_QUEUED_BYTES)
            {
                GenerateMoreAudio(audioState);
            }
            if (!audioState->running)
            {
                // Unpause the audio to begin playing
                SDL_ResumeAudioStreamDevice(audioState->stream);
                audioState->running = true;
            }
        }
        else
        {
            SDL_ClearAudioStream(audioState->stream);
            audioState->running = false;
        }
        
        // state->lastTick += CPU_CLOCK_IN_MS; // To run it multiple times we're reaally behind
        RefreshScreen(state);
    }

    return result;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = (AppState *)appstate;
    MachineState *machine = &state->machineState;
    SDL_AppResult result = SDL_APP_CONTINUE;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            result = SDL_APP_SUCCESS;
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            result = HandleKeyEventDown(machine, event->key.scancode, event->key.type);
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
        SDL_DestroyAudioStream(state->audioState.stream);
        SDL_free(state);
    }
}

#endif
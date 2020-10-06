#include "game.h"

internal void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16_t ToneVolume = 3000; 
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples; 
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(GameState->tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
        if(GameState->tSine > 2.0f*Pi32)
        {
            GameState->tSine -= 2.0f*Pi32;
        }
    }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0; X < Buffer->Width; ++X)
        {
            uint8_t Blue = (uint8_t)(X + BlueOffset);
            uint8_t Green = (uint8_t)(Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void
RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY)
{
    uint32_t Colour = 0xFFFFFFFF;
    int Top = PlayerY;
    int Bottom = PlayerY+10;
    for(int X = PlayerX; X < PlayerX+10; ++X)
    {
        uint8_t *Pixel = ((uint8_t *)Buffer->Memory +
                          X*Buffer->BytesPerPixel +
                          Top*Buffer->Pitch);
        for(int Y = Top; Y < Bottom; ++Y);
        {
            *(uint32_t *)Pixel = Colour;
            Pixel += Buffer->Pitch;
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialised)
    {
        char *Filename = __FILE__;

        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents)
        {
            Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(File.Contents);
        }

        GameState->ToneHz = 512;
        GameState->tSine = 0.0f;

        GameState->PlayerX = 100;
        GameState->PlayerY = 100;

        // TODO: May be better to do in the platform layer
        Memory->IsInitialised = true;
    }

    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalogue)
        {
            // Use analogue movement tuning
            GameState->BlueOffset += (int)(4.0f*Controller->StickAverageX);
            GameState->ToneHz = 512 + (int)(128.0f*Controller->StickAverageY);

        }
        else
        {
            // Use digital movement tuning
            if(Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffset -= 1;
            }

            if(Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffset += 1;
            }
        }

        // Input.AButtonEndedDown
        // Input.AButtonHalfTransitionCount
        if(Controller->ActionDown.EndedDown)
        {
            GameState->GreenOffset += 1;
        }

        GameState->PlayerX += (int)(4.0f*Controller->StickAverageX);
        GameState->PlayerY -= (int)(4.0f*Controller->StickAverageX);
    }

    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
    RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

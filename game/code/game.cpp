#include "game.h"

internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    local_persist real32 tSine;
    int16_t ToneVolume = 3000; 
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples; 
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        real32 SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
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
            uint8_t Blue = (X + BlueOffset);
            uint8_t Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}
internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer,bgame_sound_output_buffer *SoundBuffer)
{
    local_persist int BlueOffset = 0;
    local_persist int GreenOffset = 0;
    local_persist int ToneHz = 240;

    // TBD: Allow sample pffsets here for more robust platform options
    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}

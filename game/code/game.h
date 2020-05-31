#if !defined(GAME_H)
/*
  Services that the platform layer provides to the game
*/

/*
  Services that the game provides to the platform layer
  (this may expand in the future - sound on seperate thread, etc)
*/

struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16_t *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset,
                                  game_sound_output_buffer *SoundBuffer, int ToneHz);

#define GAME_H
#endif

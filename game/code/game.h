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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset);

#define GAME_H
#endif

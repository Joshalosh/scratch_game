#if !defined(GAME_H)

/*
  GAME_INTERNAL:
    0 - Build for public release
    1 - Build for developer only

  GAME_SLOW:
    0 - No slow code allowed
    1 - Slow code
*/

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *) 0 = 0;}
#else
#define Assert(Expression)
#endif
    
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32_t
SafeTruncateUInt64(uint64_t Value)
{
    // TODO: Define maximum values i.e UInt32Max
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;
    return(Result);
}

/*
  Services that the platform layer provides to the game
*/
#if GAME_INTERNAL
/* IMPORTANT

   These are not for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data!
 */
struct debug_read_file_result
{
    uint32_t ContentsSize;
    void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory);
#endif

/*
  Services that the game provides to the platform layer
  (this may expand in the future - sound on seperate thread, etc)
*/

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use

// TODO: In the future, rendering _specifically_ will become a three-tiered abstraction
struct game_offscreen_buffer
{
    // Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
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

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalogue;
    real32 StickAverageX;
    real32 StickAverageY;

    union
    {
        game_button_state Buttons[10];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[5];
};
inline game_controller_input *GetController(game_input *Input, int ControllerIndex) 
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return(Result);
}

struct game_memory
{
    bool32 IsInitialised;
    
    uint64_t PermanentStorageSize;
    void *PermanentStorage; // Required to be cleared to zero at startup

    uint64_t TransientStorageSize;
    void *TransientStorage; // Required to be cleared to zero at startup
};

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer,
                                  game_sound_output_buffer *SoundBuffer);

struct game_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};

#define GAME_H
#endif

#if !defined(WIN32_GAME_H)
struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32_t RunningSampleIndex;
    int BytesPerSample;
    DWORD SecondaryBufferSize;
    DWORD SafetyBytes;

    // TODO: Perhaps RunningSampleIndex should be in bytes
    // TODO: Add "bytes per second" field to make math simpler
};

struct win32_debug_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;
    DWORD ExpectedFlipPlayCursor;
    
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

struct win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    // Either of the callbacks can be 0!
    // check before calling
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;
    debug_game_frame_end *DEBUGFrameEnd;

    bool32 IsValid;
};

enum win32_fader_state
{
    Win32Fade_FadingIn,
    Win32Fade_WaitingForShow,
    Win32Fade_Inactive,
    Win32Fade_FadingGame,
    Win32Fade_FadingOut,
    Win32Fade_WaitingForClose,
};
struct win32_fader 
{
    win32_fader_state State;
    HWND Window;
    r32 Alpha;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_replay_buffer
{
    HANDLE FileHandle;
    HANDLE MemoryMap;
    char Filename[WIN32_STATE_FILE_NAME_COUNT];
    void *MemoryBlock;
};

struct win32_state
{
    uint64_t TotalSize;
    void *GameMemoryBlock;
    win32_replay_buffer ReplayBuffers[4];

    HANDLE RecordingHandle;
    int InputRecordingIndex;

    HANDLE PlaybackHandle;
    int InputPlayingIndex;

    char EXEFilename[WIN32_STATE_FILE_NAME_COUNT];
    char *OnePastLastEXEFilenameSlash;
};

#define WIN32_GAME_H
#endif

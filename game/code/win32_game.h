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

enum win32_memory_block_flags
{
    Win32Memory_NotRestored = 0x1,
};
struct win32_memory_block
{
    win32_memory_block *Prev;
    win32_memory_block *Next;
    u64 Size;
    u64 Flags;
    u64 Pad[4];
};
inline void *GetBasePointer(win32_memory_block *Block)
{
    void *Result = Block + 1;
    return(Result);
};

struct win32_saved_memory_block
{
    u64 BasePointer;
    u64 Size;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{
    // To touch the memory ring, I need to take the memory mutex
    ticket_mutex MemoryMutex;
    win32_memory_block MemorySentinel;

    HANDLE RecordingHandle;
    int InputRecordingIndex;

    HANDLE PlaybackHandle;
    int InputPlayingIndex;

    char EXEFilename[WIN32_STATE_FILE_NAME_COUNT];
    char *OnePastLastEXEFilenameSlash;
};

struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
};

struct platform_work_queue
{
    uint32_t volatile CompletionGoal;
    uint32_t volatile CompletionCount;

    uint32_t volatile NextEntryToWrite;
    uint32_t volatile NextEntryToRead;
    HANDLE SemaphoreHandle;

    platform_work_queue_entry Entries[256];
};

struct win32_thread_startup
{
    platform_work_queue *Queue;
};

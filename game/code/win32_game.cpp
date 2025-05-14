/*
TODO: Additional Platform Layer Code
  - Saved game locations
  - Getting a handle to our own executable file
  - Raw Input (support for multiple keyboards)
  - ClipCursor() (for multimonitor support)
  - QueryCancelAutoplay
  - WM_ACTIVATEAPP (for when we are not the active application)
  - GetKeyboardLayout (for French keyboards, internation WASD support)
    etc...
*/

#include "game_platform.h"
#include "game_shared.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include <gl/gl.h>

#include "win32_game.h"

platform_api Platform;

global_variable win32_state GlobalWin32State;
global_variable b32 GlobalSoftwareRendering;
global_variable b32 GlobalRunning;
global_variable b32 GlobalPause;
global_variable b32 GlobalShowSortGroups;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable s64 GlobalPerfCountFrequency;
global_variable b32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};
global_variable GLuint GlobalBlitTextureHandle;

#define WGL_DRAW_TO_WINDOW_ARB           0x2001
#define WGL_ACCELERATION_ARB             0x2003
#define WGL_SUPPORT_OPENGL_ARB           0x2010
#define WGL_DOUBLE_BUFFER_ARB            0x2011
#define WGL_PIXEL_TYPE_ARB               0x2013

#define WGL_TYPE_RGBA_ARB                0x202B
#define WGL_FULL_ACCELERATION_ARB        0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext, const int *attribList);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_iv_arb(HDC hdc,
        int iPixelFormat,
        int iLayerPlane,
        UINT nAttributes,
        const int *piAttributes,
        int *piValues);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_fv_arb(HDC hdc,
        int iPixelFormat,
        int iLayerPlane,
        UINT nAttributes,
        const int *piAttributes,
        FLOAT *pfValues);

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc,
        const int *piAttribIList,
        const FLOAT *pfAttribFList,
        UINT nMaxFormats,
        int *piFormats,
        UINT *nNumFormats);

typedef void WINAPI gl_bind_framebuffer(GLenum target, GLuint framebuffer);
typedef void WINAPI gl_gen_framebuffers(GLsizei n, GLuint *framebuffers);
typedef void WINAPI gl_framebuffer_texture_2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum WINAPI gl_check_framebuffer_status(GLenum target);

global_variable gl_bind_framebuffer *glBindFramebuffer;
global_variable gl_gen_framebuffers *glGenFramebuffers;
global_variable gl_framebuffer_texture_2D *glFramebufferTexture2D;
global_variable gl_check_framebuffer_status *glCheckFramebufferStatus;

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
typedef const char * WINAPI wgl_get_extensions_string_ext(void);

global_variable wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
global_variable wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
global_variable wgl_swap_interval_ext *wglSwapIntervalEXT;
global_variable wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;
global_variable b32 OpenGLSupportsSRGBFramebuffer;
global_variable GLuint OpenGLDefaultInternalTextureFormat;
global_variable GLuint OpenGLReservedBlitTexture;

#include "game_sort.cpp"
#include "game_render.h"
#include "game_opengl.cpp"
#include "game_render.cpp"

// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    // TODO: Dest bounds checking.
    for(int Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(int Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

internal void
Win32GetEXEFilename(win32_state *State)
{
    // Never use MAX_PATH in user-facing code, because it
    // can be dangerous and lead to bad results.
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFilename, sizeof(State->EXEFilename));
    State->OnePastLastEXEFilenameSlash = State->EXEFilename;
    for(char *Scan = State->EXEFilename; *Scan; ++Scan)
    {
        if(*Scan == '\\')
        {
            State->OnePastLastEXEFilenameSlash = Scan + 1;
        }
    }
}

internal void
Win32BuildEXEPathFilename(win32_state *State, char *Filename,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFilenameSlash - State->EXEFilename, State->EXEFilename,
               StringLength(Filename), Filename,
               DestCount, Dest);
}

#if GAME_INTERNAL
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32_t FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // File read successfully
                    Result.ContentsSize = FileSize32;
                }
                else
                {

                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: Logging
    }

    return(Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // File read successfully
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            // TODO: Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO: Logging
    }

    return(Result);
}

DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
{
    debug_executing_process Result = {};

    STARTUPINFO StartupInfo = {};
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION ProcessInfo = {};
    if(CreateProcess(Command,
                     CommandLine,
                     0,
                     0,
                     FALSE,
                     0,
                     0,
                     Path,
                     &StartupInfo,
                     &ProcessInfo))
    {
        Assert(sizeof(Result.OSHandle) >= sizeof(ProcessInfo.hProcess));
        *(HANDLE *)&Result.OSHandle = ProcessInfo.hProcess;
    }
    else
    {
        DWORD ErrorCode = GetLastError();
        *(HANDLE *)&Result.OSHandle = INVALID_HANDLE_VALUE;
    }

    return(Result);
}

DEBUG_PLATFORM_GET_PROCESS_STATE(DEBUGGetProcessState)
{
    debug_process_state Result = {};

    HANDLE hProcess = *(HANDLE *)&Process.OSHandle;
    if(hProcess != INVALID_HANDLE_VALUE)
    {
        Result.StartedSuccessfully = true;

        if(WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0)
        {
            DWORD ReturnCode = 0;
            GetExitCodeProcess(hProcess, &ReturnCode);
            Result.ReturnCode = ReturnCode;
            CloseHandle(hProcess);
        }
        else
        {
            Result.IsRunning = true;
        }
    }

    return(Result);
}
#endif

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return(LastWriteTime);
}

inline b32
Win32TimeIsValid(FILETIME Time)
{
    b32 Result = (Time.dwLowDateTime != 0) || (Time.dwHighDateTime != 0);
    return(Result);
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFilename)
{
    win32_game_code Result = {};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(!GetFileAttributesEx(LockFilename, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, FALSE);

        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if(Result.GameCodeDLL)
        {
            Result.UpdateAndRender = (game_update_and_render *)
                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.GetSoundSamples = (game_get_sound_samples *)
                GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

            Result.DEBUGFrameEnd = (debug_game_frame_end *)
                GetProcAddress(Result.GameCodeDLL, "DEBUGGameFrameEnd");

            Result.IsValid = (Result.UpdateAndRender &&
                              Result.GetSoundSamples &&
                              Result.DEBUGFrameEnd);
        }
    }

    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
        Result.DEBUGFrameEnd = 0;
    }

    return(Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;
}

internal void
Win32LoadXInput(void)
{
    // TODO: Test this on Windows 8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        // TODO: Diagnostic
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }

    if(!XInputLibrary)
    {
        // TODO: Diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}

        // TODO: Diagnostic

    }
    else
    {
        // TODO: Diagnostic
    }
}

internal void
Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
    // Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if(DSoundLibrary)
    {
        // Get a DirectionalSound object
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        // TODO: Double check this works on XP, DirectSound8 or 7?
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // "Create" a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if(SUCCEEDED(Error))
                    {
                        // The format is now set
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: Diagnostics
                    }
                }
                else
                {
                    // TODO: Diagnostics
                }
            }
            else
            {
                // TODO: Diagnostic
            }

            // TODO: In release mode, should I specify DSBCAPS_GLOBALFOCUS?

            // TODO: DSBCAPS_GETCURRENTPOSTIION2?
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
#if GAME_INTERNAL
            BufferDescription.dwFlags |= DSBCAPS_GLOBALFOCUS;
#endif
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
            if(SUCCEEDED(Error))
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
        }
        else
        {
            // TODO: Diagnostic
        }
    }
    else
    {
        // TODO: Diagnostic
    }
}

int Win32OpenGLAttribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE: Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if GAME_INTERNAL
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};

internal void
Win32SetPixelFormat(HDC WindowDC)
{
    int SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;
    if(wglChoosePixelFormatARB)
    {
        int IntAttribList[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
#if GAME_STREAMING
            WGL_DOUBLE_BUFFER_ARB, GL_FALSE,
#else
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
#endif
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0,
        };

        if(!OpenGLSupportsSRGBFramebuffer)
        {
            IntAttribList[10] = 0;
        }

        wglChoosePixelFormatARB(WindowDC, IntAttribList, 0, 1,
                                &SuggestedPixelFormatIndex, &ExtendedPick);
    }

    if(!ExtendedPick)
    {
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
#if GAME_STREAMING
        // NOTE: PFD_DOUBLEBUFFER appears to prevent OBS from reliably streaming the window
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW;
#else
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
#endif
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }

    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    // NOTE: Technically I don't need to call DescribePixelFormat here, as 
    // SetPixelFormat doesn't actually need it to be filled out properly
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

internal void
Win32LoadWGLExtensions()
{
    WNDCLASSA WindowClass = {};

    WindowClass.lpfnWndProc = DefWindowProcA;
    WindowClass.hInstance = GetModuleHandle(0);
    WindowClass.lpszClassName = "GameWGLLoader";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Game",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            WindowClass.hInstance,
            0);

        HDC WindowDC = GetDC(Window);
        Win32SetPixelFormat(WindowDC);
        HGLRC OpenGLRC = wglCreateContext(WindowDC);
        if(wglMakeCurrent(WindowDC, OpenGLRC))
        {
            wglChoosePixelFormatARB =
                (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB =
                (wgl_create_context_attribs_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
            wglSwapIntervalEXT = (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)wglGetProcAddress("wglGetExtensionsStringEXT");

            if(wglGetExtensionsStringEXT)
            {
                char *Extensions = (char *)wglGetExtensionsStringEXT();
                char *At = Extensions;
                while(*At)
                {
                    while(IsWhitespace(*At)) {++At;}
                    char *End = At;
                    while(*End && !IsWhitespace(*End)) {++End;}

                    umm Count = End - At;

                    if(0) {}
                    else if(StringsAreEqual(Count, At, "WGL_EXT_framebuffer_sRGB")) {OpenGLSupportsSRGBFramebuffer = true;}
                    else if(StringsAreEqual(Count, At, "WGL_ARB_framebuffer_sRGB")) {OpenGLSupportsSRGBFramebuffer = true;}

                    At = End;
                }
            }

            wglMakeCurrent(0, 0);
        }

        wglDeleteContext(OpenGLRC);
        ReleaseDC(Window, WindowDC);
        DestroyWindow(Window);
    }
}

internal HGLRC
Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();

    Win32SetPixelFormat(WindowDC);

    b32 ModernContext = true;
    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB)
    {
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }

    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC = wglCreateContext(WindowDC);
    }

    if(wglMakeCurrent(WindowDC, OpenGLRC))
    {
        opengl_info Info = OpenGLInit(ModernContext, OpenGLSupportsSRGBFramebuffer);
        if(Info.GL_ARB_framebuffer_object)
        {
            glBindFramebuffer = (gl_bind_framebuffer *)wglGetProcAddress("glBindFramebuffer");
            glGenFramebuffers = (gl_gen_framebuffers *)wglGetProcAddress("glGenFramebuffers");
            glFramebufferTexture2D = (gl_framebuffer_texture_2D *)wglGetProcAddress("glFramebufferTexture2D");
            glCheckFramebufferStatus = (gl_check_framebuffer_status *)wglGetProcAddress("glCheckFramebufferStatus");
        }

        if(wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(1);
        }

        glGenTextures(1, &OpenGLReservedBlitTexture);
    }

    return(OpenGLRC);
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO: Bulletproof this
    // Maybe don't free first, free after, then free first if that fails

    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    // When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom up, meaning that
    // the first three bytes of the image are the colour for the top left pixel
    // in the bitmap, not the bottom left
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    Buffer->Pitch = Align16(Width*BytesPerPixel);
    int BitmapMemorySize = (Buffer->Pitch*Buffer->Height);
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    // VirtualAlloc should _only_ have given us back zero'd memory which is all black 
    // so we don't need to clear it
}

internal void
Win32DisplayBufferInWindow(platform_work_queue *RenderQueue, game_render_commands *Commands,
                           HDC DeviceContext, rectangle2i DrawRegion, u32 WindowWidth, u32 WindowHeight,
                           memory_arena *TempArena)
{
    temporary_memory TempMem = BeginTemporaryMemory(TempArena);

    game_render_prep Prep = PrepForRender(Commands, TempArena);

    /* TODO: Do I want to check for resources like before? Probably... yes
    if(AllResourcesPresent(RenderGroup))
    {
        RenderToOutput(TranState->HighPriorityQueue, RenderGroup, &DrawBuffer, &TranState->TranArena);
    }
    */

    if(GlobalSoftwareRendering)
    {
        loaded_bitmap OutputTarget;
        OutputTarget.Memory = GlobalBackbuffer.Memory;
        OutputTarget.Width = GlobalBackbuffer.Width;
        OutputTarget.Height = GlobalBackbuffer.Height;
        OutputTarget.Pitch = GlobalBackbuffer.Pitch;

        SoftwareRenderCommands(RenderQueue, Commands, &Prep, &OutputTarget, TempArena);

        OpenGLDisplayBitmap(GlobalBackbuffer.Width, GlobalBackbuffer.Height, GlobalBackbuffer.Memory,
                            GlobalBackbuffer.Pitch, DrawRegion, Commands->ClearColor,
                            OpenGLReservedBlitTexture);
        SwapBuffers(DeviceContext);
    }
    else
    {
        BEGIN_BLOCK("OpenGLRenderCommands");
        OpenGLRenderCommands(Commands, &Prep, DrawRegion, WindowWidth, WindowHeight);
        END_BLOCK();

        BEGIN_BLOCK("SwapBuffers");
        SwapBuffers(DeviceContext);
        END_BLOCK();
    }

    EndTemporaryMemory(TempMem);
}

internal LRESULT CALLBACK
Win32FadeWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        {
        } break;

        case WM_SETCURSOR:
        {
            SetCursor(0);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        {
            // TODO: Handle this with a message to the user
            GlobalRunning = false;
        } break;

        case WM_WINDOWPOSCHANGING:
        {
            if(GetKeyState(VK_SHIFT) & 0x8000)
            {
                WINDOWPOS *NewPos = (WINDOWPOS *)LParam;

                RECT WindowRect;
                RECT ClientRect;
                GetWindowRect(Window, &WindowRect);
                GetClientRect(Window, &ClientRect);

                s32 ClientWidth = (ClientRect.right - ClientRect.left);
                s32 ClientHeight = (ClientRect.bottom - ClientRect.top);
                s32 WidthAdd = ((WindowRect.right - WindowRect.left) - ClientWidth);
                s32 HeightAdd = ((WindowRect.bottom - WindowRect.top) - ClientHeight);

                s32 RenderWidth = GlobalBackbuffer.Width;
                s32 RenderHeight = GlobalBackbuffer.Height;

                s32 SugX = NewPos->cx;
                s32 SugY = NewPos->cy;

                s32 NewCx = (RenderWidth * (NewPos->cy - HeightAdd)) / RenderHeight;
                s32 NewCy = (RenderHeight * (NewPos->cx - WidthAdd)) / RenderWidth;

                if(AbsoluteValue((r32)(NewPos->cx - NewCx)) <
                   AbsoluteValue((r32)(NewPos->cy - NewCy)))
                {
                    NewPos->cx = NewCx + WidthAdd;
                }
                else
                {
                    NewPos->cy = NewCy + HeightAdd;
                }
            }

            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;

        case WM_SETCURSOR:
        {
            if(DEBUGGlobalShowCursor)
            {
                Result = DefWindowProcA(Window, Message, WParam, LParam);
            }
            else
            {
                SetCursor(0);
            }
        } break;

        case WM_ACTIVATEAPP:
        {
#if 0
            if(WParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
#endif
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle this as an error - recreate window
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
#if 0
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);
#endif
            EndPaint(Window, &Paint);
        } break;

        default:
        {
//          OutputDebugStringA("default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size, 0)))
    {
        uint8_t *DestSample = (uint8_t *)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        DestSample = (uint8_t *)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
                     game_sound_output_buffer *SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size, 0)))
    {
        // TODO: assert that the Region1Size/Region2Size is valid

        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16_t *DestSample = (int16_t *)Region1;
        int16_t *SourceSample = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16_t *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state *OldState, DWORD ButtonBit,
                                game_button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;

    if(Value < -DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if(Value > DeadZoneThreshold)
    {
        Result = (real32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }

    return(Result);
}

internal void
Win32GetInputFileLocation(win32_state *State, bool32 InputStream,
                          int SlotIndex, int DestCount, char *Dest)
{
    char Temp[64];
    wsprintf(Temp, "loop_edit_%d_%s.gi", SlotIndex, InputStream ? "input" : "state");
    Win32BuildEXEPathFilename(State, Temp, DestCount, Dest);
}

internal
DEBUG_PLATFORM_GET_MEMORY_STATS(Win32GetMemoryStats)
{
    debug_platform_memory_stats Stats = {};
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    for(win32_memory_block *SourceBlock = Sentinel->Next; 
        SourceBlock != Sentinel; 
        SourceBlock = SourceBlock->Next)
    {
        Assert(SourceBlock->Block.Size <= U32Maximum);

        ++Stats.BlockCount;
        Stats.TotalSize += SourceBlock->Block.Size;
        Stats.TotalUsed += SourceBlock->Block.Used;
    }
    EndTicketMutex(&GlobalWin32State.MemoryMutex);

    return(Stats);
}

internal void 
Win32VerifyMemoryListIntegrity()
{
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    local_persist u32 FailCounter;
    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    for(win32_memory_block *SourceBlock = Sentinel->Next;
            SourceBlock != Sentinel;
            SourceBlock = SourceBlock->Next)
    {
        Assert(SourceBlock->Block.Size <= U32Maximum);
    }
    ++FailCounter;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
    char Filename[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
    State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(State->RecordingHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;

        State->InputRecordingIndex = InputRecordingIndex;
        win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;

        BeginTicketMutex(&GlobalWin32State.MemoryMutex);
        for(win32_memory_block *SourceBlock = Sentinel->Next;
            SourceBlock != Sentinel;
            SourceBlock = SourceBlock->Next)
        {
            if(!(SourceBlock->Block.Flags & PlatformMemory_NotRestored))
            {
                win32_saved_memory_block DestBlock;
                void *BasePointer = SourceBlock->Block.Base;
                DestBlock.BasePointer = (u64)BasePointer;
                DestBlock.Size = SourceBlock->Block.Size;
                WriteFile(State->RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
                Assert(DestBlock.Size <= U32Maximum);
                WriteFile(State->RecordingHandle, BasePointer, (u32)DestBlock.Size, &BytesWritten, 0);
            }
        }
        EndTicketMutex(&GlobalWin32State.MemoryMutex);

        win32_saved_memory_block DestBlock = {};
        WriteFile(State->RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
    }
}

internal void
Win32EndRecordingInput(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

internal void
Win32FreeMemoryBlock(win32_memory_block *Block)
{
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Block->Prev->Next = Block->Next;
    Block->Next->Prev = Block->Prev;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);

    BOOL Result = VirtualFree(Block, 0, MEM_RELEASE);
    Assert(Result);
}

internal void
Win32ClearBlocksByMask(win32_state *State, u64 Mask)
{
    for(win32_memory_block *BlockIter = State->MemorySentinel.Next; BlockIter != &State->MemorySentinel;)
    {
        win32_memory_block *Block = BlockIter;
        BlockIter = BlockIter->Next;

        if((Block->LoopingFlags & Mask) == Mask)
        {
            Win32FreeMemoryBlock(Block);
        }
        else
        {
            Block->LoopingFlags = 0;
        }
    }
}

internal void
Win32BeginInputPlayback(win32_state *State, int InputPlayingIndex)
{
    Win32ClearBlocksByMask(State, Win32Mem_AllocatedDuringLooping);

    char Filename[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(Filename), Filename);
    State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if(State->PlaybackHandle != INVALID_HANDLE_VALUE)
    {
        State->InputPlayingIndex = InputPlayingIndex;

        for(;;)
        {
            win32_saved_memory_block Block = {};
            DWORD BytesRead;
            ReadFile(State->PlaybackHandle, &Block, sizeof(Block), &BytesRead, 0);
            if(Block.BasePointer != 0)
            {
                void *BasePointer = (void *)Block.BasePointer;
                Assert(Block.Size <= U32Maximum);
                DWORD BytesRead;
                ReadFile(State->PlaybackHandle, BasePointer, (u32)Block.Size, &BytesRead, 0);
            }
            else
            {
                break;
            }
        }
        // TODO: Stream memory in from the file
    }
}

internal void
Win32EndInputPlayback(win32_state *State)
{
    Win32ClearBlocksByMask(State, Win32Mem_FreedDuringLooping);
    CloseHandle(State->PlaybackHandle);
    State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlaybackInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesRead = 0;
    if(ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
    {
        if(BytesRead == 0)
        {
            // Hit the end of stream, go back to the beginning.
            int PlayingIndex = State->InputPlayingIndex;
            Win32EndInputPlayback(State);
            Win32BeginInputPlayback(State, PlayingIndex);
            ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
        }
    }
}

internal void
ToggleFullScreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
    MSG Message;
    for(;;)
    {
        BOOL GotMessage = FALSE;

        {
            TIMED_BLOCK("PeekMessage");
            GotMessage = PeekMessage(&Message, 0, 0, 0, PM_REMOVE);
        }

        if(!GotMessage)
        {
            break;
        }

        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32_t VKCode = (uint32_t)Message.wParam;

                bool32 AltKeyWasDown = (Message.lParam & (1 << 29));

                // Comparing WasDown to IsDown needs to use
                // == and != to convert these but tests to 
                // actual 0 or 1 values.
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
#if GAME_INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if(VKCode == 'L')
                    {
                        if(IsDown)
                        {
                            if(AltKeyWasDown)
                            {
                                Win32BeginInputPlayback(State, 1);
                            }
                            else
                            {
                                if(State->InputPlayingIndex == 0)
                                {
                                    if(State->InputRecordingIndex == 0)
                                    {
                                        Win32BeginRecordingInput(State, 1);
                                    }
                                    else
                                    {
                                        Win32EndRecordingInput(State);
                                        Win32BeginInputPlayback(State, 1);
                                    }
                                }
                                else
                                {
                                    Win32EndInputPlayback(State);
                                }
                            }
                        }
                    }
#endif
                    if(IsDown)
                    {
                        if((VKCode == VK_F4) && AltKeyWasDown)
                        {
                            GlobalRunning = false;
                        }
                        if((VKCode == VK_RETURN) && AltKeyWasDown)
                        {
                            if(Message.hwnd)
                            {
                                ToggleFullScreen(Message.hwnd);
                            }
                        }
                    }
                }

            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                     (real32)GlobalPerfCountFrequency);
    return(Result);
}

internal void
Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
{
    // TODO: Switch to InterlockedCompareExchange eventually so that any thread can add
    uint32_t NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal bool32
Win32DoNextWorkQueueEntry(platform_work_queue *Queue)
{
    bool32 WeShouldSleep = false;

    uint32_t OriginalNextEntryToRead = Queue->NextEntryToRead;
    uint32_t NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
    {
        uint32_t Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                                    NewNextEntryToRead,
                                                    OriginalNextEntryToRead);
        if(Index == OriginalNextEntryToRead)
        {
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    }
    else
    {
        WeShouldSleep = true;
    }

    return(WeShouldSleep);
}

internal void
Win32CompleteAllWork(platform_work_queue *Queue)
{
    while(Queue->CompletionGoal != Queue->CompletionCount)
    {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    win32_thread_startup *Thread = (win32_thread_startup *)lpParameter;
    platform_work_queue *Queue = Thread->Queue;

    u32 TestThreadID = GetThreadID();
    Assert(TestThreadID == GetCurrentThreadId());

    for(;;)
    {
        if(Win32DoNextWorkQueueEntry(Queue))
        {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }

//    return(0);
}

internal PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork)
{
    char Buffer[256];
    wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char *)Data);
    OutputDebugStringA(Buffer);
}

internal void
Win32MakeQueue(platform_work_queue *Queue, uint32_t ThreadCount, win32_thread_startup *Startups)
{
    Queue->CompletionGoal = 0;
    Queue->CompletionCount = 0;

    Queue->NextEntryToWrite = 0;
    Queue->NextEntryToRead = 0;

    uint32_t InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);

    for(uint32_t ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
    {
        win32_thread_startup *Startup = Startups + ThreadIndex;
        Startup->Queue = Queue;

        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Startup, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}

struct win32_platform_file_handle
{
    HANDLE Win32Handle;
};

struct win32_platform_file_group
{
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;
};

internal PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
    platform_file_group Result = {};

    // TODO: Someday make an actual arena used by Win32.
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(
        0, sizeof(win32_platform_file_group),
        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Result.Platform = Win32FileGroup;

    wchar_t *WildCard = L"*.*";
    switch(Type)
    {
        case PlatformFileType_AssetFile:
        {
            WildCard = L"*.ga";
        } break;

        case PlatformFileType_SavedGameFile:
        {
            WildCard = L"*.gs";
        } break;

        InvalidDefaultCase;
    }

    Result.FileCount = 0;

    WIN32_FIND_DATAW FindData;
    HANDLE FindHandle = FindFirstFileW(WildCard, &FindData);
    while(FindHandle != INVALID_HANDLE_VALUE)
    {
        ++Result.FileCount;

        if(!FindNextFileW(FindHandle, &FindData))
        {
            break;
        }
    }
    FindClose(FindHandle);

    Win32FileGroup->FindHandle = FindFirstFileW(WildCard, &Win32FileGroup->FindData);

    return(Result);
}

internal PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup->Platform;
    if(Win32FileGroup)
    {
        FindClose(Win32FileGroup->FindHandle);

        VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
    }
}

internal PLATFORM_OPEN_FILE(Win32OpenNextFile)
{
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)FileGroup->Platform;
    platform_file_handle Result = {};

    if(Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
    {
        win32_platform_file_handle *Win32Handle = (win32_platform_file_handle *)VirtualAlloc(
            0, sizeof(win32_platform_file_handle),
            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        Result.Platform = Win32Handle;

        if(Win32Handle)
        {
            wchar_t *Filename = Win32FileGroup->FindData.cFileName;
            Win32Handle->Win32Handle = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            Result.NoErrors = (Win32Handle->Win32Handle != INVALID_HANDLE_VALUE);
        }

        if(!FindNextFileW(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
        {
            FindClose(Win32FileGroup->FindHandle);
            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
        }
    }

    return(Result);
}

internal PLATFORM_FILE_ERROR(Win32FileError)
{
#if GAME_INTERNAL
    OutputDebugString("WIN32 FILE ERROR: ");
    OutputDebugString(Message);
    OutputDebugString("\n");
#endif

    Handle->NoErrors = false;
}

internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
    if(PlatformNoFileErrors(Source))
    {
        win32_platform_file_handle *Handle = (win32_platform_file_handle *)Source->Platform;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);

        u32 FileSize32 = SafeTruncateUInt64(Size);

        DWORD BytesRead;
        if(ReadFile(Handle->Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped) &&
           (FileSize32 == BytesRead))
        {
            // File read succeeded.
        }
        else
        {
            Win32FileError(Source, "Read file failed.");
        }
    }
}

/*

internal PLATFORM_FILE_ERROR(Win32CloseFile)
{
    CloseHandle(FileHandle);
}

*/

inline b32
Win32IsInLoop(win32_state *State)
{
    b32 Result = ((State->InputRecordingIndex != 0) ||
                  (State->InputPlayingIndex));
    return(Result);
}

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    // I require memory block headers not to change the cache
    // line alignment of an allocation
    Assert(sizeof(win32_memory_block) == 64);

    umm PageSize = 4096; // TODO: Get this from the system maybe? 
    umm TotalSize = Size + sizeof(win32_memory_block);
    umm BaseOffset = sizeof(win32_memory_block);
    umm ProtectOffset = 0;
    if(Flags & PlatformMemory_UnderflowCheck)
    {
        TotalSize = Size + 2*PageSize;
        BaseOffset = 2*PageSize;
        ProtectOffset = PageSize;
    }
    else if(Flags & PlatformMemory_OverflowCheck)
    {
        umm SizeRoundedUp = AlignPow2(Size, PageSize);
        TotalSize = SizeRoundedUp + 2*PageSize;
        BaseOffset = PageSize + SizeRoundedUp - Size;
        ProtectOffset = PageSize + SizeRoundedUp;
    }

    win32_memory_block *Block = (win32_memory_block *) VirtualAlloc(0, TotalSize,
                                                                    MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Block);
    Block->Block.Base = (u8 *)Block + BaseOffset;
    Assert(Block->Block.Used == 0);
    Assert(Block->Block.ArenaPrev == 0);

    if(Flags & (PlatformMemory_UnderflowCheck|PlatformMemory_OverflowCheck))
    {
        DWORD OldProtect = 0;
        BOOL Protected = VirtualProtect((u8 *)Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
        Assert(Protected);
    }

    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    Block->Next = Sentinel;
    Block->Block.Size = Size;
    Block->Block.Flags = Flags;
    Block->LoopingFlags = Win32IsInLoop(&GlobalWin32State) ? Win32Mem_AllocatedDuringLooping : 0;

    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Block->Prev = Sentinel->Prev;
    Block->Prev->Next = Block;
    Block->Next->Prev = Block;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);

    platform_memory_block *PlatBlock = &Block->Block;
    return(PlatBlock);
}

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    if(Block)
    {
        win32_memory_block *Win32Block = ((win32_memory_block *)Block);
        if(Win32IsInLoop(&GlobalWin32State))
        {
            Win32Block->LoopingFlags = Win32Mem_FreedDuringLooping;
        }
        else
        {
            Win32FreeMemoryBlock(Win32Block);
        }
    }
}

#if GAME_INTERNAL
global_variable debug_table GlobalDebugTable_;
debug_table *GlobalDebugTable = &GlobalDebugTable_;
#endif

internal void
Win32FullRestart(char *SourceEXE, char *DestEXE, char *DeleteEXE)
{
    DeleteFile(DeleteEXE);
    if(MoveFile(DestEXE, DeleteEXE))
    {
        if(MoveFile(SourceEXE, DestEXE))
        {
            STARTUPINFO StartupInfo = {};
            StartupInfo.cb = sizeof(StartupInfo);
            PROCESS_INFORMATION ProcessInfo = {};
            if(CreateProcess(DestEXE,
                             GetCommandLine(),
                             0,
                             0,
                             FALSE,
                             0,
                             0,
                             "w:\\game\\data\\",
                             &StartupInfo,
                             &ProcessInfo))
            {
                CloseHandle(ProcessInfo.hProcess);
            }
            else
            {
                // TODO: Error!
            }

            ExitProcess(0);
        }
    }
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    DEBUGSetEventRecording(true);

    win32_state *State = &GlobalWin32State;
    State->MemorySentinel.Prev = &State->MemorySentinel;
    State->MemorySentinel.Next = &State->MemorySentinel;

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    Win32GetEXEFilename(State);

    char Win32EXEFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(State, "win32_game.exe",
                              sizeof(Win32EXEFullPath), Win32EXEFullPath);

    char TempWin32EXEFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(State, "win32_game_temp.exe",
                              sizeof(TempWin32EXEFullPath), TempWin32EXEFullPath);

    char DeleteWin32EXEFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(State, "win32_game_old.exe",
                              sizeof(DeleteWin32EXEFullPath), DeleteWin32EXEFullPath);

    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(State, "game.dll",
                              sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);
    
    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(State, "game_temp.dll",
                              sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(State, "lock.tmp",
                              sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

    // Set the windows scheduler granularity to 1ms
    // so that sleep can be more granular
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadXInput();

#if GAME_INTERNAL
    DEBUGGlobalShowCursor = true;
#endif
    WNDCLASSA WindowClass = {};

//    Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);
    Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);
//    Win32ResizeDIBSection(&GlobalBackbuffer, 1279, 719);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
//    WindowClass.hIcon;
    WindowClass.lpszClassName = "GameWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0, // WS_EX_TOPMOST|WS_EX_LAYERED,
                WindowClass.lpszClassName,
                "Game",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(Window)
        {
            // NOTE: I've toggled this off as I think my large monitor makes large window sizes fuck up
            //ToggleFullScreen(Window); 
            HDC OpenGLDC = GetDC(Window);
            HGLRC OpenGLRC = 0;

            OpenGLRC = Win32InitOpenGL(OpenGLDC);

            win32_thread_startup HighPriStartups[6] = {};
            platform_work_queue HighPriorityQueue = {};
            Win32MakeQueue(&HighPriorityQueue, ArrayCount(HighPriStartups), HighPriStartups);

            win32_thread_startup LowPriStartups[2] = {};
            platform_work_queue LowPriorityQueue = {};
            Win32MakeQueue(&LowPriorityQueue, ArrayCount(LowPriStartups), LowPriStartups);

            win32_sound_output SoundOutput = {};

            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            real32 GameUpdateHz = (real32)(MonitorRefreshHz);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16_t)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            // TODO: Actually compute this variance and see
            // what the lowest reasonable value is.
            SoundOutput.SafetyBytes = (int)(((real32)SoundOutput.SamplesPerSecond *
                                      (real32)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;

            // TODO: I want to make this a growable arena
            memory_arena FrameTempArena = {};

            // TODO: Need to figure out what the pushbuffer size is.
            u32 PushBufferSize = Megabytes(64);
            platform_memory_block *PushBufferBlock = Win32AllocateMemory(PushBufferSize, PlatformMemory_NotRestored);
            u8 *PushBuffer = PushBufferBlock->Base;

            // TODO: Probably remove MaxPossibleOverrun.
            u32 MaxPossibleOverrun = 2*8*sizeof(u16); 
            int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize + MaxPossibleOverrun,
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if GAME_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
#if GAME_INTERNAL
            GameMemory.DebugTable = GlobalDebugTable;
#endif
            GameMemory.HighPriorityQueue = &HighPriorityQueue;
            GameMemory.LowPriorityQueue = &LowPriorityQueue;
            GameMemory.PlatformAPI.AddEntry = Win32AddEntry;
            GameMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;

            GameMemory.PlatformAPI.GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
            GameMemory.PlatformAPI.GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
            GameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
            GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
            GameMemory.PlatformAPI.FileError = Win32FileError;

            GameMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
            GameMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;

#if GAME_INTERNAL
            GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;
            GameMemory.PlatformAPI.DEBUGExecuteSystemCommand = DEBUGExecuteSystemCommand;
            GameMemory.PlatformAPI.DEBUGGetProcessState = DEBUGGetProcessState;
            GameMemory.PlatformAPI.DEBUGGetMemoryStats = Win32GetMemoryStats;
#endif
            u32 TextureOpCount = 1024;
            platform_texture_op_queue *TextureOpQueue = &GameMemory.TextureOpQueue;
            TextureOpQueue->FirstFree = (texture_op *)VirtualAlloc(0, sizeof(texture_op)*TextureOpCount,
                                                                   MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            for(u32 TextureOpIndex = 0; TextureOpIndex < (TextureOpCount - 1); ++TextureOpIndex)
            {
                texture_op *Op = TextureOpQueue->FirstFree + TextureOpIndex;
                Op->Next = TextureOpQueue->FirstFree + TextureOpIndex + 1;
            }

            Platform = GameMemory.PlatformAPI;

            if(Samples)
            {
                // TODO: Monitor XBox controllers for being plugged in after the fact
                b32 XBoxControllerPresent[XUSER_MAX_COUNT] = {};
                for(u32 ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                {
                    XBoxControllerPresent[ControllerIndex] = true;
                }

                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[30] = {0};

                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0;
                bool32 SoundIsValid = false;

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                         TempGameCodeDLLFullPath,
                                                         GameCodeLockFullPath);
                DEBUGSetEventRecording(Game.IsValid);

                ShowWindow(Window, SW_SHOW);
                while(GlobalRunning)
                {
                    {DEBUG_DATA_BLOCK("Platform/Controls");
                        DEBUG_B32(GlobalPause);
                        DEBUG_B32(GlobalSoftwareRendering);
                        DEBUG_B32(GlobalShowSortGroups);
                    }

                    //
                    //
                    //

                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    
                    //
                    //
                    //

                    BEGIN_BLOCK("Input Processing");

                    game_render_commands RenderCommands = RenderCommandStruct(PushBufferSize, PushBuffer,
                                                                              GlobalBackbuffer.Width,
                                                                              GlobalBackbuffer.Height);
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    rectangle2i DrawRegion = AspectRatioFit(RenderCommands.Width, RenderCommands.Height,
                                                            Dimension.Width, Dimension.Height);

                    // TODO: Zeroing macro
                    // TODO: We can't zero everything because the up/down state will be
                    // wrong
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    {
                        TIMED_BLOCK("Win32 Message Processing");
                        Win32ProcessPendingMessages(State, NewKeyboardController);
                    }

                    if(!GlobalPause)
                    {
                        {
                            TIMED_BLOCK("Mouse Position");
                            POINT MouseP;
                            GetCursorPos(&MouseP);
                            ScreenToClient(Window, &MouseP);
                            r32 MouseX = (r32)MouseP.x;
                            r32 MouseY = (r32)((Dimension.Height - 1) - MouseP.y);

                            r32 MouseU = Clamp01MapToRange((r32)DrawRegion.MinX, MouseX, (r32)DrawRegion.MaxX);
                            r32 MouseV = Clamp01MapToRange((r32)DrawRegion.MinY, MouseY, (r32)DrawRegion.MaxY);

                            NewInput->MouseX = (r32)RenderCommands.Width*MouseU;
                            NewInput->MouseY = (r32)RenderCommands.Height*MouseV;

                            NewInput->MouseZ = 0; //TODO: Support mousewheel.
                            
                            NewInput->ShiftDown = (GetKeyState(VK_SHIFT) & (1 << 15));
                            NewInput->AltDown = (GetKeyState(VK_MENU) & (1 << 15));
                            NewInput->ControlDown = (GetKeyState(VK_CONTROL) & (1 << 15));
                        }

                        {
                            TIMED_BLOCK("Keyboard Processing");
                            DWORD WinButtonID[PlatformMouseButton_Count] =
                            {
                                VK_LBUTTON,
                                VK_MBUTTON,
                                VK_RBUTTON,
                                VK_XBUTTON1,
                                VK_XBUTTON2,
                            };
                            for(u32 ButtonIndex = 0; ButtonIndex < PlatformMouseButton_Count; ++ButtonIndex)
                            {
                                NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
                                NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                                Win32ProcessKeyboardMessage(&NewInput->MouseButtons[ButtonIndex],
                                                            GetKeyState(WinButtonID[ButtonIndex]) & (1 << 15));
                            }
                        }

                        {
                            TIMED_BLOCK("XBox Controllers");

                            // TODO: Need to not poll disconnected controllers to avoid
                            // xinput frame rate hit on older libraries
                            // TODO: Should I poll this more freqently?
                            DWORD MaxControllerCount = XUSER_MAX_COUNT;
                            if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                            {
                                MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                            }

                            for (DWORD ControllerIndex = 0;
                                 ControllerIndex < MaxControllerCount;
                                 ++ControllerIndex)
                            {
                                DWORD OurControllerIndex = ControllerIndex + 1;
                                game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                                game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                                XINPUT_STATE ControllerState;
                                if(XBoxControllerPresent[ControllerIndex] &&
                                   XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                                {
                                    NewController->IsConnected = true;
                                    NewController->IsAnalogue = OldController->IsAnalogue;

                                    // NOTE: This controller is plugged in
                                    // TODO: See if ControllerState.dwPacketNumber increments too rapidly
                                    XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                    NewController->StickAverageX = Win32ProcessXInputStickValue(
                                        Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                    NewController->StickAverageY = Win32ProcessXInputStickValue(
                                        Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                    if((NewController->StickAverageX != 0.0f) ||
                                       (NewController->StickAverageY != 0.0f))
                                    {
                                        NewController->IsAnalogue = true;
                                    }

                                    if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                    {
                                        NewController->StickAverageY = 1.0f;
                                        NewController->IsAnalogue = false;
                                    }

                                    if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                    {
                                        NewController->StickAverageY = -1.0f;
                                        NewController->IsAnalogue = false;
                                    }

                                    if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                    {
                                        NewController->StickAverageX = -1.0f;
                                        NewController->IsAnalogue = false;
                                    }

                                    if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                    {
                                        NewController->StickAverageX = 1.0f;
                                        NewController->IsAnalogue = false;
                                    }

                                    real32 Threshold = 0.5f;
                                    Win32ProcessXInputDigitalButton(
                                        (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                        &OldController->MoveLeft, 1,
                                        &NewController->MoveLeft);
                                    Win32ProcessXInputDigitalButton(
                                        (NewController->StickAverageX > Threshold) ? 1 : 0,
                                        &OldController->MoveRight, 1,
                                        &NewController->MoveRight);
                                    Win32ProcessXInputDigitalButton(
                                        (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                        &OldController->MoveDown, 1,
                                        &NewController->MoveDown);
                                    Win32ProcessXInputDigitalButton(
                                        (NewController->StickAverageY > Threshold) ? 1 : 0,
                                        &OldController->MoveUp, 1,
                                        &NewController->MoveUp);

                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                                                    &NewController->ActionDown);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ActionRight, XINPUT_GAMEPAD_B,
                                                                    &NewController->ActionRight);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                                                                    &NewController->ActionLeft);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                                                                    &NewController->ActionUp);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->LeftShoulder,
                                                                    XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                                    &NewController->LeftShoulder);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->RightShoulder,
                                                                    XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                                    &NewController->RightShoulder);

                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->Start, XINPUT_GAMEPAD_START,
                                                                    &NewController->Start);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->Back, XINPUT_GAMEPAD_BACK,
                                                                    &NewController->Back);
                                }
                                else
                                {
                                    // NOTE: This controller is not available
                                    NewController->IsConnected = false;
                                    XBoxControllerPresent[ControllerIndex] = false;
                                }
                            }
                        }
                    }
                    END_BLOCK();

                    //
                    //
                    //

                    BEGIN_BLOCK("Game Update");

                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width;
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Pitch = GlobalBackbuffer.Pitch;
                    if(!GlobalPause)
                    {
                        if(State->InputRecordingIndex)
                        {
                            Win32RecordInput(State, NewInput);
                        }

                        if(State->InputPlayingIndex)
                        {
                            game_input Temp = *NewInput;
                            Win32PlaybackInput(State, NewInput);
                            for(u32 MouseButtonIndex = 0;
                                MouseButtonIndex < PlatformMouseButton_Count;
                                ++MouseButtonIndex)
                            {
                                NewInput->MouseButtons[MouseButtonIndex] = Temp.MouseButtons[MouseButtonIndex];
                            }
                            NewInput->MouseX = Temp.MouseX;
                            NewInput->MouseY = Temp.MouseY;
                            NewInput->MouseZ = Temp.MouseZ;
                        }
                        if(Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&GameMemory, NewInput, &RenderCommands);
                            if(NewInput->QuitRequested)
                            {
                                GlobalRunning = false;
                            }
                        }
                    }

                    END_BLOCK();

                    //
                    //
                    //

                    BEGIN_BLOCK("Audio Update");

                    if(!GlobalPause)
                    {
                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                        {
                            /*
                               Sound output computation works like this:

                               Define a safety value that is the number
                               of samples the game update loop may vary 
                               by (up tp 2ms).

                               When woken up to write audio, look 
                               and see what the play cursor position is
                               and forecast ahead where the play cursor
                               will be on the next frame boundary.

                               Then look to see if the write cursor is
                               before that by at least the safety value.
                               If it is, the target fill position is that
                               frame boundary plus one frame. This gives
                               perfect audio sync in the case of a card that
                               has low latency.

                               If the write cursor is _after_ that safety margin,
                               then we assume we can never sync the audio
                               perfectly, so will then write one frame's
                               worth of audio plus the safety margin's worth
                               of guard samples.
                            */
                            if(!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }

                            DWORD ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                                                SoundOutput.SecondaryBufferSize);

                            DWORD ExpectedSoundBytesPerFrame = (int)((real32)(SoundOutput.SamplesPerSecond *
                                                               SoundOutput.BytesPerSample) / GameUpdateHz);
                            real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
                            DWORD ExpectedBytesUntilFlip =
                                (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame) *
                                (real32)ExpectedSoundBytesPerFrame);

                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

                            DWORD SafeWriteCursor = WriteCursor;
                            if(SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;

                            bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

                            DWORD TargetCursor = 0;
                            if(AudioCardIsLowLatency)
                            {
                                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                            }
                            else
                            {
                                TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame +
                                                SoundOutput.SafetyBytes);
                            }
                            TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);

                            DWORD BytesToWrite = 0;
                            if(ByteToLock > TargetCursor)
                            {
                                BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                                BytesToWrite += TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }

                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = Align8(BytesToWrite / SoundOutput.BytesPerSample);
                            BytesToWrite = SoundBuffer.SampleCount*SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            if(Game.GetSoundSamples)
                            {
                                Game.GetSoundSamples(&GameMemory, &SoundBuffer);
                            }

#if GAME_INTERNAL
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if(UnwrappedWriteCursor < PlayCursor)
                            {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                            AudioLatencySeconds =
                                (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) /
                                 (real32)SoundOutput.SamplesPerSecond);

#endif
                            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }
                    }

                    END_BLOCK();

                    //
                    //
                    //

#if GAME_INTERNAL
                    BEGIN_BLOCK("Debug Collation");

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    b32 ExecutableNeedsToBeReloaded =
                        (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0);

#if 0
                    FILETIME NewEXETime = Win32GetLastWriteTime(Win32EXEFullPath);
                    FILETIME OldEXETime = Win32GetLastWriteTime(TempWin32EXEFullPath);
                    b32 Win32NeedsToBeReloaded = (CompareFileTime(&NewEXETime, &OldEXETime) != 0);
                    if(Win32TimeIsValid(NewEXETime))
                    {
                        b32 Win32NeedsToBeReloaded =
                            (CompareFileTime(&NewEXETime, &OldEXETime) != 0);
                        // TODO: Compare file contents here
                        if(Win32NeedsToBeReloaded)
                        {
                            Win32FullRestart(TempWin32EXEFullPath, Win32EXEFullPath, DeleteWin32EXEFullPath);
                        }
                    }
#endif

                    GameMemory.ExecutableReloaded = false;
                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
                        DEBUGSetEventRecording(false);
                    }

                    if(Game.DEBUGFrameEnd)
                    {
                        Game.DEBUGFrameEnd(&GameMemory, NewInput, &RenderCommands);
                    }

                    if(ExecutableNeedsToBeReloaded)
                    {
                        Win32UnloadGameCode(&Game);
                        for(u32 LoadTryIndex = 0; !Game.IsValid && (LoadTryIndex < 100); ++LoadTryIndex)
                        {
                            Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                     TempGameCodeDLLFullPath,
                                                     GameCodeLockFullPath);
                            Sleep(100);
                        }

                        GameMemory.ExecutableReloaded = true;
                        DEBUGSetEventRecording(Game.IsValid);
                    }


                    END_BLOCK();
#endif

                    //
                    //
                    //

                    // TODO: Leave this off until I implement actual vblank support.
#if 0
                    BEGIN_BLOCK(FramerateWait);

                    if(!GlobalPause)
                    {
                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        // TODO: NOT TESTED YET! PROBABLY BUGGY!
                        real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if(SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                                   SecondsElapsedForFrame));
                                if(SleepMS > 0)
                                {
                                    Sleep(SleepMS);
                                }
                            }

                            real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                                       Win32GetWallClock());
                            if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                // TODO: Log missed sleep
                            }

                            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                                Win32GetWallClock());
                            }
                        }
                        else
                        {
                            // TODO: MISSED FRAME RATE!
                            // TODO: Logging
                        }
                    }

                    END_BLOCK(FramerateWait);
#endif

                    //
                    //
                    //

                    BEGIN_BLOCK("Frame Display");

                    BeginTicketMutex(&TextureOpQueue->Mutex);
                    texture_op *FirstTextureOp = TextureOpQueue->First;
                    texture_op *LastTextureOp = TextureOpQueue->Last;
                    TextureOpQueue->Last = TextureOpQueue->First = 0;
                    EndTicketMutex(&TextureOpQueue->Mutex);

                    if(FirstTextureOp)
                    {
                        Assert(LastTextureOp);
                        OpenGLManageTextures(FirstTextureOp);
                        BeginTicketMutex(&TextureOpQueue->Mutex);
                        LastTextureOp->Next = TextureOpQueue->FirstFree;
                        TextureOpQueue->FirstFree = FirstTextureOp;
                        EndTicketMutex(&TextureOpQueue->Mutex);
                    }

                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(&HighPriorityQueue, &RenderCommands, DeviceContext,
                                               DrawRegion, Dimension.Width, Dimension.Height, &FrameTempArena);
                    ReleaseDC(Window, DeviceContext);

                    FlipWallClock = Win32GetWallClock();

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    // TODO: Should I clear these here?

                    END_BLOCK();

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    FRAME_MARKER(Win32GetSecondsElapsed(LastCounter, EndCounter));
                    LastCounter = EndCounter;
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }

    return(0);
}

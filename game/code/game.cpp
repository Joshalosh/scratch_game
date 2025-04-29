#include "game.h"
#include "game_sort.cpp"
#include "game_render_group.cpp"
#include "game_asset.cpp"
#include "game_audio.cpp"
#include "game_world.cpp"
#include "game_particles.cpp"
#include "game_sim_region.cpp"
#include "game_brain.cpp"
#include "game_entity.cpp"
#include "game_world_mode.cpp"
#include "game_cutscene.cpp"

internal task_with_memory *
BeginTaskWithMemory(transient_state *TranState, b32 DependsOnGameMode)
{
    task_with_memory *FoundTask = 0;

    for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
    {
        task_with_memory *Task = TranState->Tasks + TaskIndex;
        if(!Task->BeingUsed)
        {
            FoundTask = Task;
            Task->BeingUsed = true;
            Task->DependsOnGameMode = DependsOnGameMode;
            Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
            break;
        }
    }

    return(FoundTask);
}

internal void
EndTaskWithMemory(task_with_memory *Task)
{
    EndTemporaryMemory(Task->MemoryFlush);

    CompletePreviousWritesBeforeFutureWrites;
    Task->BeingUsed = false;
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, int32_t Width, int32_t Height, bool32 ClearToZero = true)
{
    loaded_bitmap Result = {};

    Result.AlignPercentage = V2(0.5f, 0.5f);
    Result.WidthOverHeight = SafeRatio1((r32)Width, (r32)Height);

    Result.Width = Width;
    Result.Height = Height;
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
    int32_t TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
    Result.Memory = PushSize(Arena, TotalBitmapSize, Align(16, ClearToZero));

    return(Result);
}

internal void
MakeSphereNormalMap(loaded_bitmap *Bitmap, real32 Roughness, real32 Cx = 1.0f, real32 Cy = 1.0f)
{
    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);

    uint8_t *Row = (uint8_t *)Bitmap->Memory;
    for(int32_t Y = 0; Y < Bitmap->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int32_t X = 0; X < Bitmap->Width; ++X)
        {
            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

            real32 Nx = Cx*(2.0f*BitmapUV.x - 1.0f);
            real32 Ny = Cy*(2.0f*BitmapUV.y - 1.0f);

            real32 RootTerm = 1.0f - Nx*Nx - Ny*Ny;
            v3 Normal = {0, 0.707106781188f, 0.707106781188f};
            real32 Nz = 0.0f;
            if(RootTerm >= 0.0f)
            {
                Nz = SquareRoot(RootTerm);
                Normal = V3(Nx, Ny, Nz);
            }

            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
                        255.0f*(0.5f*(Normal.y + 1.0f)),
                        255.0f*(0.5f*(Normal.z + 1.0f)),
                        255.0f*Roughness};

            *Pixel++ = (((uint32_t)(Color.a + 0.5f) << 24) |
                        ((uint32_t)(Color.r + 0.5f) << 16) |
                        ((uint32_t)(Color.g + 0.5f) << 8) |
                        ((uint32_t)(Color.b + 0.5f) << 0));
        }

        Row += Bitmap->Pitch;
    }
}

internal void
MakeSphereDiffuseMap(loaded_bitmap *Bitmap, real32 Cx = 1.0f, real32 Cy = 1.0f)
{
    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);

    uint8_t *Row = (uint8_t *)Bitmap->Memory;
    for(int32_t Y = 0; Y < Bitmap->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int32_t X = 0; X < Bitmap->Width; ++X)
        {
            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

            real32 Nx = Cx*(2.0f*BitmapUV.x - 1.0f);
            real32 Ny = Cy*(2.0f*BitmapUV.y - 1.0f);

            real32 RootTerm = 1.0f - Nx*Nx - Ny*Ny;
            real32 Alpha = 0.0f;
            if(RootTerm >= 0.0f)
            {
                Alpha = 1.0f;
            }

            v3 BaseColor = {0.0f, 0.0f, 0.0f};
            Alpha *= 255.0f;
            v4 Color = {Alpha*BaseColor.x,
                        Alpha*BaseColor.y,
                        Alpha*BaseColor.z,
                        Alpha};

            *Pixel++ = (((uint32_t)(Color.a + 0.5f) << 24) |
                        ((uint32_t)(Color.r + 0.5f) << 16) |
                        ((uint32_t)(Color.g + 0.5f) << 8) |
                        ((uint32_t)(Color.b + 0.5f) << 0));
        }

        Row += Bitmap->Pitch;
    }
}

internal void
MakePyramidNormalMap(loaded_bitmap *Bitmap, real32 Roughness)
{
    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);

    uint8_t *Row = (uint8_t *)Bitmap->Memory;
    for(int32_t Y = 0; Y < Bitmap->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int32_t X = 0; X < Bitmap->Width; ++X)
        {
            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

            int32_t InvX = (Bitmap->Width - 1) - X;
            real32 Seven = 0.707106781188f;
            v3 Normal = {0, 0, Seven};
            if(X < Y)
            {
                if(InvX < Y)
                {
                    Normal.x = -Seven;
                }
                else
                {
                    Normal.y = Seven;
                }
            }
            else
            {
                if(InvX < Y)
                {
                    Normal.y = -Seven;
                }
                else
                {
                    Normal.x = Seven;
                }
            }

            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
                        255.0f*(0.5f*(Normal.y + 1.0f)),
                        255.0f*(0.5f*(Normal.z + 1.0f)),
                        255.0f*Roughness};

            *Pixel++ = (((uint32_t)(Color.a + 0.5f) << 24) |
                        ((uint32_t)(Color.r + 0.5f) << 16) |
                        ((uint32_t)(Color.g + 0.5f) << 8) |
                        ((uint32_t)(Color.b + 0.5f) << 0));
        }

        Row += Bitmap->Pitch;
    }
}

// TODO: Really need to get rid of main generation ID
internal u32
DEBUGGetMainGenerationID(game_memory *Memory)
{
    u32 Result = 0;

    transient_state *TranState = Memory->TransientState;
    if(TranState)
    {
        Result = TranState->MainGenerationID;
    }

    return(Result);
}

internal game_assets *
DEBUGGetGameAssets(game_memory *Memory)
{
    game_assets *Assets = 0;

    transient_state *TranState = Memory->TransientState;
    if(TranState)
    {
        Assets = TranState->Assets;
    }

    return(Assets);
}

internal void
SetGameMode(game_state *GameState, transient_state *TranState, game_mode GameMode)
{
    b32 NeedToWait = false;
    for(u32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
    {
        NeedToWait = NeedToWait || TranState->Tasks[TaskIndex].DependsOnGameMode;
    }
    if(NeedToWait)
    {
        Platform.CompleteAllWork(TranState->LowPriorityQueue);
    }
    Clear(&GameState->ModeArena);
    GameState->GameMode = GameMode;
}

#if GAME_INTERNAL
debug_table *GlobalDebugTable;
game_memory *DebugGlobalMemory;
#endif
platform_api Platform;
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Platform = Memory->PlatformAPI;

#if GAME_INTERNAL
    GlobalDebugTable = Memory->DebugTable;
    DebugGlobalMemory = Memory;

    {DEBUG_DATA_BLOCK("Renderer");
        DEBUG_B32(Global_Renderer_TestWeirdDrawBufferSize);
        {DEBUG_DATA_BLOCK("Camera");
            DEBUG_B32(Global_Renderer_Camera_UseDebug);
            DEBUG_VALUE(Global_Renderer_Camera_DebugDistance);
            DEBUG_B32(Global_Renderer_Camera_RoomBased);
        }
    }
    {DEBUG_DATA_BLOCK("AI/Familiar");
        DEBUG_B32(Global_AI_Familiar_FollowsHero);
    }
    {DEBUG_DATA_BLOCK("Particles");
        DEBUG_B32(Global_Particles_Test);
        DEBUG_B32(Global_Particles_ShowGrid);
    }
    {DEBUG_DATA_BLOCK("Simulation");
        DEBUG_VALUE(Global_Timestep_Percentage);
        DEBUG_B32(Global_Simulation_UseSpaceOutlines);
    }
    {DEBUG_DATA_BLOCK("Profile");
        DEBUG_UI_ELEMENT(DebugType_FrameSlider, FrameSlider);
        DEBUG_UI_ELEMENT(DebugType_LastFrameInfo, LastFrame);
        DEBUG_UI_ELEMENT(DebugType_DebugMemoryInfo, DebugMemory);
        DEBUG_UI_ELEMENT(DebugType_TopClocksList, GameUpdateAndRender);
    }

#endif
    TIMED_FUNCTION();

    Input->dtForFrame *= Global_Timestep_Percentage / 100.0f;

    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    game_state *GameState = Memory->GameState;
    if(!GameState)
    {
        GameState = Memory->GameState = BootstrapPushStruct(game_state, TotalArena);
        InitialiseAudioState(&GameState->AudioState, &GameState->AudioArena);
    }

    // Transient initialisation
    transient_state *TranState = Memory->TransientState;
    if(!TranState)
    {
        TranState = Memory->TransientState = BootstrapPushStruct(transient_state, TranArena);

        TranState->HighPriorityQueue = Memory->HighPriorityQueue;
        TranState->LowPriorityQueue = Memory->LowPriorityQueue;
        for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
        {
            task_with_memory *Task = TranState->Tasks + TaskIndex;
            Task->BeingUsed = false;
        }

        TranState->Assets = AllocateGameAssets(Megabytes(256), TranState, &Memory->TextureOpQueue);

//        GameState->Music = PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));

        GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
        GameState->TestNormal = MakeEmptyBitmap(&TranState->TranArena, GameState->TestDiffuse.Width,
                                                GameState->TestDiffuse.Height, false);
        MakeSphereNormalMap(&GameState->TestNormal, 0.0f);
        MakeSphereDiffuseMap(&GameState->TestDiffuse);
//          MakePyramidNormalMap(&GameState->TestNormal, 0.0f);

        TranState->EnvMapWidth = 512;
        TranState->EnvMapHeight = 256;
        for(uint32_t MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex)
        {
            environment_map *Map = TranState->EnvMaps + MapIndex;
            uint32_t Width = TranState->EnvMapWidth;
            uint32_t Height = TranState->EnvMapHeight;
            for(uint32_t LODIndex = 0; LODIndex < ArrayCount(Map->LOD); ++LODIndex)
            {
                Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, false);
                Width >>= 1;
                Height >>= 1;
            }
        }
    }

    {DEBUG_DATA_BLOCK("Memory");
        memory_arena *ModeArena = &GameState->ModeArena;
        DEBUG_VALUE(ModeArena);

        memory_arena *AudioArena = &GameState->AudioArena;
        DEBUG_VALUE(AudioArena);
        
        memory_arena *TranArena = &TranState->TranArena;
        DEBUG_VALUE(TranArena);
    }

    // TODO: I should probably pull the generation stuff, because
    // if I don't use ground chunks, it's a huge waste of effort
    if(TranState->MainGenerationID)
    {
        EndGeneration(TranState->Assets, TranState->MainGenerationID);
    }
    TranState->MainGenerationID = BeginGeneration(TranState->Assets);

    if(GameState->GameMode == GameMode_None)
    {
        PlayIntroCutscene(GameState, TranState);
#if 1
        // NOTE: This jumps straight into the game, no title sequence
        game_controller_input *Controller = GetController(Input, 0);
        Controller->Start.EndedDown = true;
        Controller->Start.HalfTransitionCount = 1;
#endif
    }

#if 0
    {
        v2 MusicVolume;
        MusicVolume.y = SafeRatio0((r32)Input->MouseX, (r32)Buffer->Width);
        MusicVolume.x = 1.0f - MusicVolume.y;
        ChangeVolume(&GameState->AudioState, GameState->Music, 0.01f, MusicVolume);
    }
#endif

    //
    // Render
    //
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);

    // TODO: Need to figure out what the pushbuffer size is.
    render_group RenderGroup_ = BeginRenderGroup(TranState->Assets, RenderCommands, TranState->MainGenerationID, false,
                                                 RenderCommands->Width, RenderCommands->Height);
    render_group *RenderGroup = &RenderGroup_;

    // TODO: Eliminate these entirely
    loaded_bitmap DrawBuffer = {};
    DrawBuffer.Width = RenderCommands->Width;
    DrawBuffer.Height = RenderCommands->Height;

    b32 Rerun = false;
    do
    {
        switch(GameState->GameMode)
        {
            case GameMode_TitleScreen:
            {
                Rerun = UpdateAndRenderTitleScreen(GameState, TranState, RenderGroup, &DrawBuffer,
                                                   Input, GameState->TitleScreen);
            } break;

            case GameMode_Cutscene:
            {
                Rerun = UpdateAndRenderCutscene(GameState, TranState, RenderGroup, &DrawBuffer,
                                                Input, GameState->Cutscene);
            } break;

            case GameMode_World:
            {
                Rerun = UpdateAndRenderWorld(GameState, GameState->WorldMode, TranState, Input, RenderGroup, &DrawBuffer);
            } break;

            InvalidDefaultCase;
        }
    } while(Rerun);

    EndRenderGroup(RenderGroup);

    EndTemporaryMemory(RenderMemory);

#if 0
    if(!HeroesExist && QuitRequested)
    {
        Memory->QuitRequested = true;
    }
#endif

    CheckArena(&GameState->ModeArena);
    CheckArena(&TranState->TranArena);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = Memory->GameState;
    transient_state *TranState = Memory->TransientState;

    OutputPlayingSounds(&GameState->AudioState, SoundBuffer, TranState->Assets, &TranState->TranArena);
}

#if GAME_INTERNAL
#include "game_debug.cpp"
#else
extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
}
#endif


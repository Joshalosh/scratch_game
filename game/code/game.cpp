#include "game.h"
#include "game_render_group.cpp"
#include "game_audio.cpp"
#include "game_asset.cpp"
#include "game_world.cpp"
#include "game_sim_region.cpp"
#include "game_entity.cpp"
#include "game_world_mode.cpp"
#include "game_meta.cpp"
#include "game_cutscene.cpp"

internal task_with_memory *
BeginTaskWithMemory(transient_state *TranState)
{
    task_with_memory *FoundTask = 0;

    for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
    {
        task_with_memory *Task = TranState->Tasks + TaskIndex;
        if(!Task->BeingUsed)
        {
            FoundTask = Task;
            Task->BeingUsed = true;
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

internal void
ClearBitmap(loaded_bitmap *Bitmap)
{
    if(Bitmap->Memory)
    {
        int32_t TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
        ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
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
    Result.Memory = PushSize(Arena, TotalBitmapSize, 16);
    if(ClearToZero)
    {
        ClearBitmap(&Result);
    }

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

internal game_assets *
DEBUGGetGameAssets(game_memory *Memory)
{
    game_assets *Assets = 0;

    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    if(TranState->IsInitialised)
    {
        Assets = TranState->Assets;
    }

    return(Assets);
}

internal void
SetGameMode(game_state *GameState, game_mode GameMode)
{
    Clear(&GameState->ModeArena);
    GameState->GameMode = GameMode;
}

#if GAME_INTERNAL
game_memory *DebugGlobalMemory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Platform = Memory->PlatformAPI;

#if GAME_INTERNAL
    DebugGlobalMemory = Memory;
#endif
    TIMED_FUNCTION();

    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialised)
    {
        memory_arena TotalArena;
        InitialiseArena(&TotalArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

        SubArena(&GameState->AudioArena, &TotalArena, Megabytes(1));
        SubArena(&GameState->ModeArena, &TotalArena,
                 GetArenaSizeRemaining(&TotalArena));

        InitialiseAudioState(&GameState->AudioState, &GameState->AudioArena);
        //PlayIntroCutscene(GameState);
        PlayTitleScreen(GameState);

        GameState->IsInitialised = true;
    }

    // Transient initialisation
    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    if(!TranState->IsInitialised)
    {
        InitialiseArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                        (uint8_t *)Memory->TransientStorage + sizeof(transient_state));

        TranState->HighPriorityQueue = Memory->HighPriorityQueue;
        TranState->LowPriorityQueue  = Memory->LowPriorityQueue;
        for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
        {
            task_with_memory *Task = TranState->Tasks + TaskIndex;

            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, Megabytes(1));
        }

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(256), TranState);

//        GameState->Music = PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));

        // TODO: Pick a real number here.
        TranState->GroundBufferCount = 256;
        TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
        for(uint32_t GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
            GroundBuffer->P = NullPosition();
        }

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

        TranState->IsInitialised = true;
    }

    DEBUG_IF(GroundChunks_RecomputeOnEXEChange)
    {
        if(Memory->ExecutableReloaded)
        {
            for(uint32_t GroundBufferIndex = 0;
                GroundBufferIndex < TranState->GroundBufferCount;
                ++GroundBufferIndex)
            {
                ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
                GroundBuffer->P = NullPosition();
            }
        }
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

    loaded_bitmap DrawBuffer_ = {};
    loaded_bitmap *DrawBuffer = &DrawBuffer_;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    DrawBuffer->Memory = Buffer->Memory;

    DEBUG_IF(Renderer_TestWeirdDrawBufferSize)
    {
        // Enable this to test weird buffer sizes in the renderer.
        DrawBuffer->Width = 1279;
        DrawBuffer->Height = 719;
    }

    // TODO: Need to figure out what the pushbuffer size is.
    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
    BeginRender(RenderGroup);

    b32 Rerun = false;
    do 
    {
        switch(GameState->GameMode)
        {
            case GameMode_TitleScreen:
            {
                Rerun = UpdateAndRenderTitleScreen(GameState, TranState->Assets, RenderGroup, DrawBuffer,
                                                   Input, GameState->TitleScreen);
            } break;

            case GameMode_Cutscene:
            {
                Rerun = UpdateAndRenderCutscene(GameState, TranState->Assets, RenderGroup, DrawBuffer,
                                                Input, GameState->Cutscene);
            } break;

            case GameMode_World:
            {
                Rerun = UpdateAndRenderWorld(GameState, GameState->WorldMode, TranState, Input, RenderGroup, DrawBuffer);
            } break;

            InvalidDefaultCase;
        }
    } while(Rerun);

    if(AllResourcesPresent(RenderGroup))
    {
        TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);
    }
    EndRender(RenderGroup);

    EndTemporaryMemory(RenderMemory);

#if 0
    if(!HeroesExist &&QuitRequested)
    {
        Memory->QuitRequested = true;
    }
#endif

    CheckArena(&GameState->ModeArena);
    CheckArena(&TranState->TranArena);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    transient_state *TranState = (transient_state *)Memory->TransientStorage;

    OutputPlayingSounds(&GameState->AudioState, SoundBuffer, TranState->Assets, &TranState->TranArena);
}

#if GAME_INTERNAL
#include "game_debug.cpp"
#else
extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
    return(0);
}
#endif


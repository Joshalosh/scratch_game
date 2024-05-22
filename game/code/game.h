#if !defined(GAME_H)

#include "game_platform.h"
#include "game_config.h"
#include "game_shared.h"
#include "game_random.h"
#include "game_file_formats.h"
#include "game_cutscene.h"


#define DLIST_INSERT(Sentinel, Element)     \
    (Element)->Next = (Sentinel)->Next;     \
    (Element)->Prev = (Sentinel);           \
    (Element)->Next->Prev = (Element);      \
    (Element)->Prev->Next = (Element);
#define DLIST_INSERT_AS_LAST(Sentinel, Element) \
    (Element)->Next = (Sentinel);               \
    (Element)->Prev = (Sentinel)->Prev;         \
    (Element)->Next->Prev = (Element);          \
    (Element)->Prev->Next = (Element);

#define DLIST_INIT(Sentinel)       \
    (Sentinel)->Next = (Sentinel); \
    (Sentinel)->Prev = (Sentinel);

#define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode) \
    (Result) = (FreeListPointer); \
    if(Result) {FreeListPointer = (Result)->NextFree;} else {Result = AllocationCode;}
#define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
    if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#include "game_world.h"
#include "game_brain.h"
#include "game_entity.h"
#include "game_sim_region.h"
#include "game_world_mode.h"
#include "game_render.h"
#include "game_render_group.h"
#include "game_asset.h"
#include "game_audio.h"

struct controlled_hero
{
    brain_id BrainID;
    r32 RecentreTimer;
    v2 ddP;
};

struct hero_bitmap_ids
{
    bitmap_id Head;
    bitmap_id Cape;
    bitmap_id Torso;
};

enum game_mode
{
    GameMode_None,

    GameMode_TitleScreen,
    GameMode_Cutscene,
    GameMode_World,
};

struct game_state
{
    bool32 IsInitialised;
    
    memory_arena ModeArena;
    memory_arena AudioArena; // TODO: Move this into the audio system proper

    controlled_hero ControlledHeroes[MAX_CONTROLLER_COUNT];

    loaded_bitmap TestDiffuse; // TODO: Re-fill this bad boy with grey.
    loaded_bitmap TestNormal;

    audio_state AudioState;
    playing_sound *Music;

    game_mode GameMode;
    union
    {
        game_mode_title_screen *TitleScreen;
        game_mode_cutscene *Cutscene;
        game_mode_world *WorldMode;
    };
};

struct task_with_memory
{
    b32 BeingUsed;
    b32 DependsOnGameMode;
    memory_arena Arena;

    temporary_memory MemoryFlush;
};

struct transient_state
{
    bool32 IsInitialised;
    memory_arena TranArena;

    task_with_memory Tasks[4];

    game_assets *Assets;
    u32 MainGenerationID;

    platform_work_queue *HighPriorityQueue;
    platform_work_queue *LowPriorityQueue;

    uint32_t EnvMapWidth;
    uint32_t EnvMapHeight;
    // 0 is bottom, 1 is middle and 2 is top.
    environment_map EnvMaps[3];
};

global_variable platform_api Platform;

internal task_with_memory *BeginTaskWithMemory(transient_state *TranState, b32 DependsOnGameMode);
internal void EndTaskWithMemory(task_with_memory *Task);
internal void SetGameMode(game_state *GameState, transient_state *TranState, game_mode GameMode);

// TODO: Get these into a more reasonable location
#define GroundBufferWidth 256
#define GroundBufferHeight 256

#define GAME_H
#endif

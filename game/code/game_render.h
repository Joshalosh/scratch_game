
struct sprite_bound
{
    s32 ChunkZ;
    r32 YMin;
    r32 YMax;
    r32 ZMax;
    manual_sort_key Manual;
};

struct sprite_edge
{
    sprite_edge *NextEdgeWithSameFront;
    u32 Front;
    u32 Behind;
};

enum sprite_flag
{
    Sprite_Visited  = 0x10000000,
    Sprite_Drawn    = 0x20000000,
    Sprite_DebugBox = 0x40000000,
    Sprite_Cycle    = 0x80000000,

    Sprite_IndexMask = 0x0FFFFFFF,

    Sprite_BarrierTurnsOffSorting = 0xFFFF0000,
};

#define SPRITE_BARRIER_OFFSET_VALUE 0xFFFFFFFF
struct sort_sprite_bound
{
    sprite_edge *FirstEdgeWithMeAsFront;
    rectangle2 ScreenArea;
    sprite_bound SortKey;
    u32 Offset;
    u32 Flags;
#if GAME_SLOW
    u32 DebugTag;
#endif
};

struct sprite_graph_walk
{
    sort_sprite_bound *InputNodes;
    u32 *OutIndex;
    b32 HitCycle;
};

struct tile_render_work
{
    game_render_commands *Commands;
    game_render_prep *Prep;
    struct loaded_bitmap *RenderTargets;
    rectangle2i ClipRect;
};

#define SORT_GRID_WIDTH   64
#define SORT_GRID_HEIGHT  36
struct sort_grid_entry
{
    sort_grid_entry *Next;
    u32 OccupantIndex;
};

inline b32
IsZSprite(sprite_bound A)
{
    b32 Result = (A.YMin != A.YMax);
    return(Result);
}

struct texture_op_allocate
{
    u32 Width;
    u32 Height;
    void *Data;

    void **ResultHandle;
};
struct texture_op_deallocate
{
    void *Handle;
};
struct texture_op
{
    texture_op *Next;
    b32 IsAllocate;
    union 
    {
        texture_op_allocate Allocate;
        texture_op_deallocate Deallocate;
    };
};

struct camera_params
{
    f32 WidthOfMonitor;
    f32 MetersToPixels;
    f32 FocalLength;
};
inline camera_params
GetStandardCameraParams(u32 WidthInPixels, f32 FocalLength)
{
    camera_params Result;

    // NOTE: Horizontal measurement of monitor in meters
    Result.WidthOfMonitor = 0.635f;
    Result.MetersToPixels = (f32)WidthInPixels / Result.WidthOfMonitor;
    Result.FocalLength = FocalLength;

    return(Result);
}



struct sprite_bound 
{
    r32 YMin;
    r32 YMax;
    r32 ZMax;
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
};
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
    struct loaded_bitmap *OutputTarget;
    rectangle2i ClipRect;
};

inline sort_sprite_bound *
GetSortEntries(game_render_commands *Commands)
{
    sort_sprite_bound *SortEntries = (sort_sprite_bound *)(Commands->PushBufferBase + Commands->SortEntryAt);
    return(SortEntries);
}

#define SORT_GRID_WIDTH  64
#define SORT_GRID_HEIGHT 36
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


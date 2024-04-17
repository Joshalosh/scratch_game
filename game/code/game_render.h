
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
    Sprite_Visited = 0x1,
    Sprite_Drawn = 0x2,

    Sprite_DebugBox = 0x4,
    Sprite_Cycle = 0x10,
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



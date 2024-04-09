
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
};
struct sort_sprite_bound
{
    sprite_edge *FirstEdgeWithMeAsFront;
    rectangle2 ScreenArea;
    sprite_bound SortKey;
    u32 Offset;
    u32 Flags;
};

struct sprite_graph_walk
{
    sort_sprite_bound *InputNodes;
    u32 *OutIndex;
};

struct tile_render_work
{
    game_render_commands *Commands;
    game_render_prep *Prep;
    loaded_bitmap *OutputTarget;
    rectangle2i ClipRect;
};


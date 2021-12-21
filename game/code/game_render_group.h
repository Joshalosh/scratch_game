#if !defined(GAME_RENDER_GROUP_H)

struct render_basis
{
    v3 P;
};

struct render_entity_basis
{
    render_basis *Basis;
    v2 Offset;
    real32 OffsetZ;
    real32 EntityZC;
};

// render_group_entry is a "compact discriminated union"
enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_coordinate_system,
};
struct render_group_entry_header
{
    render_group_entry_type Type;
};

struct render_entry_clear
{
    render_group_entry_header Header;
    v4 Color;
};

struct render_entry_coordinate_system
{
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
};

struct render_entry_bitmap
{
    render_group_entry_header Header;
    loaded_bitmap *Bitmap;
    render_entity_basis EntityBasis;
    real32 R, G, B, A;
};

struct render_entry_rectangle
{
    render_group_entry_header Header;
    render_entity_basis EntityBasis;
    real32 R, G, B, A;
    v2 Dim;
};

struct render_group
{
    render_basis *DefaultBasis;
    real32 MetresToPixels;

    uint32_t MaxPushBufferSize;
    uint32_t PushBufferSize;
    uint8_t *PushBufferBase;
};

#define GAME_RENDER_GROUP_H
#endif

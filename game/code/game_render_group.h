#if !defined(GAME_RENDER_GROUP_H)

struct loaded_bitmap
{
    int32_t Width;
    int32_t Height;
    int32_t Pitch;
    void *Memory;
};

struct environment_map
{
    loaded_bitmap LOD[4];
};

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
// TODO: Remove the header.
enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_coordinate_system,
    RenderGroupEntryType_render_entry_saturation,
};
struct render_group_entry_header
{
    render_group_entry_type Type;
};

struct render_entry_clear
{
    v4 Color;
};

struct render_entry_saturation
{
    real32 Level;
};

struct render_entry_coordinate_system
{
    v2 Origin;
    v2 XAxis;
    v2 YAxis;
    v4 Color;
    loaded_bitmap *Texture;
    loaded_bitmap *NormalMap;

    environment_map *Top;
    environment_map *Middle;
    environment_map *Bottom;
};

struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;
    render_entity_basis EntityBasis;
    v4 Color;
};

struct render_entry_rectangle
{
    render_entity_basis EntityBasis;
    v4 Color;
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

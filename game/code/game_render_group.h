#if !defined(GAME_RENDER_GROUP_H)

/*

   1) Everywhere outside the renderer, Y ALWAYS goes upwards, X to the right.

   2) All bitmaps including the render target are assumed to be bottom-up
      (meaning that the first row pointer points to the bottom-most row
       when viewed on screen).

    3) Unless otherwise specified, all inputs to the renderer are in world
       coordinate ("metres"), NOT pixels. Anything that is in pixel values
       will be explicitly marked as such.

    4) Z is special coordinate because it is broken up into discrete slices,
       and the renderer actually understands these slices. Z slices are what
       control the _scaling_ of things, whereas Z offsets inside a slice are
       what control Y offsetting.

    5) All color values specified to the renderer as V4's are in
       non-premultiplied aloha.

*/

struct loaded_bitmap
{
    v2 Align;

    int32_t Width;
    int32_t Height;
    int32_t Pitch;
    void *Memory;
};

struct environment_map
{
    loaded_bitmap LOD[4];
    real32 Pz;
};

struct render_basis
{
    v3 P;
};

struct render_entity_basis
{
    render_basis *Basis;
    v3 Offset;
};

// render_group_entry is a "compact discriminated union"
// TODO: Remove the header.
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
    v4 Color;
};

struct render_entry_saturation
{
    real32 Level;
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

// This is purely for testing purposes:
// {
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
// }

struct render_group
{
    real32 GlobalAlpha;

    render_basis *DefaultBasis;
    real32 MetresToPixels;

    uint32_t MaxPushBufferSize;
    uint32_t PushBufferSize;
    uint8_t *PushBufferBase;
};

// Renderer API
#if 0
inline void PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v2 Offset, real32 OffsetZ,
                       v4 Color = V4(1, 1, 1, 1));
inline void PushRect(render_group *Group, v2 Offset, real32 OffsetZ, v2 Dim, v4 Color);
inline void PushRectOutline(render_group *Group, v2 Offset, real32 OffsetZ, v2 Dim, v4 Color);
inline void Clear(render_group *Group, v4 Color);
#endif



#define GAME_RENDER_GROUP_H
#endif

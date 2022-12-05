#if !defined(GAME_RENDER_GROUP_H)

/*

   1) Everywhere outside the renderer, Y ALWAYS goes upwards, X to the right.

   2) All bitmaps including the render target are assumed to be bottom-up
      (meaning that the first row pointer points to the bottom-most row
       when viewed on screen).

    3) It is mandatory that all inputs to the renderer are in world
       coordinate ("metres"), NOT pixels. If for some reasone something
       absolutely has to be specified in pixels, then it will be explicitly
       marked in the API, but this should happen rarely.

    4) Z is special coordinate because it is broken up into discrete slices,
       and the renderer actually understands these slices. Z slices are what
       control the _scaling_ of things, whereas Z offsets inside a slice are
       what control Y offsetting.

    5) All color values specified to the renderer as V4's are in
       non-premultiplied aloha.

*/

struct loaded_bitmap
{
    void *Memory;
    v2 AlignPercentage;
    r32 WidthOverHeight;
    s16 Width;
    s16 Height;
    s16 Pitch;
};

struct environment_map
{
    loaded_bitmap LOD[4];
    real32 Pz;
};

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

    v4 Color;
    v2 P;
    v2 Size;
};

struct render_entry_rectangle
{
    v4 Color;
    v2 P;
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

//    real32 PixelsToMetres; // TODO: I need to store this for lighting.

    environment_map *Top;
    environment_map *Middle;
    environment_map *Bottom;
};
// }

struct render_transform
{
    bool32 Orthographic;

    // TODO: Camera parameters.
    real32 MetresToPixels; // This translates metres on the monitor into pixels on the monitor.
    v2 ScreenCentre;

    real32 FocalLength;
    real32 DistanceAboveTarget;

    v3 OffsetP;
    real32 Scale;
};

struct render_group
{
    struct game_assets *Assets;
    real32 GlobalAlpha;

    v2 MonitorHalfDimInMetres;

    render_transform Transform;

    uint32_t MaxPushBufferSize;
    uint32_t PushBufferSize;
    uint8_t *PushBufferBase;

    uint32_t MissingResourceCount;
};

void DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                          loaded_bitmap *Texture, real32 PixelsToMetres,
                          rectangle2i ClipRect, bool32 Even);

#define GAME_RENDER_GROUP_H
#endif

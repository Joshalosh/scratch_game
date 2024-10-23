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
    s32 Width;
    s32 Height;
    // TODO: Get rid of the pitch.
    s32 Pitch;
    void *TextureHandle;
};

struct environment_map
{
    loaded_bitmap LOD[4];
    real32 Pz;
};

enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_bitmap,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_cliprect,
    RenderGroupEntryType_render_entry_coordinate_system,
    RenderGroupEntryType_render_entry_blend_render_target,
};
struct render_group_entry_header // TODO: Don't store type here, perhaps better to store in sort index
{
    u16 Type;
    u16 ClipRectIndex;

#if GAME_SLOW
    u32 DebugTag;
#endif
};

struct render_entry_cliprect
{
    render_entry_cliprect *Next;
    rectangle2i Rect;
    u32 RenderTargetIndex;
};

struct render_entry_clear
{
    v4 PremulColor;
};

struct render_entry_saturation
{
    real32 Level;
};

struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;

    v4 PremulColor;
    v2 P;

    // NOTE: X and Y axes are already scaled by the dimension
    v2 XAxis;
    v2 YAxis;
};

struct render_entry_rectangle
{
    v4 PremulColor;
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

struct render_entry_blend_render_target
{
    u32 SourceTargetIndex;
    r32 Alpha;
};

struct object_transform
{
    b32 Upright;
    v3 OffsetP;
    r32 Scale;
    r32 FloorZ;
    r32 NextFloorZ;
    s32 ChunkZ;
    manual_sort_key ManualSort;
    v4 tColor;
    v4 Color;
};

struct camera_transform
{
    b32 Orthographic;

    // TODO: Camera parameters.
    r32 MetresToPixels; // This translates metres on the monitor into pixels on the monitor.
    v2 ScreenCentre;

    r32 FocalLength;
    r32 DistanceAboveTarget;
};

struct render_group
{
    struct game_assets *Assets;

    rectangle2 ScreenArea;

#if GAME_SLOW
    u32 DebugTag;
#endif
    v2 MonitorHalfDimInMetres;

    camera_transform CameraTransform;

    uint32_t MissingResourceCount;
    b32 RendersInBackground;

    u32 CurrentClipRectIndex;

    u32 GenerationID;
    game_render_commands *Commands;

    sprite_bound AggregateBound;
    sort_sprite_bound *FirstAggregate;
};

struct entity_basis_p_result
{
    v2 P;
    r32 Scale;
    b32 Valid;
};

struct used_bitmap_dim
{
    entity_basis_p_result Basis;
    v2 Size;
    v2 Align;
    v3 P;
};

struct push_buffer_result
{
    sort_sprite_bound *SortEntry;
    render_group_entry_header *Header;
};

void DrawRectangleQuickly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                          loaded_bitmap *Texture, real32 PixelsToMetres,
                          rectangle2i ClipRect);

inline object_transform
DefaultUprightTransform(void)
{
    object_transform Result = {};

    Result.Upright = true;
    Result.Scale = 1.0f;

    return(Result);
}

inline object_transform
DefaultFlatTransform(void)
{
    object_transform Result = {};

    Result.Scale = 1.0f;

    return(Result);
}

struct transient_clip_rect
{
    transient_clip_rect(render_group *RenderGroupInit, u32 NewClipRectIndex)
    {
        RenderGroup = RenderGroupInit;
        OldClipRect = RenderGroup->CurrentClipRectIndex;
        RenderGroup->CurrentClipRectIndex = NewClipRectIndex;
    }

    transient_clip_rect(render_group *RenderGroupInit)
    {
        RenderGroup = RenderGroupInit;
        OldClipRect = RenderGroup->CurrentClipRectIndex;
    }

    ~transient_clip_rect()
    {
        RenderGroup->CurrentClipRectIndex = OldClipRect;
    }

    render_group *RenderGroup;
    u32 OldClipRect;
};


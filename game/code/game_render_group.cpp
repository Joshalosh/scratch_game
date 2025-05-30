
inline u16
ReserveSortKey(render_group *RenderGroup)
{
    game_render_commands *Commands = RenderGroup->Commands;
    Assert(Commands->LastUsedManualSortKey < U16Maximum)
    u16 Result = (u16)++Commands->LastUsedManualSortKey;
    return(Result);
}

inline entity_basis_p_result GetRenderEntityBasisP(camera_transform *CameraTransform,
                                                   object_transform *ObjectTransform,
                                                   v3 OriginalP)
{
    entity_basis_p_result Result = {};

    v3 P = OriginalP + ObjectTransform->OffsetP;
    if(CameraTransform->Orthographic)
    {
        Result.P = CameraTransform->ScreenCentre + CameraTransform->MetresToPixels*P.xy;
        Result.Scale = CameraTransform->MetresToPixels;
        Result.Valid = true;
    }
    else
    {
        real32 DistanceAboveTarget = CameraTransform->DistanceAboveTarget;

        if(Global_Renderer_Camera_UseDebug)
        {
            DistanceAboveTarget += Global_Renderer_Camera_DebugDistance;
        }

        f32 FloorZ = ObjectTransform->FloorZ;
        real32 DistanceToPZ = (DistanceAboveTarget - FloorZ);
        real32 NearClipPlane = 0.1f;

        if(DistanceToPZ > NearClipPlane)
        {
            f32 HeightOffFloor = P.z - FloorZ;

            v3 RawXY = V3(P.xy, 1.0f);
            f32 OrthoFromZ = 1.0f;
            RawXY.y += OrthoFromZ*HeightOffFloor;

            v3 ProjectedXY = (1.0f / DistanceToPZ) * CameraTransform->FocalLength*RawXY;
            Result.Scale = CameraTransform->MetresToPixels*ProjectedXY.z;
            Result.P = CameraTransform->ScreenCentre + CameraTransform->MetresToPixels*ProjectedXY.xy; 
            Result.Valid = true;
        }
    }

    return(Result);
}

inline push_buffer_result
PushBuffer(render_group *RenderGroup, u32 SortEntryCount, u32 DataSize)
{
    game_render_commands *Commands = RenderGroup->Commands;
    sort_sprite_bound *SpriteBounds = GetSortEntries(Commands);

    push_buffer_result Result = {};

    if((u8 *)(SpriteBounds + (Commands->SortEntryCount + SortEntryCount)) <=
       (Commands->PushBufferDataAt - DataSize))
    {
        Commands->PushBufferDataAt -= DataSize;
        Result.Header = (render_group_entry_header *)Commands->PushBufferDataAt;

        Result.SortEntry = SpriteBounds + Commands->SortEntryCount;
        Commands->SortEntryCount += SortEntryCount;
    }
    else
    {
        InvalidCodePath;
    }

    return(Result);
}

inline void
PushSortBarrier(render_group *RenderGroup, b32 TurnOffSorting)
{
    push_buffer_result Push = PushBuffer(RenderGroup, 1, 0);
    if(Push.SortEntry)
    {
        Push.SortEntry->Offset = SPRITE_BARRIER_OFFSET_VALUE;
        Push.SortEntry->Flags = TurnOffSorting ? Sprite_BarrierTurnsOffSorting : 0;
    }
}

#define PushRenderElement(Group, type, SortKey, ScreenArea) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type, SortKey, ScreenArea)
inline void *
PushRenderElement_(render_group *Group, uint32_t Size, render_group_entry_type Type, 
                   sprite_bound SortKey, rectangle2 ScreenArea)
{
    game_render_commands *Commands = Group->Commands;

    void *Result = 0;

    Size += sizeof(render_group_entry_header);
    push_buffer_result Push = PushBuffer(Group, 1, Size);
    if(Push.SortEntry)
    {
        render_group_entry_header *Header = Push.Header;
        Header->Type = (u16)Type;
        Header->ClipRectIndex = SafeTruncateToU16(Group->CurrentClipRectIndex);
        Result = (uint8_t *)Header + sizeof(*Header);
#if GAME_SLOW
        Header->DebugTag = Group->DebugTag;
#endif

        sort_sprite_bound *Entry = Push.SortEntry;
        Entry->FirstEdgeWithMeAsFront = 0;
        Entry->SortKey = SortKey;
        Entry->Offset = (u32)((u8 *)Header - Commands->PushBufferBase);
        Entry->ScreenArea = ScreenArea;
        Entry->Flags = 0;
#if GAME_SLOW
        Entry->DebugTag = Group->DebugTag;
#endif
        Assert(Entry->Offset != 0);

        if(Group->FirstAggregate)
        {
            if(Group->FirstAggregate == Push.SortEntry)
            {
                Group->AggregateBound = SortKey;
            }
            else if(IsZSprite(Group->AggregateBound))
            {
                Assert(IsZSprite(SortKey));
                Group->AggregateBound.YMin = Minimum(Group->AggregateBound.YMin, SortKey.YMin);
                Group->AggregateBound.YMax = Maximum(Group->AggregateBound.YMax, SortKey.YMax);
                Group->AggregateBound.ZMax = Maximum(Group->AggregateBound.ZMax, SortKey.ZMax);
            }
            else
            {
                Assert(!IsZSprite(SortKey));
                // TODO: Should I try to let the user specify what should happen for 
                // y selection here?
                //Group->AggregateBound.YMin = Minimum(Group->AggregateBound.YMin, SortKey.YMin);
                //Group->AggregateBound.YMax = Maximum(Group->AggregateBound.YMax, SortKey.YMax);
                Group->AggregateBound.ZMax = Maximum(Group->AggregateBound.ZMax, SortKey.ZMax);
            }
        }
    }
    else
    {
        InvalidCodePath;
    }

    return(Result);
}

inline void
BeginAggregateSortKey(render_group *RenderGroup)
{
    Assert(!RenderGroup->FirstAggregate);

    game_render_commands *Commands = RenderGroup->Commands;
    RenderGroup->FirstAggregate = GetSortEntries(Commands) + Commands->SortEntryCount;
    RenderGroup->AggregateBound.YMin = R32Maximum;
    RenderGroup->AggregateBound.YMax = R32Minimum;
    RenderGroup->AggregateBound.ZMax = R32Minimum;
}

inline void
EndAggregateSortKey(render_group *RenderGroup)
{
    Assert(RenderGroup->FirstAggregate);

    game_render_commands *Commands = RenderGroup->Commands;
    sort_sprite_bound *OnePastLastEntry = GetSortEntries(Commands) + Commands->SortEntryCount;
    for(sort_sprite_bound *Entry = RenderGroup->FirstAggregate; Entry != OnePastLastEntry; ++Entry)
    {
        Entry->SortKey = RenderGroup->AggregateBound;
    }

    RenderGroup->FirstAggregate = 0;
}

inline used_bitmap_dim
GetBitmapDim(render_group *Group, object_transform *ObjectTransform,
             loaded_bitmap *Bitmap, r32 Height, v3 Offset, r32 CAlign,
             v2 XAxis = V2(1, 0), v2 YAxis = V2(0, 1))
{
    used_bitmap_dim Dim;

    Dim.Size = V2(Height*Bitmap->WidthOverHeight, Height);
    Dim.Align = CAlign*Hadamard(Bitmap->AlignPercentage, Dim.Size);
    Dim.P.z = Offset.z;
    Dim.P.xy = Offset.xy - Dim.Align.x*XAxis - Dim.Align.y*YAxis;
    Dim.Basis = GetRenderEntityBasisP(&Group->CameraTransform, ObjectTransform, Dim.P);

    return(Dim);
}

inline v4
StoreColor(object_transform *Transform, v4 Source)
{
    v4 Dest;
    v4 t = Transform->tColor;
    v4 C = Transform->Color;

    Dest.a = Lerp(Source.a, t.a, C.a);

    Dest.r = Dest.a*Lerp(Source.r, t.r, C.r);
    Dest.g = Dest.a*Lerp(Source.g, t.g, C.g);
    Dest.b = Dest.a*Lerp(Source.b, t.b, C.b);

    return(Dest);
}

inline sprite_bound
GetBoundFor(object_transform *ObjectTransform, v3 Offset, r32 Height)
{
    sprite_bound SpriteBound;
    SpriteBound.Manual = ObjectTransform->ManualSort;
    SpriteBound.ChunkZ = ObjectTransform->ChunkZ;
    SpriteBound.YMin = SpriteBound.YMax = ObjectTransform->OffsetP.y + Offset.y;
    SpriteBound.ZMax = ObjectTransform->OffsetP.z + Offset.z;
    if(ObjectTransform->Upright)
    {
        // TODO: More accurate ZMax calculations - this doesn't handle
        // alignment, rotation, or axis shear/scale
        SpriteBound.ZMax += 0.5f*Height;
    }
    else
    {
        // TODO: More accurate ZMax calculations - this doesn't handle
        // alignment, rotation, or axis shear/scale
        SpriteBound.YMin -= 0.5f*Height;
        SpriteBound.YMax += 0.5f*Height;
    }

    return(SpriteBound);
}

inline void
PushBitmap(render_group *Group, object_transform *ObjectTransform,
           loaded_bitmap *Bitmap, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1), r32 CAlign = 1.0f,
           v2 XAxis = V2(1, 0), v2 YAxis = V2(0, 1))
{
    used_bitmap_dim Dim = GetBitmapDim(Group, ObjectTransform, Bitmap, Height, Offset, CAlign, XAxis, YAxis);
    if(Dim.Basis.Valid)
    {
        v2 Size = Dim.Basis.Scale*Dim.Size;
        sprite_bound SpriteBound = GetBoundFor(ObjectTransform, Offset, Height);
        // TODO: I need more conservative bounds here
        rectangle2 ScreenArea = RectMinDim(Dim.Basis.P, Size);
        render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap, SpriteBound, ScreenArea);
        if(Entry)
        {
            Entry->Bitmap = Bitmap;
            Entry->P = Dim.Basis.P;
            Entry->PremulColor = StoreColor(ObjectTransform, Color);
            Entry->XAxis = Size.x*XAxis;
            Entry->YAxis = Size.y*YAxis;
        }
    }
}

inline void
PushBitmap(render_group *Group, object_transform *ObjectTransform,
           bitmap_id ID, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1), r32 CAlign = 1.0f,
           v2 XAxis = V2(1, 0), v2 YAxis = V2(0, 1))
{

    loaded_bitmap *Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
    if(Group->RendersInBackground && !Bitmap)
    {
        LoadBitmap(Group->Assets, ID, true);
        Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
    }

    if(Bitmap)
    {
        PushBitmap(Group, ObjectTransform, Bitmap, Height, Offset, Color, CAlign, XAxis, YAxis);
    }
    else
    {
        Assert(!Group->RendersInBackground);
        LoadBitmap(Group->Assets, ID, false);
        ++Group->MissingResourceCount;
    }
}

inline loaded_font *
PushFont(render_group *Group, font_id ID)
{
    loaded_font *Font = GetFont(Group->Assets, ID, Group->GenerationID);
    if(Font)
    {
        // Nothing to do.
    }
    else
    {
        Assert(!Group->RendersInBackground);
        LoadFont(Group->Assets, ID, false);
        ++Group->MissingResourceCount;
    }

    return(Font);
}

inline void
PushRect(render_group *Group, object_transform *ObjectTransform, v3 Offset, v2 Dim, v4 Color = V4(1, 1, 1, 1))
{
    v3 P = (Offset - V3(0.5f*Dim, 0));
    entity_basis_p_result Basis = GetRenderEntityBasisP(&Group->CameraTransform, ObjectTransform, P);
    if(Basis.Valid)
    {
        v2 ScaledDim = Basis.Scale*Dim;
        sprite_bound SpriteBound = GetBoundFor(ObjectTransform, Offset, Dim.y);
        rectangle2 ScreenArea = RectMinDim(Basis.P, ScaledDim);
        render_entry_rectangle *Rect = PushRenderElement(Group, render_entry_rectangle, SpriteBound, ScreenArea);
        if(Rect)
        {
            Rect->P = Basis.P;
            Rect->PremulColor = StoreColor(ObjectTransform, Color);
            Rect->Dim = ScaledDim;
        }
    }
}

inline void
PushRect(render_group *Group, object_transform *ObjectTransform, rectangle2 Rectangle, r32 Z, v4 Color = V4(1, 1, 1, 1))
{
    PushRect(Group, ObjectTransform, V3(GetCenter(Rectangle), Z), GetDim(Rectangle), Color);
}

inline void
PushRectOutline(render_group *Group, object_transform *ObjectTransform, v3 Offset, v2 Dim, v4 Color = V4(1, 1, 1, 1), real32 Thickness = 0.1f)
{
    // Top and Bottom.
    PushRect(Group, ObjectTransform, Offset - V3(0, 0.5f*Dim.y, 0), V2(Dim.x-Thickness-0.01f, Thickness), Color);
    PushRect(Group, ObjectTransform, Offset + V3(0, 0.5f*Dim.y, 0), V2(Dim.x-Thickness-0.01f, Thickness), Color);

    // Left and Right.
    PushRect(Group, ObjectTransform, Offset - V3(0.5f*Dim.x, 0, 0), V2(Thickness, Dim.y + Thickness), Color);
    PushRect(Group, ObjectTransform, Offset + V3(0.5f*Dim.x, 0, 0), V2(Thickness, Dim.y + Thickness), Color);
}

inline void
PushRectOutline(render_group *Group, object_transform *ObjectTransform, rectangle2 Rectangle, r32 Z, v4 Color = V4(1, 1, 1, 1), real32 Thickness = 0.1f)
{
    PushRectOutline(Group, ObjectTransform, V3(GetCenter(Rectangle), Z), GetDim(Rectangle), Color, Thickness);
}

inline void
Clear(render_group *Group, v4 Color)
{
    Group->Commands->ClearColor = Color;
}

inline void
CoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
                 loaded_bitmap *Texture, loaded_bitmap *NormalMap,
                 environment_map *Top, environment_map *Middle, environment_map *Bottom)
{
#if 0
    entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
    if(Basis.Valid)
    {
        render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
        if(Entry)
        {
            Entry->Origin = Origin;
            Entry->XAxis = XAxis;
            Entry->YAxis = YAxis;
            Entry->Color = Color;
            Entry->Texture = Texture;
            Entry->NormalMap = NormalMap;
            Entry->Top = Top;
            Entry->Middle = Middle;
            Entry->Bottom = Bottom;
        }
    }
#endif
}

inline u32
PushClipRect(render_group *Group, u32 X, u32 Y, u32 W, u32 H, u32 RenderTargetIndex)
{
    u32 Result = 0;

    u32 Size = sizeof(render_entry_cliprect);
    push_buffer_result Push = PushBuffer(Group, 0, Size);
    if(Push.Header)
    {
        render_entry_cliprect *Rect = (render_entry_cliprect *)Push.Header;

        Result = Group->Commands->ClipRectCount++;

        if(Group->Commands->LastRect)
        {
            Group->Commands->LastRect = Group->Commands->LastRect->Next = Rect;
        }
        else
        {
            Group->Commands->LastRect = Group->Commands->FirstRect = Rect;
        }
        Rect->Next = 0;

        Rect->Rect.MinX = X;
        Rect->Rect.MinY = Y;
        Rect->Rect.MaxX = X + W;
        Rect->Rect.MaxY = Y + H;

        Rect->RenderTargetIndex = RenderTargetIndex;
        if(Group->Commands->MaxRenderTargetIndex < RenderTargetIndex)
        {
            Group->Commands->MaxRenderTargetIndex = RenderTargetIndex;
        }
    }

    return(Result);
}

inline u32
PushClipRect(render_group *Group, object_transform *ObjectTransform, v3 Offset, v2 Dim, u32 RenderTargetIndex)
{
    u32 Result = 0;

    v3 P = (Offset - V3(0.5f*Dim, 0));
    entity_basis_p_result Basis = GetRenderEntityBasisP(&Group->CameraTransform, ObjectTransform, P);
    if(Basis.Valid)
    {
        v2 P = Basis.P;
        v2 DimB = Basis.Scale*Dim;

        Result = PushClipRect(Group,
                              RoundReal32ToInt32(P.x), RoundReal32ToInt32(P.y),
                              RoundReal32ToInt32(DimB.x), RoundReal32ToInt32(DimB.y), RenderTargetIndex);
    }

    return(Result);
}

inline u32
PushClipRect(render_group *Group, object_transform *ObjectTransform, rectangle2 Rectangle, r32 Z, u32 RenderTargetIndex)
{
    u32 Result = PushClipRect(Group, ObjectTransform, V3(GetCenter(Rectangle), Z), GetDim(Rectangle), RenderTargetIndex);
    return(Result);
}

inline void
PushBlendRenderTarget(render_group *Group, r32 Alpha, u32 SourceRenderTargetIndex)
{
    PushSortBarrier(Group, false);

    sprite_bound SortKey = {};
    rectangle2 ScreenArea = {};
    render_entry_blend_render_target *Blend = PushRenderElement(Group, render_entry_blend_render_target, 
                                                                SortKey, ScreenArea);
    Blend->SourceTargetIndex = SourceRenderTargetIndex;
    Blend->Alpha = Alpha;

    PushSortBarrier(Group, false);
}

inline v3
Unproject(render_group *Group, object_transform ObjectTransform, v2 PixelsXY)
{
    camera_transform Transform = Group->CameraTransform;

    v2 UnprojectedXY;
    if(Transform.Orthographic)
    {
        UnprojectedXY = (1.0f / Transform.MetresToPixels)*(PixelsXY - Transform.ScreenCentre);
    }
    else
    {
        v2 A = (PixelsXY - Transform.ScreenCentre) * (1.0f / Transform.MetresToPixels);
        UnprojectedXY = ((Transform.DistanceAboveTarget - ObjectTransform.OffsetP.z)/Transform.FocalLength) * A;
    }

    v3 Result = V3(UnprojectedXY, ObjectTransform.OffsetP.z);
    Result -= ObjectTransform.OffsetP;

    return(Result);
}

inline v2
UnprojectOld(render_group *Group, v2 ProjectedXY, real32 AtDistanceFromCamera)
{
    v2 WorldXY = (AtDistanceFromCamera / Group->CameraTransform.FocalLength)*ProjectedXY;
    return(WorldXY);
}

inline rectangle2
GetCameraRectangleAtDistance(render_group *Group, real32 DistanceFromCamera)
{
    v2 RawXY = UnprojectOld(Group, Group->MonitorHalfDimInMetres, DistanceFromCamera);

    rectangle2 Result = RectCenterHalfDim(V2(0, 0), RawXY);

    return(Result);
}

inline rectangle2
GetCameraRectangleAtTarget(render_group *Group)
{
    rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->CameraTransform.DistanceAboveTarget);

    return(Result);
}

inline bool32
AllResourcesPresent(render_group *Group)
{
    bool32 Result = (Group->MissingResourceCount == 0);

    return(Result);
}

inline render_group
BeginRenderGroup(game_assets *Assets, game_render_commands *Commands, u32 GenerationID, b32 RendersInBackground, 
                 int32_t PixelWidth, int32_t PixelHeight)
{
    render_group Result = {};

    Result.Assets = Assets;
    Result.RendersInBackground = RendersInBackground;
    Result.MissingResourceCount = 0;
    Result.GenerationID = GenerationID;
    Result.Commands = Commands;
    Result.ScreenArea = RectMinDim(V2(0, 0), V2i(PixelWidth, PixelHeight));
    Result.CurrentClipRectIndex = PushClipRect(&Result, 0, 0, PixelWidth, PixelHeight, 0);

    return(Result);
}

inline void
EndRenderGroup(render_group *RenderGroup)
{
    // TODO: Implement
    // RenderGroup->Commands->MissingResourceCount += RenderGroup->MissingResourceCount;
}

inline void
Perspective(render_group *RenderGroup,
            real32 MetresToPixels, real32 FocalLength, real32 DistanceAboveTarget)
{
    // TODO: I need to adjust this based on buffer size.
    real32 PixelsToMetres = SafeRatio1(1.0f, MetresToPixels);
    r32 PixelWidth = GetDim(RenderGroup->ScreenArea).x;
    r32 PixelHeight = GetDim(RenderGroup->ScreenArea).y;

    RenderGroup->MonitorHalfDimInMetres = {0.5f*PixelWidth*PixelsToMetres,
                                           0.5f*PixelHeight*PixelsToMetres};

    RenderGroup->CameraTransform.MetresToPixels = MetresToPixels;
    RenderGroup->CameraTransform.FocalLength = FocalLength; // Metres the person is sitting from their monitor.
    RenderGroup->CameraTransform.DistanceAboveTarget = DistanceAboveTarget;
    RenderGroup->CameraTransform.ScreenCentre = V2(0.5f*PixelWidth, 0.5f*PixelHeight);
    RenderGroup->CameraTransform.Orthographic = false;
}

inline void
Orthographic(render_group *RenderGroup, r32 MetresToPixels)
{
    r32 PixelsToMetres = SafeRatio1(1.0f, MetresToPixels);
    r32 PixelWidth = GetDim(RenderGroup->ScreenArea).x;
    r32 PixelHeight = GetDim(RenderGroup->ScreenArea).y;

    RenderGroup->MonitorHalfDimInMetres = {0.5f*PixelWidth*PixelsToMetres, 
                                           0.5f*PixelHeight*PixelsToMetres};

    RenderGroup->CameraTransform.MetresToPixels = MetresToPixels;
    RenderGroup->CameraTransform.FocalLength = 1.0f; // Metres the person is sitting from their monitor.
    RenderGroup->CameraTransform.DistanceAboveTarget = 1.0f;
    RenderGroup->CameraTransform.ScreenCentre = V2(0.5f*PixelWidth, 0.5f*PixelHeight);
    RenderGroup->CameraTransform.Orthographic = true;
}


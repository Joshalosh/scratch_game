
inline v4
SRGB255ToLinear1(v4 C)
{
    v4 Result;

    real32 Inv255 = 1.0f / 255.0f;

    Result.r = Square(Inv255*C.r);
    Result.g = Square(Inv255*C.g);
    Result.b = Square(Inv255*C.b);
    Result.a = Inv255*C.a;

    return(Result);
}

inline v4
Linear1ToSRGB255(v4 C)
{
    v4 Result;

    real32 One255 = 255.0f;

    Result.r = One255*SquareRoot(C.r);
    Result.g = One255*SquareRoot(C.g);
    Result.b = One255*SquareRoot(C.b);
    Result.a = One255*C.a;

    return(Result);
}

internal void
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, real32 R, real32 G, real32 B, real32 A = 1.0f)
{
    int32_t MinX = RoundReal32ToInt32(vMin.x);
    int32_t MinY = RoundReal32ToInt32(vMin.y);
    int32_t MaxX = RoundReal32ToInt32(vMax.x);
    int32_t MaxY = RoundReal32ToInt32(vMax.y);

    if(MinX < 0)
    {
        MinX = 0;
    }

    if(MinY < 0)
    {
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32_t Color = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
                      (RoundReal32ToUInt32(R * 255.0f) << 16) |
                      (RoundReal32ToUInt32(G * 255.0f) << 8) |
                      (RoundReal32ToUInt32(B * 255.0f) << 0));

    uint8_t *Row = ((uint8_t *)Buffer->Memory +
                    MinX*BITMAP_BYTES_PER_PIXEL +
                    MinY*Buffer->Pitch);
    for(int Y = MinY; Y < MaxY; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
    }
}

internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color, loaded_bitmap *Texture)
{
    // Premultiply color up front
    Color.rgb *= Color.a;

    real32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    real32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);

    uint32_t Color32 = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
                        (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
                        (RoundReal32ToUInt32(Color.g * 255.0f) << 8) |
                        (RoundReal32ToUInt32(Color.b * 255.0f) << 0));

    int WidthMax = (Buffer->Width - 1);
    int HeightMax = (Buffer->Height - 1);

    int XMin = WidthMax;
    int XMax = 0;
    int YMin = HeightMax;
    int YMax = 0;

    v2 P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};
    for(int PIndex = 0; PIndex < ArrayCount(P); ++PIndex)
    {
        v2 TestP = P[PIndex];
        int FloorX = FloorReal32ToInt32(TestP.x);
        int CeilX = CeilReal32ToInt32(TestP.x);
        int FloorY = FloorReal32ToInt32(TestP.y);
        int CeilY = CeilReal32ToInt32(TestP.y);

        if(XMin > FloorX) {XMin = FloorX;}
        if(YMin > FloorY) {YMin = FloorY;}
        if(XMax < CeilX) {XMax = CeilX;}
        if(YMax < CeilY) {YMax = CeilY;}
    }

    if(XMin < 0) {XMin = 0;}
    if(YMin < 0) {YMin = 0;}
    if(XMax > WidthMax) {XMax = WidthMax;}
    if(YMax > HeightMax) {YMax = HeightMax;}

    uint8_t *Row = ((uint8_t *)Buffer->Memory + XMin*BITMAP_BYTES_PER_PIXEL + YMin*Buffer->Pitch);
    for(int Y = YMin; Y <= YMax; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = XMin; X <= XMax; ++X)
        {
#if 1
            v2 PixelP = V2i(X, Y);
            v2 d = PixelP - Origin;

            // TODO: Implement Perp dot product.
            // TODO: Simpler Origin.
            real32 Edge0 = Inner(d, -Perp(XAxis));
            real32 Edge1 = Inner(d - XAxis, -Perp(YAxis));
            real32 Edge2 = Inner(d - XAxis - YAxis, Perp(XAxis));
            real32 Edge3 = Inner(d - YAxis, Perp(YAxis));

            if((Edge0 < 0) && (Edge1 < 0) &&
               (Edge2 < 0) && (Edge3 < 0))
            {
                real32 U = InvXAxisLengthSq*Inner(d, XAxis);
                real32 V = InvYAxisLengthSq*Inner(d, YAxis);

                Assert((U >= 0.0f) && (U <= 1.0f));
                Assert((V >= 0.0f) && (V <= 1.0f));

                real32 tX = ((U*(real32)(Texture->Width - 2)));
                real32 tY = ((V*(real32)(Texture->Height - 2)));

                int32_t X = (int32_t)tX;
                int32_t Y = (int32_t)tY;

                real32 fX = tX - (real32)X;
                real32 fY = tY - (real32)Y;

#if 0
                Assert((X >= 0) && (X < Texture->Width));
                Assert((Y >= 0) && (Y < Texture->Height));
#endif

                uint8_t *TexelPtr = ((uint8_t *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32_t);
                uint32_t TexelPtrA = *(uint32_t *)(TexelPtr);
                uint32_t TexelPtrB = *(uint32_t *)(TexelPtr + sizeof(uint32_t));
                uint32_t TexelPtrC = *(uint32_t *)(TexelPtr + Texture->Pitch);
                uint32_t TexelPtrD = *(uint32_t *)(TexelPtr + Texture->Pitch + sizeof(uint32_t));

                v4 TexelA = {(real32)((TexelPtrA >> 16) & 0xFF),
                             (real32)((TexelPtrA >> 8) & 0xFF),
                             (real32)((TexelPtrA >> 0) & 0xFF),
                             (real32)((TexelPtrA >> 24) & 0xFF)};
                v4 TexelB = {(real32)((TexelPtrB >> 16) & 0xFF),
                             (real32)((TexelPtrB >> 8) & 0xFF),
                             (real32)((TexelPtrB >> 0) & 0xFF),
                             (real32)((TexelPtrB >> 24) & 0xFF)};
                v4 TexelC = {(real32)((TexelPtrC >> 16) & 0xFF),
                             (real32)((TexelPtrC >> 8) & 0xFF),
                             (real32)((TexelPtrC >> 0) & 0xFF),
                             (real32)((TexelPtrC >> 24) & 0xFF)};
                v4 TexelD = {(real32)((TexelPtrD >> 16) & 0xFF),
                             (real32)((TexelPtrD >> 8) & 0xFF),
                             (real32)((TexelPtrD >> 0) & 0xFF),
                             (real32)((TexelPtrD >> 24) & 0xFF)};

                // Go from sRGB to "linear" brightness space.
                TexelA = SRGB255ToLinear1(TexelA);
                TexelB = SRGB255ToLinear1(TexelB);
                TexelC = SRGB255ToLinear1(TexelC);
                TexelD = SRGB255ToLinear1(TexelD);

                v4 Texel = Lerp(Lerp(TexelA, fX, TexelB),
                                fY,
                                Lerp(TexelC, fX, TexelD));

                Texel = Hadamard(Texel, Color);

                v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
                           (real32)((*Pixel >> 8) & 0xFF),
                           (real32)((*Pixel >> 0) & 0xFF),
                           (real32)((*Pixel>> 24) & 0xFF)};

                // Go from sRGB to "linear" brightness space.
                Dest = SRGB255ToLinear1(Dest);

                v4 Blended = (1.0f-Texel.a)*Dest + Texel;

                // Go from "linear" brightness space to sRGB.
                v4 Blended255 = Linear1ToSRGB255(Blended);

                *Pixel = (((uint32_t)(Blended255.a + 0.5f) << 24) |
                          ((uint32_t)(Blended255.r + 0.5f) << 16) |
                          ((uint32_t)(Blended255.g + 0.5f) << 8) |
                          ((uint32_t)(Blended255.b + 0.5f) << 0));
            }
#else
            *Pixel = Color32;
#endif

            ++Pixel;
        }

        Row += Buffer->Pitch;
    }
}

internal void
DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
           real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
{
    int32_t MinX = RoundReal32ToInt32(RealX);
    int32_t MinY = RoundReal32ToInt32(RealY);
    int32_t MaxX = MinX + Bitmap->Width;
    int32_t MaxY = MinY + Bitmap->Height;

    int32_t SourceOffsetX = 0;
    if(MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32_t SourceOffsetY = 0;
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8_t *SourceRow = (uint8_t *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL*SourceOffsetX;
    uint8_t *DestRow = ((uint8_t *)Buffer->Memory +
                        MinX*BITMAP_BYTES_PER_PIXEL +
                        MinY*Buffer->Pitch);

    for(int32_t Y = MinY; Y < MaxY; ++Y)
    {
        uint32_t *Dest = (uint32_t *)DestRow;
        uint32_t *Source = (uint32_t *)SourceRow;
        for(int32_t X = MinX; X < MaxX; ++X)
        {

            v4 Texel = {(real32)((*Source >> 16) & 0xFF),
                        (real32)((*Source >> 8) & 0xFF),
                        (real32)((*Source >> 0) & 0xFF),
                        (real32)((*Source >> 24) & 0xFF)};

            Texel = SRGB255ToLinear1(Texel);

            Texel *= CAlpha;

            v4 D = {(real32)((*Dest >> 16) & 0xFF),
                    (real32)((*Dest >> 8) & 0xFF),
                    (real32)((*Dest >> 0) & 0xFF),
                    (real32)((*Dest >> 24) & 0xFF)};

            D = SRGB255ToLinear1(D);

            v4 Result = (1.0f-Texel.a)*D + Texel;

            Result = Linear1ToSRGB255(Result);

            *Dest = (((uint32_t)(Result.a + 0.5f) << 24) |
                     ((uint32_t)(Result.r + 0.5f) << 16) |
                     ((uint32_t)(Result.g + 0.5f) << 8) |
                     ((uint32_t)(Result.b + 0.5f) << 0));

            ++Dest;
            ++Source;
        }

        DestRow += Buffer->Pitch;
        SourceRow += Bitmap->Pitch;
    }
}

internal void
DrawRectangleOutline(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v3 Color, real32 R = 2.0f)
{
    // For the top and bottom.
    DrawRectangle(Buffer, V2(vMin.x - R, vMin.y - R), V2(vMax.x + R, vMin.y + R), Color.r, Color.g, Color.b);
    DrawRectangle(Buffer, V2(vMin.x - R, vMax.y - R), V2(vMax.x + R, vMax.y + R), Color.r, Color.g, Color.b);

    // For the left and right.
    DrawRectangle(Buffer, V2(vMin.x - R, vMin.y - R), V2(vMin.x + R, vMax.y + R), Color.r, Color.g, Color.b);
    DrawRectangle(Buffer, V2(vMax.x - R, vMin.y - R), V2(vMax.x + R, vMax.y + R), Color.r, Color.g, Color.b);
}

internal void
DrawMatte(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
           real32 RealX, real32 RealY, real32 CAlpha = 1.0f)
{
    int32_t MinX = RoundReal32ToInt32(RealX);
    int32_t MinY = RoundReal32ToInt32(RealY);
    int32_t MaxX = MinX + Bitmap->Width;
    int32_t MaxY = MinY + Bitmap->Height;

    int32_t SourceOffsetX = 0;
    if(MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32_t SourceOffsetY = 0;
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint8_t *SourceRow = (uint8_t *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch + BITMAP_BYTES_PER_PIXEL*SourceOffsetX;
    uint8_t *DestRow = ((uint8_t *)Buffer->Memory +
                        MinX*BITMAP_BYTES_PER_PIXEL +
                        MinY*Buffer->Pitch);

    for(int32_t Y = MinY; Y < MaxY; ++Y)
    {
        uint32_t *Dest = (uint32_t *)DestRow;
        uint32_t *Source = (uint32_t *)SourceRow;
        for(int32_t X = MinX; X < MaxX; ++X)
        {
            real32 SA = (real32)((*Source >> 24) & 0xFF);
            real32 RSA = (SA / 255.0f) * CAlpha;
            real32 SR = CAlpha*(real32)((*Source >> 16) & 0xFF);
            real32 SG = CAlpha*(real32)((*Source >> 8) & 0xFF);
            real32 SB = CAlpha*(real32)((*Source >> 0) & 0xFF);

            real32 DA = (real32)((*Dest >> 24) & 0xFF);
            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >> 8) & 0xFF);
            real32 DB = (real32)((*Dest >> 0) & 0xFF);
            real32 RDA = (DA / 255.0f);

            real32 InvRSA = (1.0f-RSA);
            real32 A = InvRSA*DA;
            real32 R = InvRSA*DR;
            real32 G = InvRSA*DG;
            real32 B = InvRSA*DB;

            *Dest = (((uint32_t)(A + 0.5f) << 24) |
                     ((uint32_t)(R + 0.5f) << 16) |
                     ((uint32_t)(G + 0.5f) << 8) |
                     ((uint32_t)(B + 0.5f) << 0));

            ++Dest;
            ++Source;
        }

        DestRow += Buffer->Pitch;
        SourceRow += Bitmap->Pitch;
    }
}

inline v2 GetRenderEntityBasisP(render_group *RenderGroup, render_entity_basis *EntityBasis, v2 ScreenCentre)
{
    v3 EntityBaseP = EntityBasis->Basis->P;
    real32 ZFudge = (1.0f + 0.1f*(EntityBaseP.z + EntityBasis->OffsetZ));

    real32 EntityGroundPointX = ScreenCentre.x + RenderGroup->MetresToPixels*ZFudge*EntityBaseP.x;
    real32 EntityGroundPointY = ScreenCentre.y - RenderGroup->MetresToPixels*ZFudge*EntityBaseP.y;
    real32 EntityZ = -RenderGroup->MetresToPixels*EntityBaseP.z;

    v2 Centre = {EntityGroundPointX + EntityBasis->Offset.x,
                 EntityGroundPointY + EntityBasis->Offset.y + EntityBasis->EntityZC*EntityZ};
    return(Centre);
}

internal void
RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
{
    v2 ScreenCentre = {0.5f*(real32)OutputTarget->Width, 0.5f*(real32)OutputTarget->Height};

    for(uint32_t BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->PushBufferBase + BaseAddress);
        BaseAddress += sizeof(*Header);

        void *Data = (uint8_t *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupEntryType_render_entry_clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)Data;

                DrawRectangle(OutputTarget, V2(0.0f, 0.0f), V2((real32)OutputTarget->Width, (real32)OutputTarget->Height),
                              Entry->Color.r, Entry->Color.g, Entry->Color.b, Entry->Color.a);

                BaseAddress += sizeof(*Entry);
            } break;

            case RenderGroupEntryType_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCentre);
                Assert(Entry->Bitmap);
                DrawBitmap(OutputTarget, Entry->Bitmap, P.x, P.y, Entry->A);

                BaseAddress += sizeof(*Entry);
            } break;

            case RenderGroupEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
                v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCentre);
                DrawRectangle(OutputTarget, P, P + Entry->Dim, Entry->R, Entry->G, Entry->B);

                BaseAddress += sizeof(*Entry);
            } break;

            case RenderGroupEntryType_render_entry_coordinate_system:
            {
                render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;

                v2 vMax = (Entry->Origin + Entry->XAxis + Entry->YAxis);
                DrawRectangleSlowly(OutputTarget,
                                    Entry->Origin,
                                    Entry->XAxis,
                                    Entry->YAxis,
                                    Entry->Color,
                                    Entry->Texture);

                v4 Color = {1, 1, 0, 1};
                v2 Dim = {2, 2};
                v2 P = Entry->Origin;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);

                P = Entry->Origin + Entry->XAxis;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);

                P = Entry->Origin + Entry->YAxis;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);

                DrawRectangle(OutputTarget, vMax - Dim, vMax + Dim, Color.r, Color.g, Color.b);

#if 0
                for(uint32_t PIndex = 0; PIndex < ArrayCount(Entry->Points); ++PIndex)
                {
                    v2 P = Entry->Points[PIndex];
                    P = Entry->Origin + P.x*Entry->XAxis + P.y*Entry->YAxis;
                    DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);
                }
#endif

                BaseAddress += sizeof(*Entry);
            } break;

            InvalidDefaultCase;
        }
    }
}

internal render_group *
AllocateRenderGroup(memory_arena *Arena, uint32_t MaxPushBufferSize, real32 MetresToPixels)
{
    render_group *Result = PushStruct(Arena, render_group);
    Result->PushBufferBase = (uint8_t *)PushSize(Arena, MaxPushBufferSize);

    Result->DefaultBasis = PushStruct(Arena, render_basis);
    Result->DefaultBasis->P = V3(0, 0, 0);
    Result->MetresToPixels = MetresToPixels;

    Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferSize = 0;

    return(Result);
}

#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)
inline void *
PushRenderElement_(render_group *Group, uint32_t Size, render_group_entry_type Type)
{
    void *Result = 0;

    Size += sizeof(render_group_entry_header);

    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
        Header->Type = Type;
        Result = (uint8_t *)Header + sizeof(*Header);
        Group->PushBufferSize += Size;
    }
    else
    {
        InvalidCodePath;
    }

    return(Result);
}

inline void
PushPiece(render_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC)
{
    render_entry_bitmap *Piece = PushRenderElement(Group, render_entry_bitmap);
    if(Piece)
    {
        Piece->EntityBasis.Basis = Group->DefaultBasis;
        Piece->Bitmap = Bitmap;
        Piece->EntityBasis.Offset = Group->MetresToPixels*V2(Offset.x, -Offset.y) - Align;
        Piece->EntityBasis.OffsetZ = OffsetZ;
        Piece->EntityBasis.EntityZC = EntityZC;
        Piece->R = Color.r;
        Piece->G = Color.g;
        Piece->B = Color.b;
        Piece->A = Color.a;
    }
}

inline void
PushBitmap(render_group *Group, loaded_bitmap *Bitmap,
           v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f)
{
    PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0, 0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZC);
}

inline void
PushRect(render_group *Group, v2 Offset, real32 OffsetZ,
         v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
    render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
    if(Piece)
    {
        v2 HalfDim = 0.5f*Group->MetresToPixels*Dim;

        Piece->EntityBasis.Basis = Group->DefaultBasis;
        Piece->EntityBasis.Offset = Group->MetresToPixels*V2(Offset.x, -Offset.y) - HalfDim;
        Piece->EntityBasis.OffsetZ = OffsetZ;
        Piece->EntityBasis.EntityZC = EntityZC;
        Piece->R = Color.r;
        Piece->G = Color.g;
        Piece->B = Color.b;
        Piece->A = Color.a;
        Piece->Dim = Group->MetresToPixels*Dim;
    }
}

inline void
PushRectOutline(render_group *Group, v2 Offset, real32 OffsetZ,
                v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
    real32 Thickness = 0.1f;

    // Top and Bottom.
    PushRect(Group, Offset - V2(0, 0.5f*Dim.y), OffsetZ, V2(Dim.x, Thickness), Color, EntityZC);
    PushRect(Group, Offset + V2(0, 0.5f*Dim.y), OffsetZ, V2(Dim.x, Thickness), Color, EntityZC);

    // Left and Right.
    PushRect(Group, Offset - V2(0.5f*Dim.x, 0), OffsetZ, V2(Thickness, Dim.y), Color, EntityZC);
    PushRect(Group, Offset + V2(0.5f*Dim.x, 0), OffsetZ, V2(Thickness, Dim.y), Color, EntityZC);
}

inline void
Clear(render_group *Group, v4 Color)
{
    render_entry_clear *Entry = PushRenderElement(Group, render_entry_clear);
    if(Entry)
    {
        Entry->Color = Color;
    }
}

inline render_entry_coordinate_system *
CoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color, loaded_bitmap *Texture)
{
    render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
    if(Entry)
    {
        Entry->Origin = Origin;
        Entry->XAxis = XAxis;
        Entry->YAxis = YAxis;
        Entry->Color = Color;
        Entry->Texture = Texture;
    }

    return(Entry);
}

#if !defined(HANDMADE_RENDER_GROUP_H)

struct render_basis
{
    v3 P;
};

struct entity_visible_piece
{
    render_basis *Basis;
    loaded_bitmap *Bitmap;
    v2 Offset;
    real32 OffsetZ;
    real32 EntityZC;

    real32 R, G, B, A;
    v2 Dim;
};

struct entity_visible_piece_group
{
    render_basis *DefaultBasis;
    game_state *GameState;
    uint32_t PieceCount;
    entity_visible_piece Pieces[4096];
};

inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC)
{
    Assert(Group->PieceCount < ArrayCount(Group->Pieces));
    entity_visible_piece *Piece = Group->Pieces + Group->PieceCount++;
    Piece->Basis = Group->DefaultBasis;
    Piece->Bitmap = Bitmap;
    Piece->Offset = Group->GameState->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
    Piece->OffsetZ = OffsetZ;
    Piece->EntityZC = EntityZC;
    Piece->R = Color.R;
    Piece->G = Color.G;
    Piece->B = Color.B;
    Piece->A = Color.A;
    Piece->Dim = Dim;
}

inline void
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
           v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f)
{
    PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0, 0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZC);
}

inline void
PushRect(entity_visible_piece_group *Group, v2 Offset, real32 OffsetZ,
         v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
    PushPiece(Group, 0, Offset, OffsetZ, V2(0, 0), Dim, Color, EntityZC);
}

inline void
PushRectOutline(entity_visible_piece_group *Group, v2 Offset, real32 OffsetZ,
                v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
    real32 Thickness = 0.1f;

    // Top and Bottom.
    PushPiece(Group, 0, Offset - V2(0, 0.5f*Dim.Y), OffsetZ, V2(0, 0), V2(Dim.X, Thickness), Color, EntityZC);
    PushPiece(Group, 0, Offset + V2(0, 0.5f*Dim.Y), OffsetZ, V2(0, 0), V2(Dim.X, Thickness), Color, EntityZC);

    // Left and Right.
    PushPiece(Group, 0, Offset - V2(0.5f*Dim.X, 0), OffsetZ, V2(0, 0), V2(Thickness, Dim.Y), Color, EntityZC);
    PushPiece(Group, 0, Offset + V2(0.5f*Dim.X, 0), OffsetZ, V2(0, 0), V2(Thickness, Dim.Y), Color, EntityZC);
}

#define HANDMAE_RENDER_GROUP_H
#endif

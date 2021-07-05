#include "game.h"
#include "game_world.cpp"
#include "game_random.h"

internal void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16_t ToneVolume = 3000; 
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples; 
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 0
        real32 SineValue = sinf(GameState->tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
#else
        int16_t SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

#if 0
        GameState->tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
        if(GameState->tSine > 2.0f*Pi32)
        {
            GameState->tSine -= 2.0f*Pi32;
        }
#endif
    }
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, real32 R, real32 G, real32 B)
{
    int32_t MinX = RoundReal32ToInt32(vMin.X);
    int32_t MinY = RoundReal32ToInt32(vMin.Y);
    int32_t MaxX = RoundReal32ToInt32(vMax.X);
    int32_t MaxY = RoundReal32ToInt32(vMax.Y);

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

    uint32_t Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) |
                      (RoundReal32ToUInt32(G * 255.0f) <<  8) |
                      (RoundReal32ToUInt32(B * 255.0f) <<  0));

    uint8_t *Row = ((uint8_t *)Buffer->Memory +
                    MinX*Buffer->BytesPerPixel +
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
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap,
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

    uint32_t *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
    SourceRow += -SourceOffsetY*Bitmap->Width + SourceOffsetX;
    uint8_t *DestRow = ((uint8_t *)Buffer->Memory +
                        MinX*Buffer->BytesPerPixel +
                        MinY*Buffer->Pitch);

    for(int32_t Y = MinY; Y < MaxY; ++Y)
    {
        uint32_t *Dest = (uint32_t *)DestRow;
        uint32_t *Source = SourceRow;
        for(int32_t X = MinX; X < MaxX; ++X)
        {
            real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
            A *= CAlpha;

            real32 SR = (real32)((*Source >> 16) & 0xFF);
            real32 SG = (real32)((*Source >>  8) & 0xFF);
            real32 SB = (real32)((*Source >>  0) & 0xFF);

            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >>  8) & 0xFF);
            real32 DB = (real32)((*Dest >>  0) & 0xFF);

            real32 R = (1.0f-A)*DR + A*SR;
            real32 G = (1.0f-A)*DG + A*SG;
            real32 B = (1.0f-A)*DB + A*SB;

            *Dest = (((uint32_t)(R + 0.5f) << 16) |
                     ((uint32_t)(G + 0.5f) <<  8) |
                     ((uint32_t)(B + 0.5f) <<  0));

            ++Dest;
            ++Source;
        }

        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}

#pragma pack(push, 1)
struct bitmap_header
{
    uint16_t FileType;
    uint32_t FileSize;
    uint16_t Reserved1;
    uint16_t Reserved2;
    uint32_t BitmapOffset;
    uint32_t Size;
    int32_t  Width;
    int32_t  Height;
    uint16_t Planes;
    uint16_t BitsPerPixel;
    uint32_t Compression;
    uint32_t SizeOfBitmap;
    int32_t  HorzResolution;
    int32_t  VertResolution;
    uint32_t ColorsUsed;
    uint32_t ColorsImportant;

    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width  = Header->Width;
        Result.Height = Header->Height;

        Assert(Header->Compression == 3);
        uint32_t RedMask = Header->RedMask;
        uint32_t GreenMask = Header->GreenMask;
        uint32_t BlueMask  = Header->BlueMask;
        uint32_t AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedScan   = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan  = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

        int32_t RedShift   = 16 - (int32_t)RedScan.Index;
        int32_t GreenShift =  8 - (int32_t)GreenScan.Index;
        int32_t BlueShift  =  0 - (int32_t)BlueScan.Index;
        int32_t AlphaShift = 24 - (int32_t)AlphaScan.Index;

        uint32_t *SourceDest = Pixels;
        for(int32_t Y = 0; Y < Header->Height; ++Y)
        {
            for(int32_t X = 0; X < Header->Width; ++X)
            {
                uint32_t C = *SourceDest;

                *SourceDest++ = (RotateLeft(C & RedMask, RedShift) |
                                 RotateLeft(C & GreenMask, GreenShift) |
                                 RotateLeft(C & BlueMask, BlueShift) |
                                 RotateLeft(C & AlphaMask, AlphaShift));
            }
        }
    }

    return(Result);
}

inline low_entity *
GetLowEntity(game_state *GameState, uint32_t Index)
{
    low_entity *Result = 0;

    if((Index > 0) && (Index < GameState->LowEntityCount))
    {
        Result = GameState->LowEntities + Index;
    }

    return(Result);
}

inline v2
GetCameraSpaceP(game_state *GameState, low_entity *EntityLow)
{
    world_difference Diff = Subtract(GameState->World, &EntityLow->P, &GameState->CameraP);
    v2 Result = Diff.dXY;

    return(Result);
}

inline high_entity *
MakeEntityHighFrequency(game_state *GameState, low_entity *EntityLow, uint32_t LowIndex, v2 CameraSpaceP)
{
    high_entity *EntityHigh = 0;

    Assert(EntityLow->HighEntityIndex == 0);
    if(EntityLow->HighEntityIndex == 0)
    {
        if(GameState->HighEntityCount < ArrayCount(GameState->HighEntities_))
        {
            uint32_t HighIndex = GameState->HighEntityCount++;
            EntityHigh = GameState->HighEntities_ + HighIndex;

            EntityHigh->P = CameraSpaceP;
            EntityHigh->dP = V2(0, 0);
            EntityHigh->ChunkZ = EntityLow->P.ChunkZ;
            EntityHigh->FacingDirection = 0;
            EntityHigh->LowEntityIndex = LowIndex;

            EntityLow->HighEntityIndex = HighIndex;
        }
        else
        {
            InvalidCodePath;
        }
    }

    return(EntityHigh);
}

inline high_entity *
MakeEntityHighFrequency(game_state *GameState, uint32_t LowIndex)
{
    high_entity *EntityHigh = 0;

    low_entity *EntityLow = &GameState->LowEntities[LowIndex];
    if(EntityLow->HighEntityIndex)
    {
        EntityHigh = GameState->HighEntities_ + EntityLow->HighEntityIndex;
    }
    else
    {
        v2 CameraSpaceP = GetCameraSpaceP(GameState, EntityLow);
        EntityHigh = MakeEntityHighFrequency(GameState, EntityLow, LowIndex, CameraSpaceP);
    }

    return(EntityHigh);
}

inline entity
ForceEntityIntoHigh(game_state *GameState, uint32_t LowIndex)
{
    entity Result = {};

    if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount))
    {
        Result.LowIndex = LowIndex;
        Result.Low = GameState->LowEntities + LowIndex;
        Result.High = MakeEntityHighFrequency(GameState, LowIndex);
    }

    return(Result);
}

inline void
MakeEntityLowFrequency(game_state *GameState, uint32_t LowIndex)
{
    low_entity *EntityLow = &GameState->LowEntities[LowIndex];
    uint32_t HighIndex = EntityLow->HighEntityIndex;
    if(HighIndex)
    {
        uint32_t LastHighIndex = GameState->HighEntityCount - 1;
        if(HighIndex != LastHighIndex)
        {
            high_entity *LastEntity = GameState->HighEntities_ + LastHighIndex;
            high_entity *DelEntity = GameState->HighEntities_ + HighIndex;

            *DelEntity = *LastEntity;
            GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
        }
        --GameState->HighEntityCount;
        EntityLow->HighEntityIndex = 0;
    }
}

inline bool32
ValidateEntityPairs(game_state *GameState)
{
    bool32 Valid = true;

    for(uint32_t HighEntityIndex = 1;
        HighEntityIndex < GameState->HighEntityCount;
        ++HighEntityIndex)
    {
        high_entity *High = GameState->HighEntities_ + HighEntityIndex;
        Valid = Valid && 
            (GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
    }
    
    return(Valid);
}

inline void
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 HighFrequencyBounds)
{
    for(uint32_t HighEntityIndex = 1;
        HighEntityIndex < GameState->HighEntityCount;
        )
    {
        high_entity *High = GameState->HighEntities_ + HighEntityIndex;

        High->P += Offset;
        if(IsInRectangle(HighFrequencyBounds, High->P))
        {
            ++HighEntityIndex;
        }
        else
        {
            Assert(GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
            MakeEntityLowFrequency(GameState, High->LowEntityIndex);
        }
    }
}

struct add_low_entity_result
{
    low_entity *Low;
    uint32_t LowIndex;
};
internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type Type, world_position *P)
{
    Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
    uint32_t EntityIndex = GameState->LowEntityCount++;

    low_entity *EntityLow = GameState->LowEntities + EntityIndex;
    *EntityLow = {};
    EntityLow->Type = Type;

    if(P)
    {
        EntityLow->P = *P;
        ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, 0, P);
    }

    add_low_entity_result Result;
    Result.Low = EntityLow;
    Result.LowIndex = EntityIndex;
    
    return(Result);
}

internal add_low_entity_result
AddWall(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Wall, &P);

    Entity.Low->Height = GameState->World->TileSideInMeters;
    Entity.Low->Width = Entity.Low->Height;
    Entity.Low->Collides = true;

    return(Entity);
}

internal add_low_entity_result
AddPlayer(game_state *GameState)
{
    world_position P = GameState->CameraP;
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, &P);

    Entity.Low->HitPointMax = 3;
    Entity.Low->HitPoint[2].FilledAmount = HIT_POINT_SUB_COUNT;
    Entity.Low->HitPoint[0] = Entity.Low->HitPoint[1] = Entity.Low->HitPoint[2];

    Entity.Low->Height = 0.5f;
    Entity.Low->Width = 1.0f;
    Entity.Low->Collides = true;

    if(GameState->CameraFollowingEntityIndex == 0)
    {
        GameState->CameraFollowingEntityIndex = Entity.LowIndex;
    }

    return(Entity);
}

internal add_low_entity_result
AddMonster(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Monster, &P);

    Entity.Low->Height = 0.5f;
    Entity.Low->Width = 1.0f;
    Entity.Low->Collides = true;

    return(Entity);
}

internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, &P);

    Entity.Low->Height = 0.5f;
    Entity.Low->Width = 1.0f;
    Entity.Low->Collides = true;

    return(Entity);
}

internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
         real32 *tMin, real32 MinY, real32 MaxY)
{
    bool32 Hit = false;

    real32 tEpsilon = 0.001f;
    if(PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        real32 Y = RelY + tResult*PlayerDeltaY;
        if((tResult >= 0.0f) && (*tMin > tResult))
        {
            if((Y >= MinY) && (Y <= MaxY))
            {
                *tMin = Maximum(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }

    return(Hit);
}

internal void
MoveEntity(game_state *GameState, entity Entity, real32 dt, v2 ddP)
{
    world *World = GameState->World;

    real32 ddPLength = LengthSq(ddP);
    if(ddPLength > 1.0f)
    {
        ddP *= (1.0f / SquareRoot(ddPLength));
    }

    real32 PlayerSpeed = 50.0f; // m/s^2
    ddP *= PlayerSpeed;
    
    //TODO: Diagonal will be faster! Fix with vectors
    ddP += -9.0f*Entity.High->dP;

    v2 OldPlayerP = Entity.High->P;
    v2 PlayerDelta = (0.5f*ddP*Square(dt) +
                      Entity.High->dP*dt);
    Entity.High->dP = ddP*dt + Entity.High->dP;
    v2 NewPlayerP = OldPlayerP + PlayerDelta;

    /*
    uint32_t MinTileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
    uint32_t MinTileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
    uint32_t MaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
    uint32_t MaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

    uint32_t EntityTileWidth  = CeilReal32ToInt32(Entity.High->Width / World->TileSideInMeters);
    uint32_t EntityTileHeight = CeilReal32ToInt32(Entity.High->Height / World->TileSideInMeters);

    MinTileX -= EntityTileWidth;
    MinTileY -= EntityTileHeight;
    MaxTileX += EntityTileWidth;
    MaxTileY += EntityTileHeight;

    uint32_t AbsTileZ = Entity.High->P.AbsTileZ;
    */

    for(uint32_t Iteration = 0; Iteration < 4; ++Iteration)
    {
        real32 tMin = 1.0f;
        v2 WallNormal = {};
        uint32_t HitHighEntityIndex = 0;

        v2 DesiredPosition = Entity.High->P + PlayerDelta;

        for(uint32_t TestHighEntityIndex = 1; TestHighEntityIndex < GameState->HighEntityCount; ++TestHighEntityIndex)
        {
            if(TestHighEntityIndex != Entity.Low->HighEntityIndex)
            {
                entity TestEntity;
                TestEntity.High = GameState->HighEntities_ + TestHighEntityIndex;
                TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
                TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;
                if(TestEntity.Low->Collides)
                {
                    real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
                    real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;

                    v2 MinCorner = -0.5f*V2(DiameterW, DiameterH);
                    v2 MaxCorner = 0.5f*V2(DiameterW, DiameterH);

                    v2 Rel = Entity.High->P - TestEntity.High->P;

                    if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                                &tMin, MinCorner.Y, MaxCorner.Y))
                    {
                        WallNormal = V2(-1, 0);
                        HitHighEntityIndex = TestHighEntityIndex;
                    }

                    if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                                &tMin, MinCorner.Y, MaxCorner.Y))
                    {
                        WallNormal = V2(1, 0);
                        HitHighEntityIndex = TestHighEntityIndex;
                    }

                    if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                                &tMin, MinCorner.X, MaxCorner.X))
                    {
                        WallNormal = V2(0, -1);
                        HitHighEntityIndex = TestHighEntityIndex;
                    }

                    if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                                &tMin, MinCorner.X, MaxCorner.X))
                    {
                        WallNormal = V2(0, 1);
                        HitHighEntityIndex = TestHighEntityIndex;
                    }
                }
            }
        }

        Entity.High->P += tMin*PlayerDelta;
        if(HitHighEntityIndex)
        {
            Entity.High->dP = Entity.High->dP - 1*Inner(Entity.High->dP, WallNormal)*WallNormal;
            PlayerDelta = DesiredPosition - Entity.High->P;
            PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;

            high_entity *HitHigh = GameState->HighEntities_ + HitHighEntityIndex;
            low_entity *HitLow = GameState->LowEntities + HitHigh->LowEntityIndex;
            //Entity.High->AbsTileZ += HitLow->dAbsTileZ;
        }
        else
        {
            break;
        }
    }

    if((Entity.High->dP.X == 0.0f) && (Entity.High->dP.Y == 0.0f))
    {
        // NOTE: Leave facing direction as whatever it was.
    }
    else if(AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y))
    {
        if(Entity.High->dP.X > 0)
        {
            Entity.High->FacingDirection = 0;
        }
        else
        {
            Entity.High->FacingDirection = 2;
        }
    }
    else
    {
        if(Entity.High->dP.Y > 0)
        {
            Entity.High->FacingDirection = 1;
        }
        else
        {
            Entity.High->FacingDirection = 3;
        }
    }

    world_position NewP = MapIntoChunkSpace(GameState->World, GameState->CameraP, Entity.High->P);

    ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity.LowIndex,
                         &Entity.Low->P, &NewP);
    Entity.Low->P = NewP;
}

internal void
SetCamera(game_state *GameState, world_position NewCameraP)
{
    world *World = GameState->World;

    Assert(ValidateEntityPairs(GameState));

    world_difference dCameraP = Subtract(World, &NewCameraP, &GameState->CameraP);
    GameState->CameraP = NewCameraP;

    uint32_t TileSpanX = 17*3;
    uint32_t TileSpanY = 9*3;
    rectangle2 CameraBounds = RectCenterDim(V2(0, 0),
                                            World->TileSideInMeters*V2((real32)TileSpanX,
                                                                         (real32)TileSpanY));
    v2 EntityOffsetForFrame = -dCameraP.dXY;
    OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

    Assert(ValidateEntityPairs(GameState));

    world_position MinChunkP = MapIntoChunkSpace(World, NewCameraP, GetMinCorner(CameraBounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, NewCameraP, GetMaxCorner(CameraBounds));
    for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
    {
        for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
        {
            world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, NewCameraP.ChunkZ);
            if(Chunk)
            {
                for(world_entity_block *Block = &Chunk->FirstBlock; Block; Block = Block->Next)
                {
                    for(uint32_t EntityIndexIndex = 0;
                        EntityIndexIndex < Block->EntityCount;
                        ++EntityIndexIndex)
                    {
                        uint32_t LowEntityIndex = Block->LowEntityIndex[EntityIndexIndex];
                        low_entity *Low = GameState->LowEntities + LowEntityIndex;
                        if(Low->HighEntityIndex == 0)
                        {
                            v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
                            if(IsInRectangle(CameraBounds, CameraSpaceP))
                            {
                                MakeEntityHighFrequency(GameState, Low, LowEntityIndex, CameraSpaceP);
                            }
                        }
                    }
                }
            }
        }
    }

    Assert(ValidateEntityPairs(GameState));
}

inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f)
{
    Assert(Group->PieceCount < ArrayCount(Group->Pieces));
    entity_visible_pieces *Piece = Group->Pieces + Group->PieceCount++;
    Piece->Bitmap = Bitmap;
    Piece->Offset = Offset - Align;
    Piece->OffsetZ = OffsetZ;
    Piece->A = Alpha;
    Piece->EntityZC = EntityZC;
}

inline entity
EntityFromHighIndex(game_state *GameState, uint32_t HighEntityIndex)
{
    entity Result = {};

    if(HighEntityIndex)
    {
        Assert(HighEntityIndex < ArrayCount(GameState->HighEntities_));
        Result.High = GameState->HighEntities_ + HighEntityIndex;
        Result.LowIndex = Result.High->LowEntityIndex;
        Result.Low = GameState->LowEntities + Result.LowIndex;
    }

    return(Result);
}

inline void
UpdateFamiliar(game_state *GameState, entity Entity, real32 dt)
{
    entity ClosestHero = {};
    real32 ClosestHeroDSq = Square(10.0f);
    for(uint32_t HighEntityIndex = 1;
        HighEntityIndex < GameState->HighEntityCount;
        ++HighEntityIndex)
    {
        entity TestEntity = EntityFromHighIndex(GameState, HighEntityIndex);

        if(TestEntity.Low->Type == EntityType_Hero)
        {
            real32 TestDSq = LengthSq(TestEntity.High->P - Entity.High->P);
            if(TestEntity.Low->Type == EntityType_Hero)
            {
                TestDSq *= 0.75f;
            }

            if(ClosestHeroDSq > TestDSq)
            {
                ClosestHero = TestEntity;
                ClosestHeroDSq = TestDSq;
            }
        }
    }

    v2 ddP = {};
    if(ClosestHero.High && (ClosestHeroDSq > Square(3.0f)))
    {
        real32 Acceleration = 0.5f;
        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
        ddP = OneOverLength*(ClosestHero.High->P - Entity.High->P);
    }
    
    MoveEntity(GameState, Entity, dt, ddP);
}

inline void
UpdateMonster(game_state *GameState, entity Entity, real32 dt)
{
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialised)
    {
        // NOTE: Reserve entity slot 0 for the null entity
        AddLowEntity(GameState, EntityType_Null, 0);
        GameState->HighEntityCount = 1;

        GameState->Backdrop =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
        GameState->Shadow =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");
        GameState->Tree =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tree00.bmp");

        hero_bitmaps *Bitmap;

        Bitmap = GameState->HeroBitmaps;
        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        InitialiseArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitialiseWorld(World, 1.4f);

        uint32_t RandomNumberIndex = 0;
        uint32_t TilesPerWidth  = 17;
        uint32_t TilesPerHeight = 9;

        uint32_t ScreenBaseX = 0;
        uint32_t ScreenBaseY = 0;
        uint32_t ScreenBaseZ = 0;
        uint32_t ScreenX  = ScreenBaseX;
        uint32_t ScreenY  = ScreenBaseY;
        uint32_t AbsTileZ = ScreenBaseZ;

        bool32 DoorLeft   = false;
        bool32 DoorRight  = false;
        bool32 DoorTop    = false;
        bool32 DoorBottom = false;
        bool32 DoorUp     = false;
        bool32 DoorDown   = false;
        for(uint32_t ScreenIndex = 0; ScreenIndex < 2000; ++ScreenIndex)
        {
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));

            uint32_t RandomChoice; 
//            if(DoorUp || DoorDown)
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            }
#if 0
            else
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            }
#endif

            bool32 CreatedZDoor = false;
            if(RandomChoice == 2)
            {
                CreatedZDoor = true;
                if(AbsTileZ == ScreenBaseZ)
                {
                    DoorUp = true;
                }
                else
                {
                    DoorDown = true;
                }
            }
            else if(RandomChoice == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

            for(uint32_t TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for(uint32_t TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32_t AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32_t AbsTileY = ScreenY*TilesPerHeight + TileY;

                    uint32_t TileValue = 1;
                    if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
                    {
                        TileValue = 2;
                    }

                    if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
                    {
                        TileValue = 2;
                    }

                    if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
                    {
                        TileValue = 2;
                    }

                    if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
                    {
                        TileValue = 2;
                    }

                    if((TileX == 10) && (TileY == 6))
                    {
                        if(DoorUp)
                        {
                            TileValue = 3;
                        }

                        if(DoorDown)
                        {
                            TileValue = 4;
                        }
                    }

                    if(TileValue == 2)
                    {
                        AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
                    }
                }
            }

            DoorLeft = DoorRight;
            DoorBottom = DoorTop;

            if(CreatedZDoor)
            {
                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else
            {
                DoorUp = false;
                DoorDown = false;
            }

            DoorRight = false;
            DoorTop = false;

            if(RandomChoice == 2)
            {
                if(AbsTileZ == ScreenBaseZ)
                {
                    AbsTileZ = ScreenBaseZ + 1;
                }
                else
                {
                    AbsTileZ = ScreenBaseZ;
                }
            }
            else if(RandomChoice == 1)
            {
                ScreenX += 1;
            }
            else
            {
                ScreenY += 1;
            }
        }

#if 0
        while(GameState->LowEntityCount < (ArrayCount(GameState->LowEntities) - 16))
        {
            uint32_t Coordinate = 1024 + GameState->LowEntityCount;
            AddWall(GameState, Coordinate, Coordinate, Coordinate);
        }
#endif

        world_position NewCameraP = {};
        uint32_t CameraTileX = ScreenBaseX * TilesPerWidth  + 17/2;
        uint32_t CameraTileY = ScreenBaseY * TilesPerHeight +  9/2;
        uint32_t CameraTileZ = ScreenBaseZ;
        NewCameraP = ChunkPositionFromTilePosition(GameState->World,
                                                   CameraTileX,
                                                   CameraTileY,
                                                   CameraTileZ);
        AddMonster(GameState, CameraTileX + 2, CameraTileY + 2, CameraTileZ);
        for(int FamiliarIndex = 0; FamiliarIndex < 1; ++FamiliarIndex)
        {
            int32_t FamiliarOffsetX = (RandomNumberTable[RandomNumberIndex++] % 10) - 7;
            int32_t FamiliarOffsetY = (RandomNumberTable[RandomNumberIndex++] % 10) - 3;
            if((FamiliarOffsetX != 0) || (FamiliarOffsetY != 0))
            {
                AddFamiliar(GameState, CameraTileX + FamiliarOffsetX, CameraTileY + FamiliarOffsetY, CameraTileZ);
            }
        }
        SetCamera(GameState, NewCameraP);

        Memory->IsInitialised = true;
    }

    world *World = GameState->World;

    int32_t TileSideInPixels = 60;
    real32 MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;

    real32 LowerLeftX = -(real32)TileSideInPixels/2;
    real32 LowerLeftY = (real32)Buffer->Height;

    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        uint32_t LowIndex = GameState->PlayerIndexForController[ControllerIndex];
        if(LowIndex == 0)
        {
            if(Controller->Start.EndedDown)
            {
                uint32_t EntityIndex = AddPlayer(GameState).LowIndex;
                GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
            }
        }
        else
        {
            entity ControllingEntity = ForceEntityIntoHigh(GameState, LowIndex);
            v2 ddP = {};

            if(Controller->IsAnalogue)
            {
                // Use analogue movement tuning
                ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
            }
            else
            {
                // Use digital movement tuning
                if(Controller->MoveUp.EndedDown)
                {
                    ddP.Y = 1.0f;
                }
                if(Controller->MoveDown.EndedDown)
                {
                    ddP.Y = -1.0f;
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    ddP.X = -1.0f;
                }
                if(Controller->MoveRight.EndedDown)
                {
                    ddP.X = 1.0f;
                }
            }

            if(Controller->ActionUp.EndedDown)
            {
                ControllingEntity.High->dZ = 3.0f;
            }

            MoveEntity(GameState, ControllingEntity, Input->dtForFrame, ddP);
        }
    }

    entity CameraFollowingEntity = ForceEntityIntoHigh(GameState, GameState->CameraFollowingEntityIndex);
    if(CameraFollowingEntity.High)
    {
        world_position NewCameraP = GameState->CameraP;

        NewCameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;

#if 0
        if(CameraFollowingEntity.High->P.X > (9.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileX += 17;
        }
        if(CameraFollowingEntity.High->P.X < -(9.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileX -= 17;
        }
        if(CameraFollowingEntity.High->P.Y > (5.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileY += 9;
        }
        if(CameraFollowingEntity.High->P.Y < -(5.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileY -= 9;
        }
#else
        NewCameraP = CameraFollowingEntity.Low->P;
#endif
        SetCamera(GameState, NewCameraP);
    }

#if 1
    DrawRectangle(Buffer, V2(0.0f, 0.0f), V2((real32)Buffer->Width, (real32)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else
    DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#endif

    real32 ScreenCentreX = 0.5f*(real32)Buffer->Width;
    real32 ScreenCentreY = 0.5f*(real32)Buffer->Height;

    entity_visible_piece_group PieceGroup;
    for(uint32_t HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex)
    {
        PieceGroup.PieceCount = 0;

        high_entity *HighEntity = GameState->HighEntities_ + HighEntityIndex;
        low_entity *LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

        entity Entity;
        Entity.LowIndex = HighEntity->LowEntityIndex;
        Entity.Low = LowEntity;
        Entity.High = HighEntity;

        real32 dt = Input->dtForFrame;
        
        real32 ShadowAlpha = 1.0f - 0.5f*HighEntity->Z;
        if(ShadowAlpha < 0)
        {
            ShadowAlpha = 0.0f;
        }

        hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[HighEntity->FacingDirection];

        switch(LowEntity->Type)
        {
            case EntityType_Hero:
            {
                PushPiece(&PieceGroup, &GameState->Shadow,  V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                PushPiece(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);
                PushPiece(&PieceGroup, &HeroBitmaps->Cape,  V2(0, 0), 0, HeroBitmaps->Align);
                PushPiece(&PieceGroup, &HeroBitmaps->Head,  V2(0, 0), 0, HeroBitmaps->Align);
            } break;

            case EntityType_Wall:
            {
                PushPiece(&PieceGroup, &GameState->Tree, V2(0, 0), 0, V2(40, 80));
            } break;

            case EntityType_Familiar:
            {
                UpdateFamiliar(GameState, Entity, dt);
                Entity.High->tBob += dt;
                if(Entity.High->tBob > (2.0f*Pi32))
                {
                    Entity.High->tBob -= (2.0f*Pi32);
                }
                real32 BobSin = Sin(2.0f*Entity.High->tBob);
                PushPiece(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align,
                          (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
                PushPiece(&PieceGroup, &HeroBitmaps->Head, V2(0, 0), 10.0f*BobSin, HeroBitmaps->Align);
            } break;

            case EntityType_Monster:
            {
                UpdateMonster(GameState, Entity, dt);
                PushPiece(&PieceGroup, &GameState->Shadow,  V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                PushPiece(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);
            } break;

            default:
            {
                InvalidCodePath;
            } break;
        }

        real32 ddZ = -9.8f;
        HighEntity->Z = 0.5f*ddZ*Square(dt) + HighEntity->dZ*dt + HighEntity->Z;
        HighEntity->dZ = ddZ*dt + HighEntity->dZ;
        if(HighEntity->Z < 0)
        {
            HighEntity->Z = 0;
        }
        
        real32 EntityGroundPointX = ScreenCentreX + MetersToPixels*HighEntity->P.X;
        real32 EntityGroundPointY = ScreenCentreY - MetersToPixels*HighEntity->P.Y;
        real32 EntityZ = -MetersToPixels*HighEntity->Z;
#if 0
        v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*LowEntity->Width,
                            PlayerGroundPointY - 0.5f*MetersToPixels*LowEntity->Height};
        v2 EntityWidthHeight = {LowEntity->Width, LowEntity->Height};
        DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + 0.9f*MetersToPixels*EntityWidthHeight,
                      1.0f, 1.0f, 0.0f);
#endif
        for(uint32_t PieceIndex = 0; PieceIndex < PieceGroup.PieceCount; ++PieceIndex)
        {
            entity_visible_pieces *Piece = PieceGroup.Pieces + PieceIndex;
            v2 Center = {EntityGroundPointX + Piece->Offset.X,
                         EntityGroundPointY + PiecePffset.Y + Piece->OffsetZ + Piece->EntityZC*EntityZ};
            if(Piece->Bitmap)
            {
                DrawBitmap(Buffer, Piece->Bitmap, Center.X, Center.Y, Piece->A);
            }
            else
            {
                v2 HalfDim = 0.5f*MetersToPixels*Piece->Dim;
                DrawRectangle(Buffer, Center - HalfDim, Center + HalDim, Piece->R, Piece->G, Piece->B);
            }
        }
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 400);
}

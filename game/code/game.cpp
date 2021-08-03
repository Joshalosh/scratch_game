#include "game.h"
#include "game_world.cpp"
#include "game_random.h"
#include "game_sim_region.cpp"

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

inline v2
GetCameraSpaceP(game_state *GameState, low_entity *EntityLow)
{
    world_difference Diff = Subtract(GameState->World, &EntityLow->P, &GameState->CameraP);
    v2 Result = Diff.dXY;

    return(Result);
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
    EntityLow->Sim.Type = Type;

    ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, EntityLow, 0, P);

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

    Entity.Low->Sim.Height = GameState->World->TileSideInMeters;
    Entity.Low->Sim.Width  = Entity.Low->Sim.Height;
    Entity.Low->Sim.Collides = true;

    return(Entity);
}

internal void
InitHitPoints(low_entity *EntityLow, uint32_t HitPointCount)
{
    Assert(HitPointCount <= ArrayCount(EntityLow->Sim.HitPoint));
    EntityLow->Sim.HitPointMax = HitPointCount;
    for(uint32_t HitPointIndex = 0; HitPointIndex < EntityLow->Sim.HitPointMax; ++HitPointIndex)
    {
        hit_point *HitPoint = EntityLow->Sim.HitPoint + HitPointIndex;
        HitPoint->Flags = 0;
        HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
    }
}

internal add_low_entity_result
AddSword(game_state *GameState)
{
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, 0);

    Entity.Low->Sim.Height = 0.5f;
    Entity.Low->Sim.Width  = 1.0f;
    Entity.Low->Sim.Collides = false;

    return(Entity);
}

internal add_low_entity_result
AddPlayer(game_state *GameState)
{
    world_position P = GameState->CameraP;
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, &P);

    Entity.Low->Sim.Height = 0.5f;
    Entity.Low->Sim.Width  = 1.0f;
    Entity.Low->Sim.Collides = true;

    InitHitPoints(Entity.Low, 3);

    add_low_entity_result Sword = AddSword(GameState);
    Entity.Low->Sim.Sword.Index = Sword.LowIndex;

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

    Entity.Low->Sim.Height = 0.5f;
    Entity.Low->Sim.Width  = 1.0f;
    Entity.Low->Sim.Collides = true;

    InitHitPoints(Entity.Low, 3);

    return(Entity);
}

internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, &P);

    Entity.Low->Sim.Height = 0.5f;
    Entity.Low->Sim.Width  = 1.0f;
    Entity.Low->Sim.Collides = true;

    return(Entity);
}

inline move_spec
DefaultMoveSpec()
{
    move_spec Result;

    Result.UnitMaxAccelVector = false;
    Result.Speed = 1.0f;
    Result.Drag  = 0.0f;

    return(Result);
}

inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
          v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC)
{
    Assert(Group->PieceCount < ArrayCount(Group->Pieces));
    entity_visible_piece *Piece = Group->Pieces + Group->PieceCount++;
    Piece->Bitmap = Bitmap;
    Piece->Offset = Group->GameState->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
    Piece->OffsetZ  = Group->GameState->MetersToPixels*OffsetZ;
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

internal void
DrawHitPoints(sim_entity *Entity, entity_visible_piece_group *PieceGroup)
{
    if(Entity->HitPointMax >= 1)
    {
        v2 HealthDim = {0.2f, 0.2f};
        real32 SpacingX = 1.5f*HealthDim.X;
        v2 HitP = {-0.5f*(Entity->HitPointMax - 1)*SpacingX, -0.25f};
        v2 dHitP = {SpacingX, 0.0f};
        for(uint32_t HealthIndex = 0;
            HealthIndex < Entity->HitPointMax;
            ++HealthIndex)
        {
            hit_point *HitPoint = Entity->HitPoint + HealthIndex;
            v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
            if(HitPoint->FilledAmount == 0)
            {
                Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
            }

            PushRect(PieceGroup, HitP, 0, HealthDim, Color, 0.0f);
            HitP += dHitP;
        }
    }
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

        GameState->Backdrop =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
        GameState->Shadow =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");
        GameState->Tree =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tree00.bmp");
        GameState->Sword =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/rock03.bmp");

        hero_bitmaps *Bitmap;

        Bitmap = GameState->HeroBitmaps;
        Bitmap->Head  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
        Bitmap->Cape  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
        Bitmap->Cape  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
        Bitmap->Cape  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        Bitmap->Cape  = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        InitialiseArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitialiseWorld(World, 1.4f);

        int32_t TileSideInPixels  = 60;
        GameState->MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;

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

            DoorLeft   = DoorRight;
            DoorBottom = DoorTop;

            if(CreatedZDoor)
            {
                DoorDown = !DoorDown;
                DoorUp   = !DoorUp;
            }
            else
            {
                DoorUp   = false;
                DoorDown = false;
            }

            DoorRight = false;
            DoorTop   = false;

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

        Memory->IsInitialised = true;
    }

    world *World = GameState->World;

    real32 MetersToPixels = GameState->MetersToPixels;

    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
        if(ConHero->EntityIndex == 0)
        {
            if(Controller->Start.EndedDown)
            {
                *ConHero = {};
                ConHero->EntityIndex = AddPlayer(GameState).LowIndex;
            }
        }
        else
        {
            entity ControllingEntity = ForceEntityIntoHigh(GameState, LowIndex);
            ConHero->ddP = {};

            if(Controller->IsAnalogue)
            {
                // Use analogue movement tuning
                ConHero->ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
            }
            else
            {
                // Use digital movement tuning
                if(Controller->MoveUp.EndedDown)
                {
                    ConHero->ddP.Y = 1.0f;
                }
                if(Controller->MoveDown.EndedDown)
                {
                    ConHero->ddP.Y = -1.0f;
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    ConHero->ddP.X = -1.0f;
                }
                if(Controller->MoveRight.EndedDown)
                {
                    ConHero->ddP.X = 1.0f;
                }
            }

            if(Controller->Start.EndedDown)
            {
                ConHero->dZ = 3.0f;
            }

            ConHero->dSword = {};
            if(Controller->ActionUp.EndedDown)
            {
                ConHero->dSword = V2(0.0f, 1.0f);
            }
            if(Controller->ActionDown.EndedDown)
            {
                ConHero->dSword = V2(0.0f, -1.0f);
            }
            if(Controller->ActionLeft.EndedDown)
            {
                ConHero->dSword = V2(-1.0f, 0.0f);
            }
            if(Controller->ActionRight.EndedDown)
            {
                ConHero->dSword = V2(1.0f, 0.0f);
            }
        }
    }

    uint32_t TileSpanX = 17*3;
    uint32_t TileSpanY =  9*3;
    rectangle2 CameraBounds = RectCenterDim(V2(0, 0), World->TileSideInMetres*V2((real32)TileSpanX, (real32)TileSpanY));
    sim_region *SimRegion = BeginSim(SimArena, GameState->World, GameState->CameraP, CameraBounds);

#if 1
    DrawRectangle(Buffer, V2(0.0f, 0.0f), V2((real32)Buffer->Width, (real32)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else
    DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#endif

    real32 ScreenCentreX = 0.5f*(real32)Buffer->Width;
    real32 ScreenCentreY = 0.5f*(real32)Buffer->Height;

    entity_visible_piece_group PieceGroup;
    PieceGroup.GameState = GameState;
    entity *Entity = SimRegion->Entities;
    for(uint32_t EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
    {
        PieceGroup.PieceCount = 0;

        low_entity *LowEntity = GameState->LowEntities + HighEntity->StorageIndex;

        real32 dt = Input->dtForFrame;
        
        real32 ShadowAlpha = 1.0f - 0.5f*HighEntity->Z;
        if(ShadowAlpha < 0)
        {
            ShadowAlpha = 0.0f;
        }

        hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[LowEntity->FacingDirection];
        switch(LowEntity->Type)
        {
            case EntityType_Hero:
            {
                move_spec MoveSpec = DefaultMoveSpec();
                MoveSpec.UnitMaxAccelVector = true;
                MoveSpec.Speed = 50.0f;
                MoveSpec.Drag  =  9.0f;
                MoveEntity(GameState, ControllingEntity, Input->dtForFrame, &MoveSpec, ddP);
                if((dSword.X != 0.0f) || (dSword.Y != 0.0f))
                {
                    low_entity *LowSword = GetLowEntity(GameState, ControllingEntity.Low->Sim.SwordLowIndex);
                    if(LowSword && !IsValid(LowSword->P))
                    {
                        world_position SwordP = ControllingEntity.Low->P;
                        ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                                             ControllingEntity.Low->Sim.SwordLowIndex, LowSword, 0, &SwordP);
                        entity Sword = ForceEntityIntoHigh(GameState, ControllingEntity.Low->Sim.SwordLowIndex);

                        Sword.Low->Sim.DistanceRemaining = 5.0f;
                        Sword.High->dP = 5.0f*dSword;
                    }
                }
                
                PushBitmap(&PieceGroup, &GameState->Shadow,  V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                PushBitmap(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);
                PushBitmap(&PieceGroup, &HeroBitmaps->Cape,  V2(0, 0), 0, HeroBitmaps->Align);
                PushBitmap(&PieceGroup, &HeroBitmaps->Head,  V2(0, 0), 0, HeroBitmaps->Align);

                DrawHitPoints(LowEntity, &PieceGroup);
            } break;

            case EntityType_Wall:
            {
                PushBitmap(&PieceGroup, &GameState->Tree, V2(0, 0), 0, V2(40, 80));
            } break;

            case EntityType_Sword:
            {
                UpdateSword(GameState, Entity, dt);
                PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                PushBitmap(&PieceGroup, &GameState->Sword,  V2(0, 0), 0, V2(29, 10));
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
                PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align,
                          (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
                PushBitmap(&PieceGroup, &HeroBitmaps->Head, V2(0, 0), 0.2f*BobSin, HeroBitmaps->Align);
            } break;

            case EntityType_Monster:
            {
                UpdateMonster(GameState, Entity, dt);
                PushBitmap(&PieceGroup, &GameState->Shadow,  V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                PushBitmap(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);

                DrawHitPoints(LowEntity, &PieceGroup);
            } break;

            default:
            {
                InvalidCodePath;
            } break;
        }

        real32 ddZ = -9.8f;
        Entity->Z = 0.5f*ddZ*Square(dt) + Entity->dZ*dt + Entity->Z;
        Entity->dZ = ddZ*dt + Entity->dZ;
        if(Entity->Z < 0)
        {
            Entity->Z = 0;
        }
        
        real32 EntityGroundPointX = ScreenCentreX + MetersToPixels*Entity->P.X;
        real32 EntityGroundPointY = ScreenCentreY - MetersToPixels*Entity->P.Y;
        real32 EntityZ = -MetersToPixels*Entity->Z;
#if 0
        v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*LowEntity->Width,
                            PlayerGroundPointY - 0.5f*MetersToPixels*LowEntity->Height};
        v2 EntityWidthHeight = {LowEntity->Width, LowEntity->Height};
        DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + 0.9f*MetersToPixels*EntityWidthHeight,
                      1.0f, 1.0f, 0.0f);
#endif
        for(uint32_t PieceIndex = 0; PieceIndex < PieceGroup.PieceCount; ++PieceIndex)
        {
            entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
            v2 Center = {EntityGroundPointX + Piece->Offset.X,
                         EntityGroundPointY + Piece->Offset.Y + Piece->OffsetZ + Piece->EntityZC*EntityZ};
            if(Piece->Bitmap)
            {
                DrawBitmap(Buffer, Piece->Bitmap, Center.X, Center.Y, Piece->A);
            }
            else
            {
                v2 HalfDim = 0.5f*MetersToPixels*Piece->Dim;
                DrawRectangle(Buffer, Center - HalfDim, Center + HalfDim, Piece->R, Piece->G, Piece->B);
            }
        }
    }

    EndSim(SimRegion, GameState);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 400);
}

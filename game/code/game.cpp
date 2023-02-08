#include "game.h"
#include "game_render_group.cpp"
#include "game_world.cpp"
#include "game_sim_region.cpp"
#include "game_entity.cpp"
#include "game_asset.cpp"
#include "game_audio.cpp"

struct add_low_entity_result
{
    low_entity *Low;
    uint32_t LowIndex;
};
internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type Type, world_position P)
{
    Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
    uint32_t EntityIndex = GameState->LowEntityCount++;

    low_entity *EntityLow = GameState->LowEntities + EntityIndex;
    *EntityLow = {};
    EntityLow->Sim.Type = Type;
    EntityLow->Sim.Collision = GameState->NullCollision;
    EntityLow->P = NullPosition();

    ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, EntityLow, P);

    add_low_entity_result Result;
    Result.Low = EntityLow;
    Result.LowIndex = EntityIndex;
    
    return(Result);
}

internal add_low_entity_result
AddGroundedEntity(game_state *GameState, entity_type Type, world_position P, sim_entity_collision_volume_group *Collision)
{
    add_low_entity_result Entity = AddLowEntity(GameState, Type, P);
    Entity.Low->Sim.Collision = Collision;
    
    return(Entity);
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32_t AbsTileX, int32_t AbsTileY, int32_t AbsTileZ,
                              v3 AdditionalOffset = V3(0, 0, 0))
{
    world_position BasePos = {};

    real32 TileSideInMeters = 1.4f;
    real32 TileDepthInMeters = 3.0f;

    v3 TileDim = V3(TileSideInMeters, TileSideInMeters, TileDepthInMeters);
    v3 Offset = Hadamard(TileDim, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
    world_position Result = MapIntoChunkSpace(World, BasePos, AdditionalOffset + Offset);

    Assert(IsCanonical(World, Result.Offset_));

    return(Result);
}

internal add_low_entity_result
AddStandardRoom(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Space, P, GameState->StandardRoomCollision);
    AddFlags(&Entity.Low->Sim, EntityFlag_Traversable);

    return(Entity);
}

internal add_low_entity_result
AddWall(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Wall, P, GameState->WallCollision);
    AddFlags(&Entity.Low->Sim, EntityFlag_Collides);

    return(Entity);
}

internal add_low_entity_result
AddStair(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Stairwell, P, GameState->StairCollision);
    AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
    Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.xy;
    Entity.Low->Sim.WalkableHeight = GameState->TypicalFloorHeight;

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
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, NullPosition());
    Entity.Low->Sim.Collision = GameState->SwordCollision;

    AddFlags(&Entity.Low->Sim, EntityFlag_Moveable);

    return(Entity);
}

internal add_low_entity_result
AddPlayer(game_state *GameState)
{
    world_position P = GameState->CameraP;
    add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Hero, P, GameState->PlayerCollision);
    AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);

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
    add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Monster, P, GameState->MonsterCollision);
    AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);

    InitHitPoints(Entity.Low, 3);

    return(Entity);
}

internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Familiar, P, GameState->FamiliarCollision);
    AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);

    return(Entity);
}

internal void
DrawHitPoints(sim_entity *Entity, render_group *PieceGroup)
{
    if(Entity->HitPointMax >= 1)
    {
        v2 HealthDim = {0.2f, 0.2f};
        real32 SpacingX = 1.5f*HealthDim.x;
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

            PushRect(PieceGroup, V3(HitP, 0), HealthDim, Color);
            HitP += dHitP;
        }
    }
}

internal void
ClearCollisionRulesFor(game_state *GameState, uint32_t StorageIndex)
{
    // TODO Make a better data structure that allows the removal
    // of collision rules without having to search the entire table.
    // One way to make removal easy would be to always add _both_
    // orders of the pairs of storage indeces to the hash table,
    // so that way no matter which position the entity is in, I can
    // always find it. Then, when I do my first pass through for
    // removal, I can just remember the original top of the free list,
    // and then when i'm done, I can do another pass through all the
    // new things on the free list, and remove the reverse of those pairs.
    for(uint32_t HashBucket = 0; HashBucket < ArrayCount(GameState->CollisionRuleHash); ++HashBucket)
    {
        for(pairwise_collision_rule **Rule = &GameState->CollisionRuleHash[HashBucket];
            *Rule;
            )
        {
            if(((*Rule)->StorageIndexA == StorageIndex) ||
               ((*Rule)->StorageIndexB == StorageIndex))
            {
                pairwise_collision_rule *RemovedRule = *Rule;
                *Rule = (*Rule)->NextInHash;

                RemovedRule->NextInHash = GameState->FirstFreeCollisionRule;
                GameState->FirstFreeCollisionRule = RemovedRule;
            }
            else
            {
                Rule = &(*Rule)->NextInHash;
            }
        }
    }
}

internal void
AddCollisionRule(game_state *GameState, uint32_t StorageIndexA, uint32_t StorageIndexB, bool32 CanCollide)
{
    if(StorageIndexA > StorageIndexB)
    {
        uint32_t Temp = StorageIndexA;
        StorageIndexA = StorageIndexB;
        StorageIndexB = Temp;
    }

    // TODO Implement a better hash function.
    pairwise_collision_rule *Found = 0;
    uint32_t HashBucket = StorageIndexA & (ArrayCount(GameState->CollisionRuleHash) - 1);
    for(pairwise_collision_rule *Rule = GameState->CollisionRuleHash[HashBucket];
        Rule;
        Rule = Rule->NextInHash)
    {
        if((Rule->StorageIndexA == StorageIndexA) &&
           (Rule->StorageIndexB == StorageIndexB))
        {
            Found = Rule;
            break;
        }
    }

    if(!Found)
    {
        Found = GameState->FirstFreeCollisionRule;
        if(Found)
        {
            GameState->FirstFreeCollisionRule = Found->NextInHash;
        }
        else
        {
            Found = PushStruct(&GameState->WorldArena, pairwise_collision_rule);
        }

        Found->NextInHash = GameState->CollisionRuleHash[HashBucket];
        GameState->CollisionRuleHash[HashBucket] = Found;
    }

    if(Found)
    {
        Found->StorageIndexA = StorageIndexA;
        Found->StorageIndexB = StorageIndexB;
        Found->CanCollide = CanCollide;
    }
}

sim_entity_collision_volume_group *
MakeSimpleGroundedCollision(game_state *GameState, real32 DimX, real32 DimY, real32 DimZ)
{
    // TODO Change to using the fundamental types arena, etc.
    sim_entity_collision_volume_group *Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
    Group->VolumeCount = 1;
    Group->Volumes = PushArray(&GameState->WorldArena, Group->VolumeCount, sim_entity_collision_volume);
    Group->TotalVolume.OffsetP = V3(0, 0, 0.5f*DimZ);
    Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
    Group->Volumes[0] = Group->TotalVolume;

    return(Group);
}

sim_entity_collision_volume_group *
MakeNullCollision(game_state *GameState)
{
    sim_entity_collision_volume_group *Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
    Group->VolumeCount = 0;
    Group->Volumes = 0;
    Group->TotalVolume.OffsetP = V3(0, 0, 0);
    // TODO Maybe make this negative?
    Group->TotalVolume.Dim = V3(0, 0, 0);

    return(Group);
}

internal task_with_memory *
BeginTaskWithMemory(transient_state *TranState)
{
    task_with_memory *FoundTask = 0;

    for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
    {
        task_with_memory *Task = TranState->Tasks + TaskIndex;
        if(!Task->BeingUsed)
        {
            FoundTask = Task;
            Task->BeingUsed = true;
            Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
            break;
        }
    }

    return(FoundTask);
}

internal void
EndTaskWithMemory(task_with_memory *Task)
{
    EndTemporaryMemory(Task->MemoryFlush);

    CompletePreviousWritesBeforeFutureWrites;
    Task->BeingUsed = false;
}

struct fill_ground_chunk_work
{
    transient_state *TranState;
    game_state *GameState;
    ground_buffer *GroundBuffer;
    world_position ChunkP;

    task_with_memory *Task;
};
internal PLATFORM_WORK_QUEUE_CALLBACK(FillGroundChunkWork)
{
    TIMED_BLOCK();

    fill_ground_chunk_work *Work = (fill_ground_chunk_work *)Data;

    loaded_bitmap *Buffer = &Work->GroundBuffer->Bitmap;
    Buffer->AlignPercentage = V2(0.5f, 0.5f);
    Buffer->WidthOverHeight = 1.0f;

    real32 Width = Work->GameState->World->ChunkDimInMeters.x;
    real32 Height = Work->GameState->World->ChunkDimInMeters.y;
    Assert(Width == Height);
    v2 HalfDim = 0.5f*V2(Width, Height);

    // TODO: Decide what the pushbuffer size is.
    render_group *RenderGroup = AllocateRenderGroup(Work->TranState->Assets, &Work->Task->Arena, 0, true);
    BeginRender(RenderGroup);
    Orthographic(RenderGroup, Buffer->Width, Buffer->Height, (Buffer->Width - 2) / Width);
    Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));

    for(int32_t ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
    {
        for(int32_t ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
        {
            int32_t ChunkX = Work->ChunkP.ChunkX + ChunkOffsetX;
            int32_t ChunkY = Work->ChunkP.ChunkY + ChunkOffsetY;
            int32_t ChunkZ = Work->ChunkP.ChunkZ;
    
            // TODO Make random number generation more systemic.
            // Looks into wang hashing or some other spatial seed generation thing.
            random_series Series = RandomSeed(139*ChunkX + 593*ChunkY + 329*ChunkZ);

#if 0
            v4 Color = V4(1, 0, 0, 1);
            if((ChunkX % 2) == (ChunkY % 2))
            {
                Color = V4(0, 0, 1, 1);
            }
#else
            v4 Color = {1, 1, 1, 1};
#endif

            v2 Centre = V2(ChunkOffsetX*Width, ChunkOffsetY*Height);

            for(uint32_t GrassIndex = 0; GrassIndex < 100; ++GrassIndex)
            {
                bitmap_id Stamp = GetRandomBitmapFrom(Work->TranState->Assets,
                                                      RandomChoice(&Series, 2) ? Asset_Grass : Asset_Stone,
                                                      &Series);

                v2 P = Centre + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                PushBitmap(RenderGroup, Stamp, 2.0f, V3(P, 0.0f), Color);
            }
        }
    }

    for(int32_t ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY)
    {
        for(int32_t ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX)
        {
            int32_t ChunkX = Work->ChunkP.ChunkX + ChunkOffsetX;
            int32_t ChunkY = Work->ChunkP.ChunkY + ChunkOffsetY;
            int32_t ChunkZ = Work->ChunkP.ChunkZ;

            random_series Series = RandomSeed(139*ChunkX + 593*ChunkY + 329*ChunkZ);

            v2 Centre = V2(ChunkOffsetX*Width, ChunkOffsetY*Height);

            for(uint32_t GrassIndex = 0; GrassIndex < 50; ++GrassIndex)
            {
                bitmap_id Stamp = GetRandomBitmapFrom(Work->TranState->Assets, Asset_Tuft, &Series);
                v2 P = Centre + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                PushBitmap(RenderGroup, Stamp, 0.1f, V3(P, 0.0f));
            }
        }
    }

    Assert(AllResourcesPresent(RenderGroup));

    RenderGroupToOutput(RenderGroup, Buffer);
    EndRender(RenderGroup);

    EndTaskWithMemory(Work->Task);
}

internal void
FillGroundChunk(transient_state *TranState, game_state *GameState, ground_buffer *GroundBuffer, world_position *ChunkP)
{
    task_with_memory *Task = BeginTaskWithMemory(TranState);
    if(Task)
    {
        fill_ground_chunk_work *Work = PushStruct(&Task->Arena, fill_ground_chunk_work);
        Work->Task = Task;
        Work->TranState = TranState;
        Work->GameState = GameState;
        Work->GroundBuffer = GroundBuffer;
        Work->ChunkP = *ChunkP;
        GroundBuffer->P = *ChunkP;
        Platform.AddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);
    }
}

internal void
ClearBitmap(loaded_bitmap *Bitmap)
{
    if(Bitmap->Memory)
    {
        int32_t TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
        ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena *Arena, int32_t Width, int32_t Height, bool32 ClearToZero = true)
{
    loaded_bitmap Result = {};

    Result.AlignPercentage = V2(0.5f, 0.5f);
    Result.WidthOverHeight = SafeRatio1((r32)Width, (r32)Height);

    Result.Width = Width;
    Result.Height = Height;
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
    int32_t TotalBitmapSize = Width*Height*BITMAP_BYTES_PER_PIXEL;
    Result.Memory = PushSize(Arena, TotalBitmapSize, 16);
    if(ClearToZero)
    {
        ClearBitmap(&Result);
    }

    return(Result);
}

internal void
MakeSphereNormalMap(loaded_bitmap *Bitmap, real32 Roughness, real32 Cx = 1.0f, real32 Cy = 1.0f)
{
    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);

    uint8_t *Row = (uint8_t *)Bitmap->Memory;
    for(int32_t Y = 0; Y < Bitmap->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int32_t X = 0; X < Bitmap->Width; ++X)
        {
            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

            real32 Nx = Cx*(2.0f*BitmapUV.x - 1.0f);
            real32 Ny = Cy*(2.0f*BitmapUV.y - 1.0f);

            real32 RootTerm = 1.0f - Nx*Nx - Ny*Ny;
            v3 Normal = {0, 0.707106781188f, 0.707106781188f};
            real32 Nz = 0.0f;
            if(RootTerm >= 0.0f)
            {
                Nz = SquareRoot(RootTerm);
                Normal = V3(Nx, Ny, Nz);
            }

            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
                        255.0f*(0.5f*(Normal.y + 1.0f)),
                        255.0f*(0.5f*(Normal.z + 1.0f)),
                        255.0f*Roughness};

            *Pixel++ = (((uint32_t)(Color.a + 0.5f) << 24) |
                        ((uint32_t)(Color.r + 0.5f) << 16) |
                        ((uint32_t)(Color.g + 0.5f) << 8) |
                        ((uint32_t)(Color.b + 0.5f) << 0));
        }

        Row += Bitmap->Pitch;
    }
}

internal void
MakeSphereDiffuseMap(loaded_bitmap *Bitmap, real32 Cx = 1.0f, real32 Cy = 1.0f)
{
    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);

    uint8_t *Row = (uint8_t *)Bitmap->Memory;
    for(int32_t Y = 0; Y < Bitmap->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int32_t X = 0; X < Bitmap->Width; ++X)
        {
            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

            real32 Nx = Cx*(2.0f*BitmapUV.x - 1.0f);
            real32 Ny = Cy*(2.0f*BitmapUV.y - 1.0f);

            real32 RootTerm = 1.0f - Nx*Nx - Ny*Ny;
            real32 Alpha = 0.0f;
            if(RootTerm >= 0.0f)
            {
                Alpha = 1.0f;
            }

            v3 BaseColor = {0.0f, 0.0f, 0.0f};
            Alpha *= 255.0f;
            v4 Color = {Alpha*BaseColor.x,
                        Alpha*BaseColor.y,
                        Alpha*BaseColor.z,
                        Alpha};

            *Pixel++ = (((uint32_t)(Color.a + 0.5f) << 24) |
                        ((uint32_t)(Color.r + 0.5f) << 16) |
                        ((uint32_t)(Color.g + 0.5f) << 8) |
                        ((uint32_t)(Color.b + 0.5f) << 0));
        }

        Row += Bitmap->Pitch;
    }
}

internal void
MakePyramidNormalMap(loaded_bitmap *Bitmap, real32 Roughness)
{
    real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
    real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);

    uint8_t *Row = (uint8_t *)Bitmap->Memory;
    for(int32_t Y = 0; Y < Bitmap->Height; ++Y)
    {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int32_t X = 0; X < Bitmap->Width; ++X)
        {
            v2 BitmapUV = {InvWidth*(real32)X, InvHeight*(real32)Y};

            int32_t InvX = (Bitmap->Width - 1) - X;
            real32 Seven = 0.707106781188f;
            v3 Normal = {0, 0, Seven};
            if(X < Y)
            {
                if(InvX < Y)
                {
                    Normal.x = -Seven;
                }
                else
                {
                    Normal.y = Seven;
                }
            }
            else
            {
                if(InvX < Y)
                {
                    Normal.y = -Seven;
                }
                else
                {
                    Normal.x = Seven;
                }
            }

            v4 Color = {255.0f*(0.5f*(Normal.x + 1.0f)),
                        255.0f*(0.5f*(Normal.y + 1.0f)),
                        255.0f*(0.5f*(Normal.z + 1.0f)),
                        255.0f*Roughness};

            *Pixel++ = (((uint32_t)(Color.a + 0.5f) << 24) |
                        ((uint32_t)(Color.r + 0.5f) << 16) |
                        ((uint32_t)(Color.g + 0.5f) << 8) |
                        ((uint32_t)(Color.b + 0.5f) << 0));
        }

        Row += Bitmap->Pitch;
    }
}

/*
   Kanji owl codepoints.
   0x5c0f
   0x8033
   0x6728
   0x514e
*/

// TODO: Fix this for looped live code editing.
global_variable render_group *DEBUGRenderGroup;
global_variable r32 LeftEdge;
global_variable r32 AtY;
global_variable r32 FontScale;
global_variable font_id FontID;

internal void
DEBUGReset(game_assets *Assets, u32 Width, u32 Height)
{
    TIMED_BLOCK();

    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    WeightVector.E[Tag_FontType] = 1.0f;
    FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

    FontScale = 1.0f;
    Orthographic(DEBUGRenderGroup, Width, Height, 1.0f);
    LeftEdge = -0.5f*Width;

    ga_font *Info = GetFontInfo(Assets, FontID);
    AtY = 0.5f*Height - FontScale*GetStartingBaselineY(Info);
}

inline b32
IsHex(char Char)
{
    b32 Result = (((Char >= '0') && (Char <= '9')) ||
                  ((Char >= 'A') && (Char <= 'F')));

    return(Result);
}

inline u32
GetHex(char Char)
{
    u32 Result = 0;

    if((Char >= '0') && (Char <= '9'))
    {
        Result = Char - '0';
    }
    else if((Char >= 'A') && (Char <= 'F'))
    {
        Result = 0xA + (Char - 'A');
    }

    return(Result);
}

internal void
DEBUGTextLine(char *String)
{
    if(DEBUGRenderGroup)
    {
        render_group *RenderGroup = DEBUGRenderGroup;

        loaded_font *Font = PushFont(RenderGroup, FontID);
        if(Font)
        {
            ga_font *Info = GetFontInfo(RenderGroup->Assets, FontID);

            u32 PrevCodepoint = 0;
            r32 CharScale = FontScale;
            v4 Color = V4(1, 1, 1, 1);
            r32 AtX = LeftEdge;
            for(char *At = String; *At;)
            {
                if((At[0] == '\\') &&
                   (At[1] == '#') &&
                   (At[2] != 0) &&
                   (At[3] != 0) &&
                   (At[4] != 0))
                {
                    r32 CScale = 1.0f / 9.0f;
                    Color = V4(Clamp01(CScale*(r32)(At[2] - '0')),
                               Clamp01(CScale*(r32)(At[3] - '0')),
                               Clamp01(CScale*(r32)(At[4] - '0')),
                               1.0f);
                    At += 5;
                }
                else if((At[0] == '\\') &&
                        (At[1] == '^') &&
                        (At[2] != 0))
                {
                    r32 CScale = 1.0f / 9.0f;
                    CharScale = FontScale*Clamp01(CScale*(r32)(At[2] - '0'));
                    At += 3;
                }
                else
                {
                    u32 Codepoint = *At;
                    if((At[0] == '\\') &&
                       (IsHex(At[1])) &&
                       (IsHex(At[2])) &&
                       (IsHex(At[3])) &&
                       (IsHex(At[4])))
                    {
                        Codepoint = ((GetHex(At[1]) << 12) |
                                     (GetHex(At[2]) << 8) |
                                     (GetHex(At[3]) << 4) |
                                     (GetHex(At[4]) << 0));
                        At += 4;
                    }

                    r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodepoint, Codepoint);
                    AtX += AdvanceX;

                    if(Codepoint != ' ')
                    {
                        bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, Codepoint);
                        ga_bitmap *Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                        PushBitmap(RenderGroup, BitmapID, CharScale*(r32)Info->Dim[1], V3(AtX, AtY, 0), Color);
                    }

                    PrevCodepoint = Codepoint;

                    ++At;
                }
            }

            AtY -= GetLineAdvanceFor(Info)*FontScale;
        }
    }
}

// TODO: Stop using stdio
#include <stdio.h>

struct debug_statistic
{
    r64 Min;
    r64 Max;
    r64 Avg;
    r32 Count;
};
inline void
BeginDebugStatistic(debug_statistic *Stat)
{
    Stat->Count = 0.0f;
    Stat->Min = Real32Maximum;
    Stat->Max = -Real32Maximum;
    Stat->Avg = 0.0f;
}

inline void
AccumulateDebugStatistic(debug_statistic *Stat, r64 Value)
{
    ++Stat->Count;

    if(Stat->Min > Value)
    {
        Stat->Min = Value;
    }

    if(Stat->Max < Value)
    {
        Stat->Max = Value;
    }

    Stat->Avg += Value;
}

inline void
EndDebugStatistic(debug_statistic *Stat)
{
    if(Stat->Count != 0)
    {
        Stat->Avg /= (r64)Stat->Count;
    }
    else
    {
        Stat->Min = 0.0f;
        Stat->Max = 0.0f;
    }
}

internal void
OverlayCycleCounters(game_memory *Memory)
{
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    if(DebugState)
    {
        for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
        {
            debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;

            debug_statistic HitCount, CycleCount;
            BeginDebugStatistic(&HitCount);
            BeginDebugStatistic(&CycleCount);
            for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
            {
                AccumulateDebugStatistic(&HitCount, Counter->Snapshots[SnapshotIndex].HitCount);
                AccumulateDebugStatistic(&CycleCount, Counter->Snapshots[SnapshotIndex].CycleCount);
            }
            EndDebugStatistic(&HitCount);
            EndDebugStatistic(&CycleCount);

            if(HitCount.Max > 0.0f)
            {
#if 1
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "%32s(%4d): %20ucy %18uh %20ucy/h",
                            Counter->FunctionName,
                            Counter->LineNumber,
                            (u32)CycleCount.Avg,
                            (u32)HitCount.Avg,
                            (u32)(CycleCount.Avg / HitCount.Avg));
                DEBUGTextLine(TextBuffer);
#endif
            }
        }
    //    DEBUGTextLine("\\5C0F\\8033\\6728\\514E");
    //    DEBUGTextLine("111111");
    //    DEBUGTextLine("999999");
    //    DEBUGTextLine("AVA WA Ta");
    }
}

#if GAME_INTERNAL
game_memory *DebugGlobalMemory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Platform = Memory->PlatformAPI;

#if GAME_INTERNAL
    DebugGlobalMemory = Memory;
#endif
    TIMED_BLOCK();

    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons)));

    uint32_t GroundBufferWidth = 256;
    uint32_t GroundBufferHeight = 256;

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialised)
    {
        uint32_t TilesPerWidth = 17;
        uint32_t TilesPerHeight = 9;

        GameState->EffectsEntropy = RandomSeed(1234);
        GameState->TypicalFloorHeight = 3.0f;

        // TODO: Remove this.
        real32 PixelsToMetres = 1.0f / 42.0f;
        v3 WorldChunkDimInMeters = {PixelsToMetres*(real32)GroundBufferWidth,
                                    PixelsToMetres*(real32)GroundBufferHeight,
                                    GameState->TypicalFloorHeight};

        InitialiseArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

        InitialiseAudioState(&GameState->AudioState, &GameState->WorldArena);

        // NOTE: Reserve entity slot 0 for the null entity
        AddLowEntity(GameState, EntityType_Null, NullPosition());

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        InitialiseWorld(World, WorldChunkDimInMeters);

        real32 TileSideInMeters = 1.4f;
        real32 TileDepthInMeters = GameState->TypicalFloorHeight;

        GameState->NullCollision = MakeNullCollision(GameState);
        GameState->SwordCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.1f);
        GameState->StairCollision = MakeSimpleGroundedCollision(GameState,
                                                                TileSideInMeters,
                                                                2.0f*TileSideInMeters,
                                                                1.1f*TileDepthInMeters);
        GameState->PlayerCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 1.2f);
        GameState->MonsterCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
        GameState->FamiliarCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
        GameState->WallCollision = MakeSimpleGroundedCollision(GameState,
                                                               TileSideInMeters,
                                                               TileSideInMeters,
                                                               TileDepthInMeters);
        GameState->StandardRoomCollision = MakeSimpleGroundedCollision(GameState,
                                                                       TilesPerWidth*TileSideInMeters,
                                                                       TilesPerHeight*TileSideInMeters,
                                                                       0.9f*TileDepthInMeters);

        random_series Series = RandomSeed(1234);

        uint32_t ScreenBaseX = 0;
        uint32_t ScreenBaseY = 0;
        uint32_t ScreenBaseZ = 0;
        uint32_t ScreenX = ScreenBaseX;
        uint32_t ScreenY = ScreenBaseY;
        uint32_t AbsTileZ = ScreenBaseZ;

        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorUp = false;
        bool32 DoorDown = false;
        for(uint32_t ScreenIndex = 0; ScreenIndex < 2000; ++ScreenIndex)
        {
#if 1
            uint32_t DoorDirection = RandomChoice(&Series, (DoorUp || DoorDown) ? 2 : 4); 
#else
            uint32_t DoorDirection = RandomChoice(&Series, 2); 
#endif

//            DoorDirection = 3;

            bool32 CreatedZDoor = false;
            if(DoorDirection == 3)
            {
                CreatedZDoor = true;
                DoorDown = true;
            }
            else if(DoorDirection == 2)
            {
                CreatedZDoor = true;
                DoorUp = true;
            }
            else if(DoorDirection == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

            AddStandardRoom(GameState,
                            ScreenX*TilesPerWidth + TilesPerWidth/2,
                            ScreenY*TilesPerHeight + TilesPerHeight/2,
                            AbsTileZ);

            for(uint32_t TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for(uint32_t TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32_t AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32_t AbsTileY = ScreenY*TilesPerHeight + TileY;

                    bool32 ShouldBeDoor = false;
                    if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
                    {
                        ShouldBeDoor = true;
                    }

                    if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
                    {
                        ShouldBeDoor = true;
                    }

                    if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
                    {
                        ShouldBeDoor = true;
                    }

                    if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
                    {
                        ShouldBeDoor = true;
                    }

                    if(ShouldBeDoor)
                    {
                        AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
                    }
                    else if(CreatedZDoor)
                    {
                        if(((AbsTileZ % 2) && (TileX == 10) && (TileY == 5)) ||
                           (!(AbsTileZ % 2) && (TileX == 4) && (TileY == 5)))
                        {
                            AddStair(GameState, AbsTileX, AbsTileY, DoorDown ? AbsTileZ - 1 : AbsTileZ);
                        }
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

            if(DoorDirection == 3)
            {
                AbsTileZ -= 1;
            }
            else if(DoorDirection == 2)
            {
                AbsTileZ += 1;
            }
            else if(DoorDirection == 1)
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
        uint32_t CameraTileX = ScreenBaseX*TilesPerWidth + 17/2;
        uint32_t CameraTileY = ScreenBaseY*TilesPerHeight + 9/2;
        uint32_t CameraTileZ = ScreenBaseZ;
        NewCameraP = ChunkPositionFromTilePosition(GameState->World, CameraTileX, CameraTileY, CameraTileZ);
        GameState->CameraP = NewCameraP;

        AddMonster(GameState, CameraTileX - 3, CameraTileY + 2, CameraTileZ);
        for(int FamiliarIndex = 0; FamiliarIndex < 1; ++FamiliarIndex)
        {
            int32_t FamiliarOffsetX = RandomBetween(&Series, -7, 7);
            int32_t FamiliarOffsetY = RandomBetween(&Series, -3, -1);
            if((FamiliarOffsetX != 0) ||
               (FamiliarOffsetY != 0))
            {
                AddFamiliar(GameState, CameraTileX + FamiliarOffsetX, CameraTileY + FamiliarOffsetY, CameraTileZ);
            }
        }

        GameState->IsInitialised = true;
    }

    // Transient initialisation
    Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
    transient_state *TranState = (transient_state *)Memory->TransientStorage;
    if(!TranState->IsInitialised)
    {
        InitialiseArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                        (uint8_t *)Memory->TransientStorage + sizeof(transient_state));

        TranState->HighPriorityQueue = Memory->HighPriorityQueue;
        TranState->LowPriorityQueue  = Memory->LowPriorityQueue;
        for(uint32_t TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex)
        {
            task_with_memory *Task = TranState->Tasks + TaskIndex;

            Task->BeingUsed = false;
            SubArena(&Task->Arena, &TranState->TranArena, Megabytes(1));
        }

        TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(16), TranState);

        DEBUGRenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(16), false);

//        GameState->Music = PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));

        // TODO: Pick a real number here.
        TranState->GroundBufferCount = 256;
        TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
        for(uint32_t GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
            GroundBuffer->P = NullPosition();
        }

        GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
        GameState->TestNormal = MakeEmptyBitmap(&TranState->TranArena, GameState->TestDiffuse.Width,
                                                GameState->TestDiffuse.Height, false);
        MakeSphereNormalMap(&GameState->TestNormal, 0.0f);
        MakeSphereDiffuseMap(&GameState->TestDiffuse);
//          MakePyramidNormalMap(&GameState->TestNormal, 0.0f);

        TranState->EnvMapWidth = 512;
        TranState->EnvMapHeight = 256;
        for(uint32_t MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex)
        {
            environment_map *Map = TranState->EnvMaps + MapIndex;
            uint32_t Width = TranState->EnvMapWidth;
            uint32_t Height = TranState->EnvMapHeight;
            for(uint32_t LODIndex = 0; LODIndex < ArrayCount(Map->LOD); ++LODIndex)
            {
                Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, false);
                Width >>= 1;
                Height >>= 1;
            }
        }

        TranState->IsInitialised = true;
    }

    if(DEBUGRenderGroup)
    {
        BeginRender(DEBUGRenderGroup);
        DEBUGReset(TranState->Assets, Buffer->Width, Buffer->Height);
    }

#if 0
    // TODO: Re-enable this but make sure I don't touch ones that are in flight.
    if(Input->ExecutableReloaded)
    {
        for(uint32_t GroundBufferIndex = 0;
            GroundBufferIndex < TranState->GroundBufferCount;
            ++GroundBufferIndex)
        {
            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
            GroundBuffer->P = NullPosition();
        }
    }
#endif

    world *World = GameState->World;

    {
        v2 MusicVolume;
        MusicVolume.y = SafeRatio0((r32)Input->MouseX, (r32)Buffer->Width);
        MusicVolume.x = 1.0f - MusicVolume.y;
        ChangeVolume(&GameState->AudioState, GameState->Music, 0.01f, MusicVolume);
    }

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
            ConHero->dZ = 0.0f;
            ConHero->ddP = {};
            ConHero->dSword = {};

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
                    ConHero->ddP.y = 1.0f;
                }
                if(Controller->MoveDown.EndedDown)
                {
                    ConHero->ddP.y = -1.0f;
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    ConHero->ddP.x = -1.0f;
                }
                if(Controller->MoveRight.EndedDown)
                {
                    ConHero->ddP.x = 1.0f;
                }
            }

            if(Controller->Start.EndedDown)
            {
                ConHero->dZ = 3.0f;
            }

            ConHero->dSword = {};
            if(Controller->ActionUp.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 10.0f, V2(1.0f, 1.0f));
                ConHero->dSword = V2(0.0f, 1.0f);
            }
            if(Controller->ActionDown.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 10.0f, V2(0.0f, 0.0f));
                ConHero->dSword = V2(0.0f, -1.0f);
            }
            if(Controller->ActionLeft.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 5.0f, V2(1.0f, 0.0f));
                ConHero->dSword = V2(-1.0f, 0.0f);
            }
            if(Controller->ActionRight.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 5.0f, V2(0.0f, 1.0f));
                ConHero->dSword = V2(1.0f, 0.0f);
            }
        }
    }

    //
    // Render
    //
    temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);

    loaded_bitmap DrawBuffer_ = {};
    loaded_bitmap *DrawBuffer = &DrawBuffer_;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    DrawBuffer->Memory = Buffer->Memory;

#if 0
    // Enable this to test weird buffer sizes in the renderer.
    DrawBuffer->Width = 1279;
    DrawBuffer->Height = 719;
#endif

    // TODO: Need to figure out what the pushbuffer size is.
    render_group *RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(4), false);
    BeginRender(RenderGroup);
    real32 WidthOfMonitor = 0.635f; // Horizontal measurement of monitor in metres
    real32 MetresToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
    real32 FocalLength = 0.6f;
    real32 DistanceAboveGround = 9.0f;
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetresToPixels, FocalLength, DistanceAboveGround);
    Clear(RenderGroup, V4(0.25f, 0.25f, 0.25f, 0.0f));

    v2 ScreenCentre = {0.5f*(real32)DrawBuffer->Width,
                       0.5f*(real32)DrawBuffer->Height};

    rectangle2 ScreenBounds = GetCameraRectangleAtTarget(RenderGroup);
    rectangle3 CameraBoundsInMetres = RectMinMax(V3(ScreenBounds.Min, 0.0f), V3(ScreenBounds.Max, 0.0f));
    CameraBoundsInMetres.Min.z = -3.0f*GameState->TypicalFloorHeight;
    CameraBoundsInMetres.Max.z =  1.0f*GameState->TypicalFloorHeight;

    // Ground chunk rendering.
    for(uint32_t GroundBufferIndex = 0;
        GroundBufferIndex < TranState->GroundBufferCount;
        ++GroundBufferIndex)
    {
        ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        if(IsValid(GroundBuffer->P))
        {
            loaded_bitmap *Bitmap = &GroundBuffer->Bitmap;
            v3 Delta = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);

            if((Delta.z >= -1.0f) && (Delta.z < 1.0f))
            {
                real32 GroundSideInMetres = World->ChunkDimInMeters.x;
                PushBitmap(RenderGroup, Bitmap, 1.0f*GroundSideInMetres, Delta);
#if 0
                PushRectOutline(RenderGroup, Delta, V2(GroundSideInMetres, GroundSideInMetres),
                                V4(1.0f, 1.0f, 0.0f, 1.0f));
#endif
            }
        }
    }

    // This is Ground chunk updating.
    {
        world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMetres));
        world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMetres));

        for(int32_t ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ)
        {
            for(int32_t ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY)
            {
                for(int32_t ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX)
                {
//                    world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
//                    if(Chunk)
                    {
                        world_position ChunkCentreP = CentredChunkPoint(ChunkX, ChunkY, ChunkZ);
                        v3 RelP = Subtract(World, &ChunkCentreP, &GameState->CameraP);

                        // TODO: This is super inefficient and i'll need to fix it later.
                        real32 FurthestBufferLengthSq = 0.0f;
                        ground_buffer *FurthestBuffer = 0;
                        for(uint32_t GroundBufferIndex = 0;
                            GroundBufferIndex < TranState->GroundBufferCount;
                            ++GroundBufferIndex)
                        {
                            ground_buffer *GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
                            if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCentreP))
                            {
                                FurthestBuffer = 0;
                                break;
                            }
                            else if(IsValid(GroundBuffer->P))
                            {
                                v3 RelP = Subtract(World, &GroundBuffer->P, &GameState->CameraP);
                                real32 BufferLengthSq = LengthSq(RelP.xy);
                                if(FurthestBufferLengthSq < BufferLengthSq)
                                {
                                    FurthestBufferLengthSq = BufferLengthSq;
                                    FurthestBuffer = GroundBuffer;
                                }
                            }
                            else
                            {
                                FurthestBufferLengthSq = Real32Maximum;
                                FurthestBuffer = GroundBuffer;
                            }
                        }

                        if(FurthestBuffer)
                        {
                            FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCentreP);
                        }
                    }
                }
            }
        }
    }

    // TODO: How big do I actually want to expand here?
    // TODO: Do we I want to simulate upper floors and stuff?
    v3 SimBoundsExpansion = {15.0f, 15.0f, 0.0f};
    rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMetres, SimBoundsExpansion);
    temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
    world_position SimCentreP = GameState->CameraP;
    sim_region *SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
                                     SimCentreP, SimBounds, Input->dtForFrame);

    v3 CameraP = Subtract(World, &GameState->CameraP, &SimCentreP);

    PushRectOutline(RenderGroup, V3(0.0f, 0.0f, 0.0f), GetDim(ScreenBounds), V4(1.0f, 1.0f, 0.0f, 1));
//    PushRectOutline(RenderGroup, V3(0.0f, 0.0f, 0.0f), GetDim(CameraBoundsInMetres).xy, V4(1.0f, 1.0f, 1.0f, 1));
    PushRectOutline(RenderGroup, V3(0.0f, 0.0f, 0.0f), GetDim(SimBounds).xy, V4(0.0f, 1.0f, 1.0f, 1));
    PushRectOutline(RenderGroup, V3(0.0f, 0.0f, 0.0f), GetDim(SimRegion->Bounds).xy, V4(1.0f, 0.0f, 1.0f, 1));

    for(uint32_t EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex)
    {
        sim_entity *Entity = SimRegion->Entities + EntityIndex;
        if(Entity->Updatable)
        {
            real32 dt = Input->dtForFrame;
        
            // This is wrong, should be computed after update.
            real32 ShadowAlpha = 1.0f - 0.5f*Entity->P.z;
            if(ShadowAlpha < 0)
            {
                ShadowAlpha = 0.0f;
            }

            move_spec MoveSpec = DefaultMoveSpec();
            v3 ddP = {};

            // TODO: Probably indicates I want to seperate update and render
            // for entities sometime soon.
            v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
            real32 FadeTopEndZ = 0.75f*GameState->TypicalFloorHeight;
            real32 FadeTopStartZ = 0.5f*GameState->TypicalFloorHeight;
            real32 FadeBottomStartZ = -2.0f*GameState->TypicalFloorHeight;
            real32 FadeBottomEndZ = -2.25f*GameState->TypicalFloorHeight;
            RenderGroup->GlobalAlpha = 1.0f;
            if(CameraRelativeGroundP.z > FadeTopStartZ)
            {
                RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeTopEndZ,
                                                             CameraRelativeGroundP.z,
                                                             FadeTopStartZ);
            }
            else if(CameraRelativeGroundP.z < FadeBottomStartZ)
            {
                RenderGroup->GlobalAlpha = Clamp01MapToRange(FadeBottomEndZ,
                                                             CameraRelativeGroundP.z,
                                                             FadeBottomStartZ);
            }

            //
            // Pre-physics entity work.
            //
            hero_bitmap_ids HeroBitmaps = {};
            asset_vector MatchVector = {};
            MatchVector.E[Tag_FacingDirection] = Entity->FacingDirection;
            asset_vector WeightVector = {};
            WeightVector.E[Tag_FacingDirection] = 1.0f;
            HeroBitmaps.Head = GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector);
            HeroBitmaps.Cape = GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector);
            HeroBitmaps.Torso = GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector);
            switch(Entity->Type)
            {
                case EntityType_Hero:
                {
                    for(uint32_t ControlIndex = 0; ControlIndex < ArrayCount(GameState->ControlledHeroes); ++ControlIndex)
                    {
                        controlled_hero *ConHero = GameState->ControlledHeroes + ControlIndex;

                        if(Entity->StorageIndex == ConHero->EntityIndex)
                        {
                            if(ConHero->dZ != 0.0f)
                            {
                                Entity->dP.z = ConHero->dZ;
                            }

                            MoveSpec.UnitMaxAccelVector = true;
                            MoveSpec.Speed = 50.0f;
                            MoveSpec.Drag = 8.0f;
                            ddP = V3(ConHero->ddP, 0);
                            
                            if((ConHero->dSword.x != 0.0f) || (ConHero->dSword.y != 0.0f))
                            {
                                sim_entity *Sword = Entity->Sword.Ptr;
                                if(Sword && IsSet(Sword, EntityFlag_Nonspatial))
                                {
                                    Sword->DistanceLimit = 5.0f;
                                    MakeEntitySpatial(Sword, Entity->P, Entity->dP + 5.0f*V3(ConHero->dSword, 0));
                                    AddCollisionRule(GameState, Sword->StorageIndex, Entity->StorageIndex, false);

                                    PlaySound(&GameState->AudioState, GetRandomSoundFrom(TranState->Assets, Asset_Bloop, &GameState->EffectsEntropy));
                                }
                            }
                        }
                    }
                } break;

                case EntityType_Sword:
                {
                    MoveSpec.UnitMaxAccelVector = false;
                    MoveSpec.Speed = 0.0f;
                    MoveSpec.Drag = 0.0f;

                    if(Entity->DistanceLimit == 0.0f)
                    {
                        ClearCollisionRulesFor(GameState, Entity->StorageIndex);
                        MakeEntityNonspatial(Entity);
                    }
                } break;

                case EntityType_Familiar:
                {
                    sim_entity *ClosestHero = 0;
                    real32 ClosestHeroDSq = Square(10.0f);

#if 0
                    sim_entity *TestEntity = SimRegion->Entities;
                    for(uint32_t TestEntityIndex = 0;
                        TestEntityIndex < SimRegion->EntityCount;
                        ++TestEntityIndex, ++TestEntity)
                    {
                        if(TestEntity->Type == EntityType_Hero)
                        {
                            real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
                            if(ClosestHeroDSq > TestDSq)
                            {
                                ClosestHero = TestEntity;
                                ClosestHeroDSq = TestDSq;
                            }
                        }
                    }
#endif

                    if(ClosestHero && (ClosestHeroDSq > Square(3.0f)))
                    {
                        real32 Acceleration = 0.5f;
                        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
                        ddP = OneOverLength*(ClosestHero->P - Entity->P);
                    }

                    MoveSpec.UnitMaxAccelVector = true;
                    MoveSpec.Speed = 50.0f;
                    MoveSpec.Drag = 8.0f;
                } break;
            }

            if(!IsSet(Entity, EntityFlag_Nonspatial) &&
               IsSet(Entity, EntityFlag_Moveable))
            {
                MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
            }

            RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);

            //
            // Post-Physics entity work.
            //
            switch(Entity->Type)
            {
                case EntityType_Hero:
                {
                    real32 HeroSizeC = 2.5f;
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 
                               HeroSizeC*1.0f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                    PushBitmap(RenderGroup, HeroBitmaps.Torso, HeroSizeC*1.2f, V3(0, 0, 0));
                    PushBitmap(RenderGroup, HeroBitmaps.Cape, HeroSizeC*1.2f, V3(0, 0, 0));
                    PushBitmap(RenderGroup, HeroBitmaps.Head, HeroSizeC*1.2f, V3(0, 0, 0));
                    DrawHitPoints(Entity, RenderGroup);

#if 0
                    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 3; ++ParticleSpawnIndex)
                    {
                        particle *Particle = GameState->Particles + GameState->NextParticle++;
                        if(GameState->NextParticle >= ArrayCount(GameState->Particles))
                        {
                            GameState->NextParticle = 0;
                        }

                        Particle->P = V3(RandomBetween(&GameState->EffectsEntropy, -0.05f, 0.05f), 0, 0);
                        Particle->dP = V3(RandomBetween(&GameState->EffectsEntropy, -0.01f, 0.01f), 
                                          7.0f*RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f),
                                          0.0f);
                        Particle->ddP = V3(0.0f, -9.8f, 0.0f);
                        Particle->Color = V4(RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                             1.0f);
                        Particle->dColor = V4(0, 0, 0, -0.25f);

                        asset_vector MatchVector = {};
                        asset_vector WeightVector = {};
                        char Nothings[] = "NOTHINGS";
                        MatchVector.E[Tag_UnicodeCodepoint] = (r32)Nothings[RandomChoice(&GameState->EffectsEntropy, ArrayCount(Nothings) - 1)];
                        WeightVector.E[Tag_UnicodeCodepoint] = 1.0f;

                        Particle->BitmapID = GetBestMatchBitmapFrom(TranState->Assets, Asset_Font,
                                                                    &MatchVector, &WeightVector);

//                        Particle->BitmapID = GetRandomBitmapFrom(TranState->Assets, Asset_Font, &GameState->EffectsEntropy);
                    }

                    // Particle system test.
                    ZeroStruct(GameState->ParticleCels);

                    r32 GridScale = 0.25f;
                    r32 InvGridScale = 1.0f / GridScale;
                    v3 GridOrigin = {-0.5f*GridScale*PARTICLE_CEL_DIM, 0.0f, 0.0f};
                    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                    {
                        particle *Particle = GameState->Particles + ParticleIndex;

                        v3 P = InvGridScale*(Particle->P - GridOrigin);

                        s32 X = TruncateReal32ToInt32(P.x);
                        s32 Y = TruncateReal32ToInt32(P.y);

                        if(X < 0) {X = 0;}
                        if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
                        if(Y < 0) {Y = 0;}
                        if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}

                        particle_cel *Cel = &GameState->ParticleCels[Y][X];
                        r32 Density = Particle->Color.a;
                        Cel->Density += Density;
                        Cel->VelocityTimesDensity += Density*Particle->dP;
                    }

#if 0
                    for(u32 Y = 0; Y < PARTICLE_CEL_DIM; ++Y)
                    {
                        for(u32 X = 0; X < PARTICLE_CEL_DIM; ++X)
                        {
                            particle_cel *Cel = &GameState->ParticleCels[Y][X];
                            r32 Alpha = Clamp01(0.1f*Cel->Density);
                            PushRect(RenderGroup, GridScale*V3((r32)X, (r32)Y, 0) + GridOrigin, GridScale*V2(1.0f, 1.0f),
                                     V4(Alpha, Alpha, Alpha, 1.0f));
                        }
                    }
#endif

                    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
                    {
                        particle *Particle = GameState->Particles + ParticleIndex;

                        v3 P = InvGridScale*(Particle->P - GridOrigin);

                        s32 X = TruncateReal32ToInt32(P.x);
                        s32 Y = TruncateReal32ToInt32(P.y);

                        if(X < 1) {X = 1;}
                        if(X > PARTICLE_CEL_DIM - 2) {X = (PARTICLE_CEL_DIM - 2);}
                        if(Y < 1) {Y = 1;}
                        if(Y > PARTICLE_CEL_DIM - 2) {Y = (PARTICLE_CEL_DIM - 2);}

                        particle_cel *CelCentre = &GameState->ParticleCels[Y][X];
                        particle_cel *CelLeft = &GameState->ParticleCels[Y][X - 1];
                        particle_cel *CelRight = &GameState->ParticleCels[Y][X + 1];
                        particle_cel *CelDown = &GameState->ParticleCels[Y - 1][X];
                        particle_cel *CelUp = &GameState->ParticleCels[Y + 1][X];

                        v3 Dispersion = {};
                        r32 Dc = 1.0f;
                        Dispersion += Dc*(CelCentre->Density - CelLeft->Density)*V3(-1.0f, 0.0f, 0.0f);
                        Dispersion += Dc*(CelCentre->Density - CelRight->Density)*V3(1.0f, 0.0f, 0.0f);
                        Dispersion += Dc*(CelCentre->Density - CelDown->Density)*V3(0.0f, -1.0f, 0.0f);
                        Dispersion += Dc*(CelCentre->Density - CelUp->Density)*V3(0.0f, 1.0f, 0.0f);

                        v3 ddP = Particle->ddP + Dispersion;

                        // Simulate the particle forward in time.
                        Particle->P += (0.5f*Square(Input->dtForFrame)*Input->dtForFrame*ddP +
                                        Input->dtForFrame*Particle->dP);
                        Particle->dP += Input->dtForFrame*ddP;
                        Particle->Color += Input->dtForFrame*Particle->dColor;

                        if(Particle->P.y < 0.0f)
                        {
                            r32 CoefficientOfRestitution = 0.3f;
                            r32 CoefficientOfFriction = 0.7f;
                            Particle->P.y = -Particle->P.y;
                            Particle->dP.y = -CoefficientOfRestitution*Particle->dP.y;
                            Particle->dP.x = CoefficientOfFriction*Particle->dP.x;
                        }

                        // TODO: I should probably just clamp colours in the renderer.
                        v4 Color;
                        Color.r = Clamp01(Particle->Color.r);
                        Color.g = Clamp01(Particle->Color.g);
                        Color.b = Clamp01(Particle->Color.b);
                        Color.a = Clamp01(Particle->Color.a);

                        if(Color.a > 0.9f)
                        {
                            Color.a = 0.9f*Clamp01MapToRange(1.0f, Color.a, 0.9f);
                        }

                        // Render the particle.
                        PushBitmap(RenderGroup, Particle->BitmapID, 1.0f, Particle->P, Color);
                    }
#endif
                } break;

                case EntityType_Wall:
                {
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Tree), 2.5f, V3(0, 0, 0));
                } break;

                case EntityType_Stairwell:
                {
                    PushRect(RenderGroup, V3(0, 0, 0), Entity->WalkableDim, V4(1, 0.5f, 0, 1));
                    PushRect(RenderGroup, V3(0, 0, Entity->WalkableHeight), Entity->WalkableDim, V4(1, 1, 0, 1));
                } break;

                case EntityType_Sword:
                {
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 
                               0.5f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Sword), 0.5f, V3(0, 0, 0));
                } break;

                case EntityType_Familiar:
                {
                    Entity->tBob += dt;
                    if(Entity->tBob > Tau32)
                    {
                        Entity->tBob -= Tau32;
                    }
                    real32 BobSin = Sin(2.0f*Entity->tBob);
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow),
                               2.5f, V3(0, 0, 0), V4(1, 1, 1, (0.5f*ShadowAlpha) + 0.2f*BobSin));
                    PushBitmap(RenderGroup, HeroBitmaps.Head, 2.5f, V3(0, 0, 0.25f*BobSin));
                } break;

                case EntityType_Monster:
                {
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow),
                               4.5f, V3(0, 0, 0), V4(1, 1, 1, ShadowAlpha));
                    PushBitmap(RenderGroup, HeroBitmaps.Torso, 4.5f, V3(0, 0, 0));

                    DrawHitPoints(Entity, RenderGroup);
                } break;

                case EntityType_Space:
                {
#if 0
                    for(uint32_t VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; ++VolumeIndex)
                    {
                        sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
                        PushRectOutline(RenderGroup, Volume->OffsetP - V3(0, 0, 0.5f*Volume->Dim.z),
                                        Volume->Dim.xy, V4(0, 0.5f, 1.0f, 1));
                    }
#endif
                } break;

                default:
                {
                    InvalidCodePath;
                } break;
            }
        }
    }

    RenderGroup->GlobalAlpha = 1.0f;

#if 0
    GameState->Time += Input->dtForFrame;

    v3 MapColor[] =
    {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };
    for(uint32_t MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex)
    {
        environment_map *Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap *LOD = Map->LOD + 0;
        bool32 RowCheckerOn = false;
        int32_t CheckerWidth = 16;
        int32_t CheckerHeight = 16;
        for(int32_t Y = 0; Y < LOD->Height; Y += CheckerHeight)
        {
            bool32 CheckerOn = RowCheckerOn;
            for(int32_t X = 0; X < LOD->Width; X += CheckerWidth)
            {
                v4 Color = CheckerOn ? V4(MapColor[MapIndex], 1.0f) : V4(0, 0, 0, 1);
                v2 MinP = V2i(X, Y);
                v2 MaxP = MinP + V2i(CheckerWidth, CheckerHeight);
                DrawRectangle(LOD, MinP, MaxP, Color);
                CheckerOn = !CheckerOn;
            }
            RowCheckerOn = !RowCheckerOn;
        }
    }
    TranState->EnvMaps[0].Pz = -1.5f;
    TranState->EnvMaps[1].Pz = 0.0f;
    TranState->EnvMaps[2].Pz = 1.5f;

    DrawBitmap(TranState->EnvMaps[0].LOD + 0,
               &TranState->GroundBuffers[TranState->GroundBufferCount - 1].Bitmap,
               125.0f, 50.0f, 1.0f);

//    Angle = 0.0f;

    v2 Origin = ScreenCentre;

    real32 Angle = 0.1f*GameState->Time;
#if 1
    v2 Disp = {100.0f*Cos(5.0f*Angle),
               100.0f*Sin(3.0f*Angle)};
#else
    v2 Disp = {};
#endif

#if 1
    v2 XAxis = 100.0f*V2(Cos(10.0f*Angle), Sin(10.0f*Angle));
    v2 YAxis = Perp(XAxis);
#else
    v2 XAxis = {100.0f, 0};
    v2 YAxis = {0, 100.0f};
#endif
    uint32_t PIndex = 0;
    real32 CAngle = 5.0f*Angle;
#if 0
    v4 Color = V4(0.5f+0.5f*Sin(CAngle),
                  0.5f+0.5f*Sin(2.9f*CAngle),
                  0.5f+0.5f*Cos(9.9f*CAngle),
                  0.5f+0.5f*Sin(10.0f*CAngle));
#else
    v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
    CoordinateSystem(RenderGroup, Disp + Origin - 0.5f*XAxis - 0.5f*YAxis,
                     XAxis, YAxis, Color,
                     &GameState->TestDiffuse,
                     &GameState->TestNormal,
                     TranState->EnvMaps + 2,
                     TranState->EnvMaps + 1,
                     TranState->EnvMaps + 0);
    v2 MapP = {0.0f, 0.0f};
    for(uint32_t MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex)
    {
        environment_map *Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap *LOD = Map->LOD + 0;

        XAxis = 0.5f * V2((real32)LOD->Width, 0.0f);
        YAxis = 0.5f * V2(0.0f, (real32)LOD->Height);

        CoordinateSystem(RenderGroup, MapP, XAxis, YAxis, V4(1.0f, 1.0f, 1.0f, 1.0f), LOD, 0, 0, 0, 0);
        MapP += YAxis + V2(0.0f, 6.0f);
    }
#endif

    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);
    EndRender(RenderGroup);

    // TODO: Make sure we hoist the camera update out to a place where the
    // renderer can know about the location of the camera at the end of the
    // frame so there isn't a frame of lag in camera updating compared to the
    // main protagonist.

    EndSim(SimRegion, GameState);
    EndTemporaryMemory(SimMemory);
    EndTemporaryMemory(RenderMemory);

    CheckArena(&GameState->WorldArena);
    CheckArena(&TranState->TranArena);

    OverlayCycleCounters(Memory);

    if(DEBUGRenderGroup)
    {
        TiledRenderGroupToOutput(TranState->HighPriorityQueue, DEBUGRenderGroup, DrawBuffer);
        EndRender(DEBUGRenderGroup);
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    transient_state *TranState = (transient_state *)Memory->TransientStorage;

    OutputPlayingSounds(&GameState->AudioState, SoundBuffer, TranState->Assets, &TranState->TranArena);
}

debug_record DebugRecordArray[__COUNTER__];

internal void
UpdateDebugRecords(debug_state *DebugState, u32 CounterCount, debug_record *Counters)
{
    for(u32 CounterIndex = 0; CounterIndex < CounterCount; ++CounterIndex)
    {
        debug_record *Source = Counters + CounterIndex;
        debug_counter_state *Dest = DebugState->CounterStates + DebugState->CounterCount++;

        u64 HitCount_CycleCount = AtomicExchangeU64(&Source->HitCount_CycleCount, 0);
        Dest->Filename = Source->Filename;
        Dest->FunctionName = Source->FunctionName;
        Dest->LineNumber = Source->LineNumber;
        Dest->Snapshots[DebugState->SnapshotIndex].HitCount = (u32)(HitCount_CycleCount >> 32);
        Dest->Snapshots[DebugState->SnapshotIndex].CycleCount = (u32)(HitCount_CycleCount &0xFFFFFFFF);
    }
}

extern u32 const DebugRecords_Optimised_Count;
debug_record DebugRecords_Optimised[];

extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    if(DebugState)
    {
        DebugState->CounterCount = 0;
        UpdateDebugRecords(DebugState, DebugRecords_Optimised_Count, DebugRecords_Optimised);
        UpdateDebugRecords(DebugState, ArrayCount(DebugRecords_Main), DebugRecords_Main);

        ++DebugState->SnapshotIndex;
        if(DebugState->SnapshotIndex >= DEBUG_SNAPSHOT_COUNT)
        {
            DebugState->SnapshotIndex = 0;
        }
    }
}

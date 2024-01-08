#if !defined(GAME_WORLD_MODE_H)

struct game_mode_world;

struct low_entity
{
    world_position P;
    sim_entity Sim;
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32_t StorageIndexA;
    uint32_t StorageIndexB;

    pairwise_collision_rule *NextInHash;
};
struct game_state;
internal void AddCollisionRule(game_mode_world *WorldMode, uint32_t StorageIndexA, uint32_t StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_mode_world *WorldMode, uint32_t StorageIndex); 

struct particle_cel
{
    r32 Density;
    v3 VelocityTimesDensity;
};
struct particle
{
    bitmap_id BitmapID;
    v3 P;
    v3 dP;
    v3 ddP;
    v4 Color;
    v4 dColor;
};

struct game_mode_world
{
    world *World;
    real32 TypicalFloorHeight;

    // TODO: Allow split-screen?
    uint32_t CameraFollowingEntityIndex;
    world_position CameraP;
    world_position LastCameraP;

    uint32_t LowEntityCount;
    low_entity LowEntities[100000];

    // Must be a power of two.
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *HeroHeadCollision;
    sim_entity_collision_volume_group *HeroBodyCollision;
    sim_entity_collision_volume_group *MonsterCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *FloorCollision;

    real32 Time;

    random_series EffectsEntropy; // This is entropy that doesn't affect the gameplay.
    real32 tSine;

#define PARTICLE_CEL_DIM 32
    u32 NextParticle;
    particle Particles[256];

    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
};

inline low_entity *
GetLowEntity(game_mode_world *WorldMode, uint32_t Index)
{
    low_entity *Result = 0;

    if((Index > 0) && (Index < WorldMode->LowEntityCount))
    {
        Result = WorldMode->LowEntities + Index;
    }

    return(Result);
}

internal void PlayWorld(game_state *GameState, transient_state *TranState);

#define GAME_WORLD_MODE_H
#endif

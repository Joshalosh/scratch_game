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
    entity_id CameraFollowingEntityIndex;
    world_position CameraP;
    world_position LastCameraP;

    // TODO: Must be a power of two.
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
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

    b32 CreationBufferLocked; // TODO: Remove this eventually, just for catching bugs
    low_entity CreationBuffer;
    u32 LastUsedEntityStorageIndex; // TODO: Worry about this wrapping - free list for IDs?

    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
};

internal void PlayWorld(game_state *GameState, transient_state *TranState);

#define GAME_WORLD_MODE_H
#endif


struct game_mode_world;

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32_t IDA;
    uint32_t IDB;

    pairwise_collision_rule *NextInHash;
};
struct game_state;
internal void AddCollisionRule(game_mode_world *WorldMode, uint32_t StorageIndexA, uint32_t StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_mode_world *WorldMode, uint32_t StorageIndex); 

#if 0
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
#endif

struct particle_cache;
struct game_mode_world
{
    world *World;
    real32 TypicalFloorHeight;

    // TODO: Allow split-screen?
    entity_id CameraFollowingEntityIndex;
    world_position LastCameraP;
    world_position CameraP;
    v3 CameraOffset;

    // TODO: Must be a power of two.
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    entity_collision_volume_group *NullCollision;
    entity_collision_volume_group *StairCollision;
    entity_collision_volume_group *HeroHeadCollision;
    entity_collision_volume_group *HeroBodyCollision;
    entity_collision_volume_group *HeroGloveCollision;
    entity_collision_volume_group *MonsterCollision;
    entity_collision_volume_group *FamiliarCollision;
    entity_collision_volume_group *WallCollision;
    entity_collision_volume_group *FloorCollision;

    real32 Time;

    random_series GameEntropy; // NOTE: This is entropy that does affect the gameplay
    random_series EffectsEntropy; // NOTE: This is entropy that doesn't affect the gameplay.
    real32 tSine;

    sim_region *CreationRegion;
    u32 LastUsedEntityStorageIndex; // TODO: Worry about this wrapping - free list for IDs?

    particle_cache *ParticleCache;

#if 0
#define PARTICLE_CEL_DIM 32
    u32 NextParticle;
    particle Particles[256];
    particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];
#endif
};

struct world_sim
{
    sim_region *SimRegion;
    temporary_memory SimMemory;
    v3 CameraP;
};

internal void PlayWorld(game_state *GameState, transient_state *TranState);

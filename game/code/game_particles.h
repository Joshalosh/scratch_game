
#define MAX_PARTICLE_COUNT 4096
#define PARTICLE_SYSTEM_COUNT 64

struct particle_system_info
{
    entity_id ID;
    u32 FramesSinceTouched;
};
struct particle_system
{
    f32 X[MAX_PARTICLE_COUNT];
    f32 Y[MAX_PARTICLE_COUNT];
    f32 Z[MAX_PARTICLE_COUNT];
    f32 R[MAX_PARTICLE_COUNT];
    f32 G[MAX_PARTICLE_COUNT];
    f32 B[MAX_PARTICLE_COUNT];
    f32 A[MAX_PARTICLE_COUNT];
    f32 t[MAX_PARTICLE_COUNT];
};

struct particle_cache
{
    particle_system_info SystemInfos[PARTICLE_SYSTEM_COUNT];
    particle_system Systems[PARTICLE_SYSTEM_COUNT];
};

struct particle_spec
{
    f32 Something;
};

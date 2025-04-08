
#define MAX_PARTICLE_COUNT 128
#define MAX_PARTICLE_COUNT_4 (MAX_PARTICLE_COUNT/4)

struct particle_4x
{
    v3_4x P;
    v3_4x dP;
    v3_4x ddP;
    v4_4x C;
    v4_4x dC;
};

struct particle_system
{
    particle_4x Particles[MAX_PARTICLE_COUNT_4];
    u32 NextParticle4;
    bitmap_id BitmapID;
};

struct particle_cache
{
    random_series ParticleEntropy; // Not for gameplay ever
    particle_system FireSystem;
};

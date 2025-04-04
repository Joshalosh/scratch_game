
#define MAX_PARTICLE_COUNT 4096
#define MAX_PARTICLE_COUNT_4 (MAX_PARTICLE_COUNT/4)
#define PARTICLE_SYSTEM_COUNT 64

struct particle_system_info
{
    entity_id ID;
    u32 FramesSinceTouched;
};
struct particle_system
{
    __m128 Px[MAX_PARTICLE_COUNT_4];
    __m128 Py[MAX_PARTICLE_COUNT_4];
    __m128 Pz[MAX_PARTICLE_COUNT_4];

    __m128 dPx[MAX_PARTICLE_COUNT_4];
    __m128 dPy[MAX_PARTICLE_COUNT_4];
    __m128 dPz[MAX_PARTICLE_COUNT_4];

    __m128 ddPx[MAX_PARTICLE_COUNT_4];
    __m128 ddPy[MAX_PARTICLE_COUNT_4];
    __m128 ddPz[MAX_PARTICLE_COUNT_4];

    __m128 Cr[MAX_PARTICLE_COUNT_4];
    __m128 Cg[MAX_PARTICLE_COUNT_4];
    __m128 Cb[MAX_PARTICLE_COUNT_4];
    __m128 Ca[MAX_PARTICLE_COUNT_4];

    __m128 dCr[MAX_PARTICLE_COUNT_4];
    __m128 dCg[MAX_PARTICLE_COUNT_4];
    __m128 dCb[MAX_PARTICLE_COUNT_4];
    __m128 dCa[MAX_PARTICLE_COUNT_4];

    u32 NextParticle4;
};

struct v3_4x
{
    __m128 x;
    __m128 y;
    __m128 z;
};

struct v4_4x
{
    __m128 r;
    __m128 g;
    __m128 b;
    __m128 a;
};

struct particle_v3
{
    __m128 x;
    __m128 Ignored0[MAX_PARTICLE_COUNT_4 - 1];

    __m128 y;
    __m128 Ignored1[MAX_PARTICLE_COUNT_4 - 1];

    __m128 z;
    __m128 Ignored2[MAX_PARTICLE_COUNT_4 - 1];
};

particle_v3 &operator+=(particle_v3 &A, v3_4x B)
{
    A.x = _mm_add_ps(A.x, B.x);
    A.y = _mm_add_ps(A.y, B.y);
    A.z = _mm_add_ps(A.z, B.z);

    return(A);
}

inline v3_4x 
operator*(f32 As, v3_4x B)
{
    v3_4x Result;

    __m128 A = _mm_set1_ps(As);
    Result.x = _mm_mul_ps(A, B.x);
    Result.y = _mm_mul_ps(A, B.y);
    Result.z = _mm_mul_ps(A, B.z);

    return(Result);
}

inline v3_4x
operator+(v3_4x A, v3_4x B)
{
    v3_4x Result;

    Result.x = _mm_add_ps(A.x, B.x);
    Result.y = _mm_add_ps(A.y, B.y);
    Result.z = _mm_add_ps(A.z, B.z);

    return(Result);
}

inline v3_4x 
operator*(f32 As, particle_v3 B)
{
    v3_4x Result;

    __m128 A = _mm_set1_ps(As);
    Result.x = _mm_mul_ps(A, B.x);
    Result.y = _mm_mul_ps(A, B.y);
    Result.z = _mm_mul_ps(A, B.z);

    return(Result);
}

struct particle_v4
{
    __m128 r;
    __m128 Ignored0[MAX_PARTICLE_COUNT_4 - 1];

    __m128 g;
    __m128 Ignored1[MAX_PARTICLE_COUNT_4 - 1];

    __m128 b;
    __m128 Ignored2[MAX_PARTICLE_COUNT_4 - 1];

    __m128 a;
    __m128 Ignored3[MAX_PARTICLE_COUNT_4 - 1];
};

struct particle 
{
    particle_v3 *P;
    particle_v3 *dP;
    particle_v3 *ddP;
    particle_v4 *C;
    particle_v4 *dC;
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

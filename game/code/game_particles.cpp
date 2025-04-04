#define MMSetExpr(Expr) _mm_set_ps(Expr, Expr, Expr, Expr)

inline __m128
operator+(__m128 A, __m128 B)
{
    __m128 Result = _mm_add_ps(A, B);
    return(Result);
}

inline __m128
operator*(f32 A, __m128 B)
{
    __m128 Result = _mm_mul_ps(_mm_set1_ps(A), B);
    return(Result);
}

inline void 
GetParticle(particle_system *System, u32 Ai, particle *Result)
{
    Result->P = (particle_v3 *)(System->Px + Ai);
    Result->dP = (particle_v3 *)(System->dPx + Ai);
    Result->ddP = (particle_v3 *)(System->ddPx + Ai);
    Result->C = (particle_v4 *)(System->Cr + Ai);
    Result->dC = (particle_v4 *)(System->dCr + Ai);
}

internal void
PlayAround(particle_system *System, random_series *Entropy, f32 dt)
{
    {
        u32 Ai = System->NextParticle4++;
        if(System->NextParticle4 >= MAX_PARTICLE_COUNT_4)
        {
            System->NextParticle4 = 0;
        }

        particle A;
        GetParticle(System, Ai, &A);

        A.P->x = MMSetExpr(RandomBetween(Entropy, -0.05f, 0.05f));
        A.P->y = MMSetExpr(0.0f);
        A.P->z = MMSetExpr(0.0f);

        A.dP->x = MMSetExpr(RandomBetween(Entropy, -0.01f, 0.01f));
        A.dP->y = MMSetExpr(7.0f*RandomBetween(Entropy, 0.7f, 1.0f));
        A.dP->z = MMSetExpr(0.0f);

        A.ddP->x = MMSetExpr(0.0f);
        A.ddP->y = MMSetExpr(-9.8f);
        A.ddP->z = MMSetExpr(0.0f);

        A.C->r = MMSetExpr(RandomBetween(Entropy, 0.75, 1.0f));
        A.C->g = MMSetExpr(RandomBetween(Entropy, 0.75, 1.0f));
        A.C->b = MMSetExpr(RandomBetween(Entropy, 0.75, 1.0f));
        A.C->a = MMSetExpr(1.0f);

        A.dC->r = MMSetExpr(0.0f);
        A.dC->g = MMSetExpr(0.0f);
        A.dC->b = MMSetExpr(0.0f);
        A.dC->a = MMSetExpr(-0.25f);
    }

#if 0
    // Particle system test.
    ZeroStruct(WorldMode->ParticleCels);

    r32 GridScale = 0.25f;
    r32 InvGridScale = 1.0f / GridScale;
    v3 GridOrigin = {-0.5f*GridScale*PARTICLE_CEL_DIM, 0.0f, 0.0f};
    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(WorldMode->Particles); ++ParticleIndex)
    {
        particle *Particle = WorldMode->Particles + ParticleIndex;

        v3 P = InvGridScale*(Particle->P - GridOrigin);

        s32 X = TruncateReal32ToInt32(P.x);
        s32 Y = TruncateReal32ToInt32(P.y);

        if(X < 0) {X = 0;}
        if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
        if(Y < 0) {Y = 0;}
        if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}

        particle_cel *Cel = &WorldMode->ParticleCels[Y][X];
        r32 Density = Particle->Color.a;
        Cel->Density += Density;
        Cel->VelocityTimesDensity += Density*Particle->dP;
    }

    if(Global_Particles_ShowGrid)
    {
        for(u32 Y = 0; Y < PARTICLE_CEL_DIM; ++Y)
        {
            for(u32 X = 0; X < PARTICLE_CEL_DIM; ++X)
            {
                particle_cel *Cel = &WorldMode->ParticleCels[Y][X];
                r32 Alpha = Clamp01(0.1f*Cel->Density);
                PushRect(RenderGroup, EntityTransform, GridScale*V3((r32)X, (r32)Y, 0) + GridOrigin, GridScale*V2(1.0f, 1.0f),
                         V4(Alpha, Alpha, Alpha, 1.0f));
            }
        }
    }
#endif

    for(u32 Ai = 0; Ai < MAX_PARTICLE_COUNT_4; ++Ai)
    {
#if 0
        particle *Particle = WorldMode->Particles + ParticleIndex;

        v3 P = InvGridScale*(Particle->P - GridOrigin);

        s32 X = TruncateReal32ToInt32(P.x);
        s32 Y = TruncateReal32ToInt32(P.y);

        if(X < 1) {X = 1;}
        if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
        if(Y < 1) {Y = 1;}
        if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}

        particle_cel *CelCentre = &WorldMode->ParticleCels[Y][X];
        particle_cel *CelLeft = &WorldMode->ParticleCels[Y][X - 1];
        particle_cel *CelRight = &WorldMode->ParticleCels[Y][X + 1];
        particle_cel *CelDown = &WorldMode->ParticleCels[Y - 1][X];
        particle_cel *CelUp = &WorldMode->ParticleCels[Y + 1][X];

        v3 Dispersion = {};
        r32 Dc = 1.0f;
        Dispersion += Dc*(CelCentre->Density - CelLeft->Density)*V3(-1.0f, 0.0f, 0.0f);
        Dispersion += Dc*(CelCentre->Density - CelRight->Density)*V3(1.0f, 0.0f, 0.0f);
        Dispersion += Dc*(CelCentre->Density - CelDown->Density)*V3(0.0f, -1.0f, 0.0f);
        Dispersion += Dc*(CelCentre->Density - CelUp->Density)*V3(0.0f, 1.0f, 0.0f);

        v3 ddP = Particle->ddP + Dispersion;
#endif
        particle A;
        GetParticle(System, Ai, &A);

        // Simulate the particle forward in time
        *A.P += (0.5f*Square(dt)*(*A.ddP) + dt*(*A.dP));
        *A.dP += dt*(*A.ddP);
#if 0
        *A.C += dt*(*A.dC);
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
        PushBitmap(RenderGroup, EntityTransform, Particle->BitmapID, 1.0f, Particle->P, Color);
#endif
    }
}

internal void
InitParticleSystem(particle_cache *Cache, particle_system *System,
                   entity_id ID, particle_spec *Spec)
{
}
        
internal particle_system *
GetOrCreateParticleSystem(particle_cache *Cache, entity_id ID, 
                          particle_spec *Spec, b32 CreateIfNotFound)
{
    particle_system *Result = 0;

#if 0
    u32 MaxFramesSinceTouched = 0;
    particle_system *Replace = 0;
    for(u32 SystemIndex = 0; SystemIndex < ArrayCount(Cache->SystemInfos); ++SystemIndex)
    {
        particle_system_info *SystemInfos = Cache->SystemInfos + SystemIndex;
        if(IsEqual(ID, SystemInfos->ID))
        {
            Result = Cache->Systems + SystemIndex;
            break;
        }

        if(MaxFramesSinceTouched < SystemInfos->FramesSinceTouched)
        {
            MaxFramesSinceTouched = SystemInfos->FramesSinceTouched;
            Replace = Cache->Systems + SystemIndex;
        }
    }

    if(CreateIfNotFound && !Result)
    {
        Result = Replace;

        InitParticleSystem(Cache, Result, ID, Spec);
    }
#endif

    return(Result);
}

internal void
TouchParticleSystem(particle_cache *ParticleCache, particle_system *System)
{
#if 0
    if(System)
    {
        u32 Index = (u32)(System - ParticleCache->Systems);
        Assert(Index < ArrayCount(ParticleCache->SystemInfos));
        particle_system_info *Info = ParticleCache->SystemInfos + Index;

        Info->FramesSinceTouched = 0;
    }
#endif
}

internal void
UpdateAndRenderParticleSystems(particle_cache *Cache, f32 dt, render_group *RenderGroup)
{
    Assert(ArrayCount(Cache->Systems) == ArrayCount(Cache->SystemInfos));
    for(u32 SystemIndex = 0; SystemIndex < ArrayCount(Cache->Systems); ++SystemIndex)
    {
        particle_system *System = Cache->Systems + SystemIndex;
        particle_system_info *Info = Cache->SystemInfos + SystemIndex;

        // TODO: Implement
        
        ++Info->FramesSinceTouched;
    }
}

internal void
InitParticleCache(particle_cache *Cache)
{
    for(u32 SystemIndex = 0; SystemIndex < ArrayCount(Cache->Systems); ++SystemIndex)
    {
        particle_system_info *Info = Cache->SystemInfos + SystemIndex;
        Info->ID.Value = 0;
        Info->FramesSinceTouched = 0;
    }
}

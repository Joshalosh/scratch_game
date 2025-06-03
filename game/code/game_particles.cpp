#define MMSetExpr(Expr) _mm_set_ps(Expr, Expr, Expr, Expr)

internal void
SpawnFire(particle_cache *Cache, v3 AtPInit, s32 ChunkZ, r32 FloorZ)
{
    if(Cache)
    {
        particle_system *System = &Cache->FireSystem;
        random_series *Entropy = &Cache->ParticleEntropy;

        v3_4x AtP = ToV34x(AtPInit);

        u32 ParticleIndex4 = System->NextParticle4++;
        if(System->NextParticle4 >= MAX_PARTICLE_COUNT_4)
        {
            System->NextParticle4 = 0;
        }

        particle_4x *A = System->Particles + ParticleIndex4;

        A->P.x = MMSetExpr(RandomBetween(Entropy, -0.05f, 0.05f));
        A->P.y = MMSetExpr(0.0f);
        A->P.z = MMSetExpr(0.0f);
        A->P += AtP;

        A->dP.x = MMSetExpr(RandomBetween(Entropy, -0.01f, 0.01f));
        A->dP.y = MMSetExpr(7.0f*RandomBetween(Entropy, 0.7f, 1.0f));
        A->dP.z = MMSetExpr(0.0f);

        A->ddP.x = MMSetExpr(0.0f);
        A->ddP.y = MMSetExpr(-9.8f);
        A->ddP.z = MMSetExpr(0.0f);

        A->C.r = MMSetExpr(RandomBetween(Entropy, 0.75, 1.0f));
        A->C.g = MMSetExpr(RandomBetween(Entropy, 0.75, 1.0f));
        A->C.b = MMSetExpr(RandomBetween(Entropy, 0.75, 1.0f));
        A->C.a = MMSetExpr(1.0f);

        A->dC.r = MMSetExpr(0.0f);
        A->dC.g = MMSetExpr(0.0f);
        A->dC.b = MMSetExpr(0.0f);
        A->dC.a = MMSetExpr(-1.0f);

        A->FloorZ = FloorZ;
        A->ChunkZ = ChunkZ;
    }
}

internal void
UpdateAndRenderFire(particle_system *System, random_series *Entropy, f32 dt, v3 FrameDisplacementInit,
                    render_group *RenderGroup, v3 CameraP)
{
    object_transform Transform = DefaultUprightTransform();
    v3_4x FrameDisplacement = ToV34x(FrameDisplacementInit);

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

    for(u32 ParticleIndex4 = 0; ParticleIndex4 < MAX_PARTICLE_COUNT_4; ++ParticleIndex4)
    {
        particle_4x *A = System->Particles + ParticleIndex4;
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

        // Simulate the particle forward in time
        A->P += 0.5f*Square(dt)*A->ddP + dt*A->dP + FrameDisplacement;
        A->dP += dt*A->ddP;
        A->C += dt*A->dC;

#if 0
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
#endif

        // Render the particle.
        Transform.ChunkZ = A->ChunkZ;
        Transform.FloorZ = A->FloorZ - CameraP.z;
        for(u32 SubIndex = 0; SubIndex < 4; ++SubIndex)
        {
            v3 P =
            {
                M(A->P.x, SubIndex),
                M(A->P.y, SubIndex),
                M(A->P.z, SubIndex),
            };

            v4 C =
            {
                M(A->C.r, SubIndex),
                M(A->C.g, SubIndex),
                M(A->C.b, SubIndex),
                M(A->C.a, SubIndex),
            };

            Transform.OffsetP = -CameraP;
            if(C.a > 0)
            {
                PushBitmap(RenderGroup, &Transform, System->BitmapID, 1.0f, P, C);
            }
        }
    }
}

internal void
UpdateAndRenderParticleSystems(particle_cache *Cache, f32 dt, render_group *RenderGroup,
                               v3 FrameDisplacement, v3 CameraP)
{
    TIMED_FUNCTION();
    UpdateAndRenderFire(&Cache->FireSystem, &Cache->ParticleEntropy, dt, FrameDisplacement,
                        RenderGroup, CameraP);
}

internal void
InitParticleCache(particle_cache *Cache, game_assets *Assets)
{
    ZeroStruct(*Cache);
    Cache->ParticleEntropy = RandomSeed(1234);

    asset_vector MatchVector = {};
    MatchVector.E[Tag_FacingDirection] = 0.0f;
    asset_vector WeightVector = {};
    WeightVector.E[Tag_FacingDirection] = 1.0f;

    Cache->FireSystem.BitmapID = GetBestMatchBitmapFrom(Assets, Asset_Head, &MatchVector, &WeightVector);
}

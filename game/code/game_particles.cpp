
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

#if !defined(GAME_ENTITY_H)

#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

inline bool32
IsSet(sim_entity *Entity, uint32_t Flag)
{
    bool32 Result = Entity->Flags & Flag;

    return(Result);
}

inline void 
AddFlag(sim_entity *Entity, uint32_t Flag)
{
    Entity->Flags |= Flag;
}

inline void
ClearFlag(sim_entity *Entity, uint32_t Flag)
{
    Entity->Flags &= ~Flag;
}

inline void
MakeEntityNonspatial(sim_entity *Entity)
{
    AddFlag(Entity, EntityFlag_Nonspatial);
    Entity->P = InvalidP;
}

inline void
MakeEntitySpatial(sim_entity *Entity, v3 P, v3 dP)
{
    ClearFlag(Entity, EntityFlag_Nonspatial);
    Entity->P = P;
    Entity->dP = dP;
}

#define GAME_ENTITY_H
#endif

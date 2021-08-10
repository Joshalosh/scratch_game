#if !defined(GAME_ENTITY_H)

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

#define GAME_ENTITY_H
#endif

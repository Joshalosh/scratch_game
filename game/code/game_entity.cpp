
inline move_spec
DefaultMoveSpec(void)
{
    move_spec Result;

    Result.UnitMaxAccelVector = false;
    Result.Speed = 1.0f;
    Result.Drag  = 0.0f;

    return(Result);
}

inline void
UpdateFamiliar(game_state *GameState, entity Entity, real32 dt)
{
    sim_entity *ClosestHero = 0;
    real32 ClosestHeroDSq = Square(10.0f);

    sim_entity *TestEntity = SimRegion->Entities;
    for(uint32_t TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex)
    {
        if(TestEntity->Type == EntityType_Hero)
        {
            real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
            if(TestEntity->Type == EntityType_Hero)
            {
                TestDSq *= 0.75f;
            }

            if(ClosestHeroDSq > TestDSq)
            {
                ClosestHero = TestEntity;
                ClosestHeroDSq = TestDSq;
            }
        }
    }

    v2 ddP = {};
    if(ClosestHero && (ClosestHeroDSq > Square(3.0f)))
    {
        real32 Acceleration = 0.5f;
        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
        ddP = OneOverLength*(ClosestHero->P - Entity->P);
    }

    move_spec MoveSpec = DefaultMoveSpec();
    MoveSpec.UnitMaxAccelVector = true;
    MoveSpec.Speed = 50.0f;
    MoveSpec.Drag  =  9.0f;
    MoveEntity(GameState, Entity, dt, &MoveSpec, ddP);
}

inline void
UpdateMonster(sim_region *SimRegion, sim_entity *Entity, real32 dt)
{
}

inline void
UpdateSword(sim_region *SimRegion, sim_entity *Entity, real32 dt)
{
    move_spec MoveSpec = DefaultMoveSpec();
    MoveSpec.UnitMaxAccelVector = false;
    MoveSpec.Speed = 0.0f;
    MoveSpec.Drag  = 0.0f;

    v2 OldP = Entity->P;
    MoveEntity(GameState, Entity, dt, &MoveSpec, V2(0, 0));
    real32 DistanceTravelled = Length(Entity->P - OldP);

    Entity->DistanceRemaining -= DistanceTravelled;
    if(Entity->DistanceRemaining < 0.0f)
    {
        Assert(!"NEED TO MAKE ENTITES BE ABLE TO NOT BE THERE!");
    }
}


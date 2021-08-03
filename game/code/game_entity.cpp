
inline void
UpdateFamiliar(game_state *GameState, entity Entity, real32 dt)
{
    entity ClosestHero = {};
    real32 ClosestHeroDSq = Square(10.0f);
    for(uint32_t HighEntityIndex = 1;
        HighEntityIndex < GameState->HighEntityCount;
        ++HighEntityIndex)
    {
        entity TestEntity = EntityFromHighIndex(GameState, HighEntityIndex);

        if(TestEntity.Low->Sim.Type == EntityType_Hero)
        {
            real32 TestDSq = LengthSq(TestEntity.High->P - Entity.High->P);
            if(TestEntity.Low->Sim.Type == EntityType_Hero)
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
    if(ClosestHero.High && (ClosestHeroDSq > Square(3.0f)))
    {
        real32 Acceleration = 0.5f;
        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
        ddP = OneOverLength*(ClosestHero.High->P - Entity.High->P);
    }

    move_spec MoveSpec = DefaultMoveSpec();
    MoveSpec.UnitMaxAccelVector = true;
    MoveSpec.Speed = 50.0f;
    MoveSpec.Drag  =  9.0f;
    MoveEntity(GameState, Entity, dt, &MoveSpec, ddP);
}

inline void
UpdateMonster(game_state *GameState, entity Entity, real32 dt)
{
}

inline void
UpdateSword(game_state *GameState, entity Entity, real32 dt)
{
    move_spec MoveSpec = DefaultMoveSpec();
    MoveSpec.UnitMaxAccelVector = false;
    MoveSpec.Speed = 0.0f;
    MoveSpec.Drag  = 0.0f;

    v2 OldP = Entity.High->P;
    MoveEntity(GameState, Entity, dt, &MoveSpec, V2(0, 0));
    real32 DistanceTravelled = Length(Entity.High->P - OldP);

    Entity.Low->Sim.DistanceRemaining -= DistanceTravelled;
    if(Entity.Low->Sim.DistanceRemaining < 0.0f)
    {
        ChangeEntityLocation(&GameState->WorldArena, GameState->World,
                             Entity.LowIndex, Entity.Low, &Entity.Low->P, 0);
    }
}


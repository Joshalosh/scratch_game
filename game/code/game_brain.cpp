
inline void
MarkBrainActives(brain *Brain)
{
    u32 BrainFlags = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_BRAIN_SLOT_COUNT; ++SlotIndex)
    {
        entity *Entity = GetEntityInSlot(Brain, SlotIndex);
        if(Entity)
        {
            BrainFlags |= Entity->Flags;
        }
    }

    if(BrainFlags & EntityFlag_Active)
    {
        for(u32 SlotIndex = 0; SlotIndex < MAX_BRAIN_SLOT_COUNT; ++SlotIndex)
        {
            entity *Entity = GetEntityInSlot(Brain, SlotIndex);
            if(Entity)
            {
                Entity->Flags |= EntityFlag_Active;
            }
        }
    }
}

inline void
ExecuteBrain(game_state *GameState, game_mode_world *WorldMode, game_input *Input, 
             sim_region *SimRegion, brain *Brain, r32 dt)
{
    switch(Brain->Type)
    {
        case Type_brain_hero:
        {
            brain_hero *Parts = &Brain->Hero;
            entity *Head = Parts->Head;
            entity *Body = Parts->Body;
            entity *Glove = Parts->Glove;

            v2 dSword = {};
            r32 dZ = 0.0f;
            b32 Exited = false;
            b32 DebugSpawn = false;
            b32 Attacked = false;

            if(Input)
            {
                u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstHero;
                game_controller_input *Controller = GetController(Input, ControllerIndex);
                controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
                if(Controller->IsAnalogue)
                {
                    // Use analogue movement tuning
                    ConHero->ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
                }
                else
                {
                    // Use digital movement tuning
                    r32 Recentre = 0.5f;
                    if(WasPressed(Controller->MoveUp))
                    {
                        ConHero->ddP.x = 0.0f;
                        ConHero->ddP.y = 1.0f;
                        ConHero->RecentreTimer = Recentre;
                    }
                    if(WasPressed(Controller->MoveDown))
                    {
                        ConHero->ddP.x = 0.0f;
                        ConHero->ddP.y = -1.0f;
                        ConHero->RecentreTimer = Recentre;
                    }
                    if(WasPressed(Controller->MoveLeft))
                    {
                        ConHero->ddP.x = -1.0f;
                        ConHero->ddP.y = 0.0f;
                        ConHero->RecentreTimer = Recentre;
                    }
                    if(WasPressed(Controller->MoveRight))
                    {
                        ConHero->ddP.x = 1.0f;
                        ConHero->ddP.y = 0.0f;
                        ConHero->RecentreTimer = Recentre;
                    }

                    if(!IsDown(Controller->MoveLeft) &&
                       !IsDown(Controller->MoveRight))
                    {
                        ConHero->ddP.x = 0.0f;
                        if(IsDown(Controller->MoveUp))
                        {
                            ConHero->ddP.y = 1.0f;
                        }
                        if(IsDown(Controller->MoveDown))
                        {
                            ConHero->ddP.y = -1.0f;
                        }
                    }

                    if(!IsDown(Controller->MoveUp) &&
                       !IsDown(Controller->MoveDown))
                    {
                        ConHero->ddP.y = 0.0f;
                        if(IsDown(Controller->MoveLeft))
                        {
                            ConHero->ddP.x = -1.0f;
                        }
                        if(IsDown(Controller->MoveRight))
                        {
                            ConHero->ddP.x = 1.0f;
                        }
                    }

                    if(WasPressed(Controller->Start))
                    {
                        DebugSpawn = true;
                    }
                }

                if(Head && WasPressed(Controller->Start))
                {
                    entity *ClosestHero = 0;
                    r32 ClosestHeroDSq = Square(10.0f);
                    entity *TestEntity = SimRegion->Entities;
                    for(u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity)
                    {
                        if((TestEntity->BrainID.Value != Head->BrainID.Value) && TestEntity->BrainID.Value)
                        {
                            r32 TestDSq = LengthSq(TestEntity->P - Head->P);
                            if(ClosestHeroDSq > TestDSq)
                            {
                                ClosestHero = TestEntity;
                                ClosestHeroDSq = TestDSq;
                            }
                        }
                    }

                    if(ClosestHero)
                    {
                        brain_id OldBrainID = Head->BrainID;
                        brain_slot OldBrainSlot = Head->BrainSlot;
                        Head->BrainID = ClosestHero->BrainID;
                        Head->BrainSlot = ClosestHero->BrainSlot;
                        ClosestHero->BrainID = OldBrainID;
                        ClosestHero->BrainSlot = OldBrainSlot;
                    }
                }

                if(Controller->ActionUp.EndedDown)
                {
                    Attacked = true;
                    dSword = V2(0.0f, 1.0f);
                }
                if(Controller->ActionDown.EndedDown)
                {
                    Attacked = true;
                    dSword = V2(0.0f, -1.0f);
                }
                if(Controller->ActionLeft.EndedDown)
                {
                    Attacked = true;
                    dSword = V2(-1.0f, 0.0f);
                }
                if(Controller->ActionRight.EndedDown)
                {
                    Attacked = true;
                    dSword = V2(1.0f, 0.0f);
                }
            }

            if(Glove && (Glove->MovementMode != MovementMode_AngleOffset))
            {
                Attacked = false;
            }

            if(WasPressed(Controller->Back))
            {
                Exited = true;
            }

            if(Glove && Attacked)
            {
                Glove->tMovement = 0;
                Glove->MovementMode = MovementMode_AngleAttackSwipe;
                Glove->AngleStart = Glove->AngleCurrent;
                Glove->AngleTarget = (Glove->AngleCurrent > 0.0f) ? -0.25f*Tau32 : 0.25f*Tau32;
                Glove->AngleSwipeDistance = 2.0f;
            }

            if(Head)
            {
                // TODO: Change to using the acceleration vector 
                if(Attacked)
                {
                    Head->FacingDirection = ATan2(dSword.y, dSword.x);
                }
            }

            traversable_reference Traversable;
            if(Head && GetClosestTraversable(SimRegion, Head->P, &Traversable))
            {
                if(Body)
                {
                    if(Body->MovementMode == MovementMode_Planted)
                    {
                        if(!IsEqual(Traversable, Body->Occupying))
                        {
                            Body->CameFrom = Body->Occupying;
                            if(TransactionalOccupy(Body, &Body->Occupying, Traversable))
                            {
                                Body->tMovement = 0.0f;
                                Body->MovementMode = MovementMode_Hopping;
                            }
                        }
                    }
                }

                v3 ClosestP = GetSimSpaceTraversable(Body->Occupying).P;

                v3 ddP = V3(ConHero->ddP, 0);
                r32 ddPLength = LengthSq(ddP);
                if(ddPLength > 1.0f)
                {
                    ddP *= (1.0f / SquareRoot(ddPLength));
                }
                r32 MovementSpeed = 30.0f;
                r32 Drag = 8.0f;
                ddP *= MovementSpeed;

                b32 TimerIsUp = (ConHero->RecentreTimer == 0.0f);
                b32 NoPush = (LengthSq(ConHero->ddP) < 0.1f);
                r32 Cp = NoPush ? 300.0f : 25.0f;
                b32 Recentre[3] = {};
                for(u32 E = 0; E < 3; ++E)
                {
#if 1
                    if(NoPush || (TimerIsUp && (Square(ddP.E[E]) < 0.1f)))
#else
                    if(NoPush)
#endif
                    {
                        Recentre[E] = true;
                        ddP.E[E] = Cp*(ClosestP.E[E] - Head->P.E[E]) - 30.0f*Head->dP.E[E];
                    }
                    else
                    {
                        // TODO: ODE here
                        ddP.E[E] += -Drag*Head->dP.E[E];
                    }
                }

                ConHero->RecentreTimer = ClampAboveZero(ConHero->RecentreTimer - dt);

                Head->ddP = ddP;
            }

            if(Body)
            {
                Body->FacingDirection = Head->FacingDirection;
                Body->dP = V3(0, 0, 0);

                if(Body->MovementMode == MovementMode_Planted)
                {
                    Body->P = GetSimSpaceTraversable(Body->Occupying).P;
                    
                    if(Head)
                    {
                        r32 HeadDistance = 0.0f;
                        HeadDistance = Length(Head->P - Body->P);

                        r32 MaxHeadDistance = 0.5f;
                        r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);
                        Body->ddtBob = -20.0f*tHeadDistance;
                    }
                }

                v3 HeadDelta = {};
                if(Head)
                {
                    HeadDelta = Head->P - Body->P;
                }
                Body->FloorDisplace = (0.25f*HeadDelta).xy;
                Body->YAxis = V2(0, 1) + 0.5f*HeadDelta.xy;
            }

            if(Glove && Body)
            {
                Glove->AngleBase = Body->P;
                Glove->FacingDirection = Body->FacingDirection;
            }

            if(Exited)
            {
                DeleteEntity(SimRegion, Head);
                DeleteEntity(SimRegion, Body);
                ConHero->BrainID.Value = 0;
            }
        } break;

        case Type_brain_snake:
        {
            brain_snake *Parts = &Brain->Snake;

            entity *Head = Parts->Segments[0];
            if(Head)
            {
                v3 Delta = {RandomBilateral(&WorldMode->GameEntropy),
                            RandomBilateral(&WorldMode->GameEntropy),
                            0.0f};
                traversable_reference Traversable;
                if(GetClosestTraversable(SimRegion, Head->P + Delta, &Traversable))
                {
                    if(Head->MovementMode == MovementMode_Planted)
                    {
                        if(!IsEqual(Traversable, Head->Occupying))
                        {
                            traversable_reference LastOccupying = Head->Occupying;
                            Head->CameFrom = Head->Occupying;
                            if(TransactionalOccupy(Head, &Head->Occupying, Traversable))
                            {
                                Head->tMovement = 0.0f;
                                Head->MovementMode = MovementMode_Hopping;

                                for(u32 SegmentIndex = 1;
                                    SegmentIndex < ArrayCount(Parts->Segments);
                                    ++SegmentIndex)
                                {
                                    entity *Segment = Parts->Segments[SegmentIndex];
                                    if(Segment)
                                    {
                                        Segment->CameFrom = Segment->Occupying;
                                        TransactionalOccupy(Segment, &Segment->Occupying, LastOccupying);
                                        LastOccupying = Segment->CameFrom;

                                        Segment->tMovement = 0.0f;
                                        Segment->MovementMode = MovementMode_Hopping;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } break;

        case Type_brain_familiar:
        {
            brain_familiar *Parts = &Brain->Familiar;
            entity *Head = Parts->Head;
            if(Head)
            {
                b32 Blocked = true;
                
                traversable_reference Traversable;
                if(GetClosestTraversable(SimRegion, Head->P, &Traversable))
                {
                    if(IsEqual(Traversable, Head->Occupying))
                    {
                        Blocked = false;
                    }
                    else
                    {
                        if(TransactionalOccupy(Head, &Head->Occupying, Traversable))
                        {
                            Blocked = false;
                        }
                    }
                }

                v3 TargetP = GetSimSpaceTraversable(Head->Occupying).P;
                if(!Blocked)
                {
                    closest_entity Closest = GetClosestEntityWithBrain(SimRegion, Head->P, Type_brain_hero);
                    if(Closest.Entity) // && (ClosestHeroDSq > Square(3.0f)))
                    {
                        traversable_reference TargetTraversable;
                        if(GetClosestTraversableAlongRay(SimRegion, Head->P, NOZ(Closest.Delta),
                                                         Head->Occupying, &TargetTraversable))
                        {
                            if(!IsOccupied(TargetTraversable))
                            {
                                TargetP = Closest.Entity->P;
                            }
                        }
                    }
                }

                Head->ddP = 10.0f*(TargetP - Head->P) - 8.0f*(Head->dP);
            }
        } break;

        case Type_brain_floaty_thing_for_now:
        {
            // TODO: Think about what this stuff actually should mean
            //Entity->P.z += 0.05f*Cos(Entity->tBob);
            //Entity->tBob += dt;
        } break;

        case Type_brain_monster:
        {
            brain_monster *Parts = &Brain->Monster;
            entity *Body = Parts->Body;
            if(Body)
            {
                v3 Delta = {RandomBilateral(&WorldMode->GameEntropy), RandomBilateral(&WorldMode->GameEntropy), 0.0f};
                traversable_reference Traversable;
                if(GetClosestTraversable(SimRegion, Body->P + Delta, &Traversable))
                {
                    if(Body->MovementMode == MovementMode_Planted)
                    {
                        if(!IsEqual(Traversable, Body->Occupying))
                        {
                            Body->CameFrom = Body->Occupying;
                            if(TransactionalOccupy(Body, &Body->Occupying, Traversable))
                            {
                                Body->tMovement = 0.0f;
                                Body->MovementMode = MovementMode_Hopping;
                            }
                        }
                    }
                }
            }
        } break;

        InvalidDefaultCase;
    }
}

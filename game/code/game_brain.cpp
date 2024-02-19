
inline void
ExecuteBrain(game_state *GameState, game_input *Input, sim_region *SimRegion, brain *Brain, r32 dt)
{
    switch(Brain->Type)
    {
        case Brain_Hero:
        {
            // TODO: Check that they're not deleted when I do.
            brain_hero_parts *Parts = &Brain->Hero;
            entity *Head = Parts->Head;
            entity *Body = Parts->Body;

            u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstHero;
            game_controller_input *Controller = GetController(Input, ControllerIndex);
            controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;

            v2 dSword = {};
            r32 dZ = 0.0f;
            b32 Exited = false;
            b32 DebugSpawn = false;
            v2 ddP = {};

            if(Controller->IsAnalogue)
            {
                // Use analogue movement tuning
                ddP = V2(Controller->StickAverageX, Controller->StickAverageY);
            }
            else
            {
                // Use digital movement tuning
                r32 Recentre = 0.5f;
                if(WasPressed(Controller->MoveUp))
                {
                    ddP.x = 0.0f;
                    ddP.y = 1.0f;
                    ConHero->RecentreTimer = Recentre;
                }
                if(WasPressed(Controller->MoveDown))
                {
                    ddP.x = 0.0f;
                    ddP.y = -1.0f;
                    ConHero->RecentreTimer = Recentre;
                }
                if(WasPressed(Controller->MoveLeft))
                {
                    ddP.x = -1.0f;
                    ddP.y = 0.0f;
                    ConHero->RecentreTimer = Recentre;
                }
                if(WasPressed(Controller->MoveRight))
                {
                    ddP.x = 1.0f;
                    ddP.y = 0.0f;
                    ConHero->RecentreTimer = Recentre;
                }

                if(!IsDown(Controller->MoveLeft) &&
                   !IsDown(Controller->MoveRight))
                {
                    ddP.x = 0.0f;
                    if(IsDown(Controller->MoveUp))
                    {
                        ddP.y = 1.0f;
                    }
                    if(IsDown(Controller->MoveDown))
                    {
                        ddP.y = -1.0f;
                    }
                }

                if(!IsDown(Controller->MoveUp) &&
                   !IsDown(Controller->MoveDown))
                {
                    ddP.y = 0.0f;
                    if(IsDown(Controller->MoveLeft))
                    {
                        ddP.x = -1.0f;
                    }
                    if(IsDown(Controller->MoveRight))
                    {
                        ddP.x = 1.0f;
                    }
                }

                if(WasPressed(Controller->Start))
                {
                    DebugSpawn = true;
                }
            }

#if 0
            if(Controller->Start.EndedDown)
            {
                ConHero->dZ = 3.0f;
            }
#endif

            dSword = {};
            if(Controller->ActionUp.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 10.0f, V2(1.0f, 1.0f));
                dSword = V2(0.0f, 1.0f);
            }
            if(Controller->ActionDown.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 10.0f, V2(0.0f, 0.0f));
                dSword = V2(0.0f, -1.0f);
            }
            if(Controller->ActionLeft.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 5.0f, V2(1.0f, 0.0f));
                dSword = V2(-1.0f, 0.0f);
            }
            if(Controller->ActionRight.EndedDown)
            {
                ChangeVolume(&GameState->AudioState, GameState->Music, 5.0f, V2(0.0f, 1.0f));
                dSword = V2(1.0f, 0.0f);
            }

            if(WasPressed(Controller->Back))
            {
                Exited = true;
            }

#if 0
            if(ConHero->DebugSpawn && Head)
            {
                traversable_reference Traversable;
                if(GetClosestTraversable(SimRegion, Head->P, &Traversable, 
                                         TraversableSearch_Unoccupied))
                {
                    AddPlayer(WorldMode, SimRegion, Traversable);
                }
                else
                {
                    //TODO: GameUI that tells you there's no safe place
                    // maybe keep trying on subsequent frames?
                }

                ConHero->DebugSpawn = false;
            }
#endif

            ConHero->RecentreTimer = ClampAboveZero(ConHero->RecentreTimer - dt);

            if(Head)
            {
                // TODO: Change to using the acceleration vector 
                if((dSword.x == 0.0f) && (dSword.y == 0.0f))
                {
                    // NOTE: Leave FacingDirection whatever it was
                }
                else
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

                v3 ClosestP = GetSimSpaceTraversable(Traversable).P;

                b32 TimerIsUp = (ConHero->RecentreTimer == 0.0f);
                b32 NoPush = (LengthSq(ddP) < 0.1f);
                r32 Cp = NoPush ? 300.0f : 25.0f;
                v3 ddP2 = {};
                for(u32 E = 0; E < 3; ++E)
                {
#if 1
                    if(NoPush || (TimerIsUp && (Square(ddP.E[E]) < 0.1f)))
#else
                    if(NoPush)
#endif
                    {
                        ddP2.E[E] = Cp*(ClosestP.E[E] - Head->P.E[E]) - 30.0f*Head->dP.E[E];
                    }
                }
                Head->dP += dt*ddP2;
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

            if(Exited)
            {
                DeleteEntity(SimRegion, Head);
                DeleteEntity(SimRegion, Body);
                ConHero->BrainID.Value = 0;
            }
        } break;

        case Brain_Snake:
        {
        } break;

        InvalidDefaultCase;
    }
}

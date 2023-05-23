#if !defined(GAME_DEBUG_VARIABLES_H)

struct debug_variable_definition_context
{
    debug_state *State;
    memory_arena *Arena;

    debug_variable_group *Group;
};

internal debug_variable *
DEBUGAddVariable(debug_variable_definition_context *Context, debug_variable_type Type, char *Name)
{
    debug_variable *Var = PushStruct(Context->Arena, debug_variable);
    Var->Type = Type;
    Var->Name = Name;
    Var->Next = 0;

    debug_variable *Group = Context->Group;
    if(Group)
    {
        if(Group->LastChild)
        {
            Group->LastChild = Group->LastChild->Next = Var;
        }
        else 
        {
            Group->LastChild = Group->FirstChild = Var;
        }
    }

    return(Var);
}

internal void
DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char *Name)
{
}

internal void
DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, b32 Value)
{

}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context *Context)
{
}

internal void
DEBUGCreateVariables(memory_arena *Arena)
{
// TODO: Add _distance_ for the debug camera, so we have a float example
// TODO: Parameterise the fountain.

#define DEBUG_VARIABLE_LISTING(Name) DEBUGAddVariable(&Context, #Name, DEBUGUI_##Name)

    DEBUGBeginVariableGroup(&Context, "Group Chunks");
    DEBUG_VARIABLE_LISTING(GroundChunkOutlines);
    DEBUG_VARIABLE_LISTING(GroundChunkCheckerboards);
    DEBUG_VARIABLE_LISTING(RecomputeGroundChunksOnEXEChange);
    DEBUGEndVariableGroup(&Context);

    DEBUGBeginVariableGroup(&Context, "Particles");
    DEBUG_VARIABLE_LISTING(ParticleTest);
    DEBUG_VARIABLE_LISTING(ParticleGrid);
    DEBUGEndVariableGroup(&Context);

    DEBUGBeginVariableGroup(&Context, "Renderer");
    {
        DEBUG_VARIABLE_LISTING(TestWeirdDrawBufferSize);
        DEBUG_VARIABLE_LISTING(ShowLightingSamples);

        DEBUGBeginVariableGroup(&Context, "Camera");
        {
            DEBUG_VARIABLE_LISTING(UseDebugCamera);
            DEBUG_VARIABLE_LISTING(UseRoomBasedCamera);
        }

        DEBUGEndVariableGroup(&Context);

        DEBUGEndVariableGroup(&Context);
    }

    DEBUG_VARIABLE_LISTING(UseSpaceOutlines);
    DEBUG_VARIABLE_LISTING(FamiliarFollowsHero);

#undef DEBUG_VARIABLE_LISTING
}

#define GAME_DEBUG_VARIABLES_H
#endif

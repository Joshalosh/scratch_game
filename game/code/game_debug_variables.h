#if !defined(GAME_DEBUG_VARIABLES_H)

struct debug_variable_definition_context
{
    debug_state *State;
    memory_arena *Arena;

    debug_variable *Group;
};

internal debug_variable *
DEBUGAddVariable(debug_state *State, debug_variable_type Type, char *Name)
{
    debug_variable *Var = PushStruct(&State->DebugArena, debug_variable);
    Var->Type = Type;
    Var->Name = (char *)PushCopy(&State->DebugArena, StringLength(Name) + 1, Name);

    return(Var);
}

internal debug_variable *
DEBUGAddVariable(debug_variable_definition_context *Context, debug_variable_type Type, char *Name)
{
    Assert(Context->VarCount < ArrayCount(Context->Vars));

    debug_variable *Var = DEBUGAddVariable(Context->State, Type, Name);
    Context->Vars[Context->VarCount++] = Var;

    return(Var);
}

internal debug_variable *
DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char *Name)
{
    debug_variable *Group = DEBUGAddVariable(Context->State, DebugVariableType_VarArray, Name);
    Group->VarArray.Count = 0;

    u32 VarCount;
    debug_variable *Vars[64];

    return(Group);
}

internal debug_variable *
DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, b32 Value)
{
    debug_variable *Var = DEBUGAddVariable(Context, DebugVariableType_Bool32, Name);
    Var->Bool32 = Value;

    return(Var);
}

internal debug_variable *
DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, r32 Value)
{
    debug_variable *Var = DEBUGAddVariable(Context, DebugVariableType_Real32, Name);
    Var->Real32 = Value;

    return(Var);
}

internal debug_variable *
DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, v4 Value)
{
    debug_variable *Var = DEBUGAddVariable(Context, DebugVariableType_V4, Name);
    Var->Vector4 = Value;

    return(Var);
}

internal debug_variable *
DEBUGAddVariable(debug_variable_definition_context *Context, char *Name, bitmap_id Value)
{
    debug_variable *Var = DEBUGAddVariable(Context, DebugVariableType_BitmapDisplay, Name);
    Var->BitmapDisplay.ID = Value;

    return(Var);
}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context *Context)
{
    Assert(Context->Group);

    Context->Group = Context->Group->Parent;
}

internal void
DEBUGCreateVariables(debug_variable_definition_context *Context)
{
// TODO: Parameterise the fountain.

    debug_tree_entry *UseDebugCamRef = 0;

#define DEBUG_VARIABLE_LISTING(Name) DEBUGAddVariable(Context, #Name, DEBUGUI_##Name)

    DEBUGBeginVariableGroup(Context, "Group Chunks");
    DEBUG_VARIABLE_LISTING(GroundChunkOutlines);
    DEBUG_VARIABLE_LISTING(GroundChunkCheckerboards);
    DEBUG_VARIABLE_LISTING(RecomputeGroundChunksOnEXEChange);
    DEBUGEndVariableGroup(Context);

    DEBUGBeginVariableGroup(Context, "Particles");
    DEBUG_VARIABLE_LISTING(ParticleTest);
    DEBUG_VARIABLE_LISTING(ParticleGrid);
    DEBUGEndVariableGroup(Context);

    DEBUGBeginVariableGroup(Context, "Renderer");
    {
        DEBUG_VARIABLE_LISTING(TestWeirdDrawBufferSize);
        DEBUG_VARIABLE_LISTING(ShowLightingSamples);

        DEBUGBeginVariableGroup(Context, "Camera");
        {
            UseDebugCamRef = DEBUG_VARIABLE_LISTING(UseDebugCamera);
            DEBUG_VARIABLE_LISTING(DebugCameraDistance);
            DEBUG_VARIABLE_LISTING(UseRoomBasedCamera);
        }
        DEBUGEndVariableGroup(Context);

        DEBUGEndVariableGroup(Context);
    }

    DEBUG_VARIABLE_LISTING(FamiliarFollowsHero);
    DEBUG_VARIABLE_LISTING(UseSpaceOutlines);
    DEBUG_VARIABLE_LISTING(FauxV4);

#undef DEBUG_VARIABLE_LISTING
}


#define GAME_DEBUG_VARIABLES_H
#endif

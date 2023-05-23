#if !defined(GAME_DEBUG_VARIABLES_H)
#define DEBUG_VARIABLE_LISTING(Name) DebugVariableType_Boolean, #Name, Name

// TODO: Add _distance_ for the debug camera, so we have a float example
// TODO: Parameterise the fountain.
debug_variable DebugVariableList[] =
{
    DEBUG_VARIABLE_LISTING(DEBUGUI_UseDebugCamera),
    DEBUG_VARIABLE_LISTING(DEBUGUI_GroundChunkOutlines),
    DEBUG_VARIABLE_LISTING(DEBUGUI_ParticleTest),
    DEBUG_VARIABLE_LISTING(DEBUGUI_ParticleGrid),
    DEBUG_VARIABLE_LISTING(DEBUGUI_UseSpaceOutlines),
    DEBUG_VARIABLE_LISTING(DEBUGUI_GroundChunkCheckerboards),
    DEBUG_VARIABLE_LISTING(DEBUGUI_RecomputeGroundChunksOnEXEChange),
    DEBUG_VARIABLE_LISTING(DEBUGUI_TestWeirdDrawBufferSize),
    DEBUG_VARIABLE_LISTING(DEBUGUI_FamiliarFollowsHero),
    DEBUG_VARIABLE_LISTING(DEBUGUI_ShowLightingSamples),
    DEBUG_VARIABLE_LISTING(DEBUGUI_UseRoomBasedCamera),
};

#define GAME_DEBUG_VARIABLES_H
#endif

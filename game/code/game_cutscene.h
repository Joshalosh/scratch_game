
enum scene_layer_flags
{
    SceneLayerFlag_AtInfinity = 0x1,
    SceneLayerFlag_CounterCameraX = 0x2,
    SceneLayerFlag_CounterCameraY = 0x4,
    SceneLayerFlag_Transient = 0x8,
    SceneLayerFlag_Floaty = 0x10,
};

struct scene_layer
{
    v3 P;
    r32 Height;
    u32 Flags;
    v2 Param;
};

struct layered_scene
{
    asset_type_id AssetType;
    u32 ShotIndex;
    u32 LayerCount;
    scene_layer *Layers;

    r32 Duration;
    v3 CameraStart;
    v3 CameraEnd;

    r32 tFadeIn;
};

enum cutscene_id
{
    CutsceneID_Intro,
};
struct game_mode_cutscene
{
    cutscene_id ID;
    r32 t;
};

struct game_mode_title_screen
{
    r32 t;
};

struct game_state;
struct transient_state;
internal void PlayIntroCutscene(game_state *GameState, transient_state *TranState);
internal void PlayTitleScreen(game_state *GameState, transient_state *TranState);

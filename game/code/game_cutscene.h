#if !defined(GAME_CUTSCENE_H)

enum scene_layer_flags
{
    SceneLayerFlag_AtInfinity = 0x1,
    SceneLayerFlag_CounterCameraX = 0x2,
    SceneLayerFlag_CounterCameraY = 0x4,
    SceneLayerFlag_Transient = 0x8,
};

struct scene_layer 
{
    v3 P;
    r32 Height;
    u32 Flags;
    r32 MinTime;
    r32 MaxTime;
};

struct layered_scene
{
    asset_type_id AssetType;
    u32 ShotIndex;
    u32 LayerCount;
    scene_layer *Layers;

    v3 CameraStart;
    v3 CameraEnd;
};

#define GAME_CUTSCENE_H
#endif

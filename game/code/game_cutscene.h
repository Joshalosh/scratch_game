#if !defined(GAME_CUTSCENE_H)

enum scene_layer_flags
{
    SceneLayerFlag_AtInfinity = 0x1,
};

struct scene_layer 
{
    v3 P;
    r32 Height;
    u32 Flags;
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

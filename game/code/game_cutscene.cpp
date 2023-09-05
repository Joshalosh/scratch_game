
#include "game_cutscene.h"

internal void
RenderLayeredScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
                   layered_scene *Scene, r32 tNormal)
{
    // TODO: Unify this stuff?
    real32 WidthOfMonitor = 0.635f; // Horizontal measurement of monitor in metres
    real32 MetresToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;
    real32 FocalLength = 0.6f;

    v3 CameraStart = Scene->CameraStart;
    v3 CameraEnd = Scene->CameraEnd;
    v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetresToPixels, FocalLength, 0.0f);

    asset_vector MatchVector = {};  
    asset_vector WeightVector = {}; 
    WeightVector.E[Tag_ShotIndex] = 10.0f;
    WeightVector.E[Tag_LayerIndex] = 1.0f;

    MatchVector.E[Tag_ShotIndex] = (r32)Scene->ShotIndex;

    for(u32 LayerIndex = 1; LayerIndex <= Scene->LayerCount; ++LayerIndex)
    {
        scene_layer Layer = Scene->Layers[LayerIndex - 1];
        v3 P = Layer.P;
        if(Layer.Flags & SceneLayerFlag_AtInfinity)
        {
            P.z += CameraOffset.z;
        }
        RenderGroup->Transform.OffsetP = Layer.P - CameraOffset;
        MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;
        bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Asset_OpeningCutscene, &MatchVector, &WeightVector);
        PushBitmap(RenderGroup, LayerImage, Layer.Height, V3(0, 0, 0));
    }
}

internal void
RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer,
               r32 tCutScene)
{
    r32 tStart = 0.0f;
    r32 tEnd = 5.0f;

    r32 tNormal = Clamp01MapToRange(tStart, tCutScene, tEnd);

    layered_scene Scene;

    // Shot 1
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 1;
        int LayerCount = 8;
        scene_layer Layers[] =
        {
            {{0.0f, 0.0f, -200.0f}, 300.0f, SceneLayerFlag_AtInfinity}, // Sky Background
            {{0.0f, 0.0f, -170.0f}, 300.0f}, // Weird sky light
            {{0.0f, 0.0f, -100.0f}, 40.0f}, // Backmost row of trees
            {{0.0f, 10.0f, -70.0f}, 80.0f}, // Middle hills and trees
            {{0.0f, 0.0f, -50.0f}, 70.0f}, // Front hills and trees
            {{30.0f, 0.0f, -30.0f}, 50.0f}, // Right side tree and fence
            {{0.0f, -2.0f, -20.0f}, 40.0f}, // 7
            {{2.0f, -1.0f, -5.0f}, 25.0f}, // 8
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 10.0f};
        Scene.CameraEnd = {-4.0f, -2.0f, 5.0f};
    }
    
    // Shot 2
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 2;
        int LayerCount = 2;
        scene_layer Layers[] =
        {
            {{2.0f, -2.0f, -22.0f}, 30.0f}, // Hero and tree 
            {{0.0f, 0.0f, -18.0f}, 22.0f}, // Wall and window 
            {{0.0f, 2.0f, -8.0f}, 10.0f}, // Icicles 
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.5f, -0.5f, -1.0f};
    }
    
    // Shot 3
    {
        Scene.AssetType = Asset_OpeningCutscene;
        Scene.ShotIndex = 2;
        int LayerCount = 2;
        scene_layer Layers[] =
        {
            {{2.0f, -2.0f, -22.0f}, 30.0f}, // Hero and tree 
            {{0.0f, 0.0f, -18.0f}, 22.0f}, // Wall and window 
            {{0.0f, 2.0f, -8.0f}, 10.0f}, // Icicles 
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.5f, -0.5f, -1.0f};
    }
    
    RenderLayeredScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);

}

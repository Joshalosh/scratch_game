
internal void
RenderCutscene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
{
    // TODO: Unify this stuff?
    real32 WidthOfMonitor = 0.635f; // Horizontal measurement of monitor in metres
    real32 MetresToPixels = (real32)DrawBuffer->Width*WidthOfMonitor;

    real32 FocalLength = 0.6f;
    real32 DistanceAboveGround = 9.0f;
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, MetresToPixels, FocalLength, DistanceAboveGround);

    asset_vector MatchVector = {};  
    asset_vector WeightVector = {}; 
    WeightVector.E[Tag_ShotIndex] = 10.0f;
    WeightVector.E[Tag_LayerIndex] = 1.0f;

    int ShotIndex = 1;
    MatchVector.E[Tag_ShotIndex] = (r32)ShotIndex;
    for(u32 LayerIndex = 1; LayerIndex <= 1; ++LayerIndex)
    {
        MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;
        bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Asset_OpeningCutscene, &MatchVector, &WeightVector);
        PushBitmap(RenderGroup, LayerImage, 0.0f, V3(0, 0, 0));
    }
}

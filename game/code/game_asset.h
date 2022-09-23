#if !defined(GAME_ASSET_H)

struct hero_bitmaps
{
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetSTate_Locked,
};
struct asset_slot
{
    asset_state State;
    loaded_bitmap *Bitmap;
};

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,

    Tag_Count,
};

enum asset_type_id
{
    Asset_Backdrop,
    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
    Asset_Stairwell,
    Asset_Rock,

    Asset_Count,
};

struct asset_tag
{
    uint32_t ID; // Tag ID.
    real32 Value;
};
struct asset
{
    uint32_t FirstTagIndex;
    uint32_t OnePastLastTagIndex;
    uint32_t SlotID;
};

struct asset_type
{
    uint32_t Count;
    uint32_t FirstAssetIndex;
    uint32_t OnePastLastAssetIndex;
};
struct asset_bitmap_info
{
    v2 AlignPercentage;
    real32 WidthOverHeight;
    int32_t Width;
    int32_t Height;
};
struct asset_group
{
    uint32_t FirstTagIndex;
    uint32_t OnePastLastTagIndex;
};

struct game_assets
{
    // TODO: This back pointer kind of sucks.
    struct transient_state *TranState;
    memory_arena Arena;

    uint32_t BitmapCount;
    asset_slot *Bitmaps;

    uint32_t SoundCount;
    asset_slot *Sounds;

    asset_type AssetsTypes[Asset_Count];

    // Array'd assets.
    loaded_bitmap Grass[2];
    loaded_bitmap Stone[4];
    loaded_bitmap Tuft[3];

    // Structured assets.
    hero_bitmaps HeroBitmaps[4];
};

struct bitmap_id
{
    uint32_t Value;
};

struct audio_id
{
    uint32_t Value;
};

inline loaded_bitmap *GetBitmap(game_assets *Assets, bitmap_id ID)
{
    loaded_bitmap *Result = Assets->Bitmaps[ID.Value].Bitmap;

    return(Result);
}

internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
internal void LoadSound(game_assets *Assets, audio_id ID);

#define GAME_ASSET_H
#endif

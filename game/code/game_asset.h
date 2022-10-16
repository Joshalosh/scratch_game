#if !defined(GAME_ASSET_H)

struct loaded_sound
{
    uint32_t SampleCount;
    uint32_t ChannelCount;
    int16_t *Samples[2];
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
    union 
    {
        loaded_bitmap *Bitmap;
        loaded_sound *Sound;
    };
};

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    Tag_FacingDirection, // Angle in radians off of due right.

    Tag_Count,
};

enum asset_type_id
{
    Asset_None,

    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
//    Asset_Stairwell,
    Asset_Rock,

    Asset_Grass,
    Asset_Tuft,
    Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

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

struct asset_vector
{
    real32 E[Tag_Count];
};

struct asset_type
{
    uint32_t FirstAssetIndex;
    uint32_t OnePastLastAssetIndex;
};

struct asset_bitmap_info
{
    char *Filename;
    v2 AlignPercentage;
};

struct asset_sound_info
{
    char *Filename;
};

struct game_assets
{
    // TODO: This back pointer kind of sucks.
    struct transient_state *TranState;
    memory_arena Arena;

    real32 TagRange[Tag_Count];

    uint32_t BitmapCount;
    asset_bitmap_info *BitmapInfos;
    asset_slot *Bitmaps;

    uint32_t SoundCount;
    asset_sound_info *SoundInfos;
    asset_slot *Sounds;

    uint32_t TagCount;
    asset_tag *Tags;

    uint32_t AssetCount;
    asset *Assets;

    asset_type AssetTypes[Asset_Count];

    // Structured assets.
//    hero_bitmaps HeroBitmaps[4];

    // TODO: These should go away once I actually load an asset pack file.
    uint32_t DEBUGUsedBitmapCount;
    uint32_t DEBUGUsedAssetCount;
    uint32_t DEBUGUsedTagCount;
    asset_type *DEBUGAssetType;
    asset *DEBUGAsset;
};

struct bitmap_id
{
    uint32_t Value;
};

struct sound_id
{
    uint32_t Value;
};

inline loaded_bitmap *GetBitmap(game_assets *Assets, bitmap_id ID)
{
    loaded_bitmap *Result = Assets->Bitmaps[ID.Value].Bitmap;

    return(Result);
}

inline loaded_sound *GetSound(game_assets *Assets, sound_id ID)
{
    loaded_sound *Result = Assets->Sounds[ID.Value].Sound;

    return(Result);
}

internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
internal void LoadSound(game_assets *Assets, sound_id ID);

#define GAME_ASSET_H
#endif

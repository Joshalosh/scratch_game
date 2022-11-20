#if !defined(GAME_ASSET_H)

struct bitmap_id
{
    uint32_t Value;
};

struct sound_id
{
    uint32_t Value;
};

struct loaded_sound
{
    uint32_t SampleCount; // This is the sample count divided by 8.
    uint32_t ChannelCount;
    int16_t *Samples[2];
};

struct asset_tag
{
    uint32_t ID; // Tag ID.
    real32 Value;
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

struct asset
{
    ga_asset GA;
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

struct game_assets
{
    // TODO: This back pointer kind of sucks.
    struct transient_state *TranState;
    memory_arena Arena;

    real32 TagRange[Tag_Count];

    uint32_t TagCount;
    asset_tag *Tags;

    uint32_t AssetCount;
    asset *Assets;
    asset_slot *Slots;

    asset_type AssetTypes[Asset_Count];

#if 0
    // Structured assets.
//    hero_bitmaps HeroBitmaps[4];

    // TODO: These should go away once I actually load an asset pack file.
    uint32_t DEBUGUsedAssetCount;
    uint32_t DEBUGUsedTagCount;
    asset_type *DEBUGAssetType;
    asset *DEBUGAsset;
#endif
};

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    loaded_bitmap *Result = Assets->Slots[ID.Value].Bitmap;

    return(Result);
}

inline loaded_sound *
GetSound(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    loaded_sound *Result = Assets->Slots[ID.Value].Sound;

    return(Result);
}

inline asset_sound_info *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    asset_sound_info *Result = &Assets->Assets[ID.Value].Sound;

    return(Result);
}

inline bool32
IsValid(bitmap_id ID)
{
    bool32 Result = (ID.Value != 0);

    return(Result);
}

inline bool32
IsValid(sound_id ID)
{
    bool32 Result = (ID.Value != 0);

    return(Result);
}

internal void LoadBitmap(game_assets *Assets, bitmap_id ID);
inline void PrefetchBitmap(game_assets *Assets, bitmap_id ID) {LoadBitmap(Assets, ID);}
internal void LoadSound(game_assets *Assets, sound_id ID);
inline void PrefetchSound(game_assets *Assets, sound_id ID) {LoadSound(Assets, ID);}

#define GAME_ASSET_H
#endif

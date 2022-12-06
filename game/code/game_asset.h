#if !defined(GAME_ASSET_H)

struct loaded_sound
{
    // TODO: This could be shrunk to 12 bytes if the loaded_bitmap
    // ever got down that small.
    int16_t *Samples[2];
    uint32_t SampleCount; // This is the sample count divided by 8.
    uint32_t ChannelCount;
};

enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
};
struct asset_slot
{
    u32 State;
    union
    {
        loaded_bitmap Bitmap;
        loaded_sound Sound;
    };
};

struct asset
{
    ga_asset GA;
    u32 FileIndex;
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

struct asset_file
{
    platform_file_handle *Handle;

    // TODO: If we ever do thread stack, AssetTypeArray
    // doesn't actually need to be kept here probably.
    ga_header Header;
    ga_asset_type *AssetTypeArray;

    u32 TagBase;
};

struct game_assets
{
    // TODO: This back pointer kind of sucks.
    struct transient_state *TranState;
    memory_arena Arena;

    real32 TagRange[Tag_Count];

    u32 FileCount;
    asset_file *Files;

    uint32_t TagCount;
    ga_tag *Tags;

    uint32_t AssetCount;
    asset *Assets;
    asset_slot *Slots;

    asset_type AssetTypes[Asset_Count];

#if 0
    u8 *GAContents;

    // Structured assets.
//    hero_bitmaps HeroBitmaps[4];

    // TODO: These should go away once I actually load an asset pack file.
    uint32_t DEBUGUsedAssetCount;
    uint32_t DEBUGUsedTagCount;
    asset_type *DEBUGAssetType;
    asset *DEBUGAsset;
#endif
};

inline loaded_bitmap *GetBitmap(game_assets *Assets, bitmap_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    asset_slot *Slot = Assets->Slots + ID.Value;

    loaded_bitmap *Result = 0;
    if(Slot->State >= AssetState_Loaded)
    {
        CompletePreviousReadsBeforeFutureReads;
        Result = &Slot->Bitmap;
    }

    return(Result);
}

inline loaded_sound *GetSound(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    asset_slot *Slot = Assets->Slots + ID.Value;

    loaded_sound *Result = 0;
    if(Slot->State >= AssetState_Loaded)
    {
        CompletePreviousReadsBeforeFutureReads;
        Result = &Slot->Sound;
    }

    return(Result);
}

inline ga_sound *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    ga_sound *Result = &Assets->Assets[ID.Value].GA.Sound;

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

inline sound_id GetNextSoundInChain(game_assets *Assets, sound_id ID)
{
    sound_id Result = {};

    ga_sound *Info = GetSoundInfo(Assets, ID);
    switch(Info->Chain)
    {
        case GASoundChain_None:
        {
            // Nothing to do.
        } break;

        case GASoundChain_Loop:
        {
            Result = ID;
        } break;

        case GASoundChain_Advance:
        {
            Result.Value = ID.Value + 1;
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }

    return(Result);
}

#define GAME_ASSET_H
#endif

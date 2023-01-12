#if !defined(GAME_ASSET_H)

struct loaded_sound
{
    int16_t *Samples[2];
    uint32_t SampleCount; // This is the sample count divided by 8.
    uint32_t ChannelCount;
};

struct loaded_font
{
    ga_font_glyph *Glyphs;
    r32 *HorizontalAdvance;
    u32 BitmapIDOffset;
    u16 *UnicodeMap;
};

// TODO: I can streamline this by using header pointer as an indicator of unloaded status.
enum asset_state
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
};

struct asset_memory_header
{
    asset_memory_header *Next;
    asset_memory_header *Prev;

    u32 AssetIndex;
    u32 TotalSize;
    u32 GenerationID;
    union
    {
        loaded_bitmap Bitmap;
        loaded_sound Sound;
        loaded_font Font;
    };
};

struct asset
{
    u32 State;
    asset_memory_header *Header;

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
    platform_file_handle Handle;

    // TODO: If we ever do thread stack, AssetTypeArray
    // doesn't actually need to be kept here probably.
    ga_header Header;
    ga_asset_type *AssetTypeArray;

    u32 TagBase;
    s32 FontBitmapIDOffset;
};

enum asset_memory_block_flags
{
    AssetMemory_Used = 0x1,
};
struct asset_memory_block
{
    asset_memory_block *Prev;
    asset_memory_block *Next;
    u64 Flags;
    memory_index Size;
};

struct game_assets
{
    u32 NextGenerationID;

    // TODO: This back pointer kind of sucks.
    struct transient_state *TranState;

    asset_memory_block MemorySentinel;

    asset_memory_header LoadedAssetSentinel;

    real32 TagRange[Tag_Count];

    u32 FileCount;
    asset_file *Files;

    uint32_t TagCount;
    ga_tag *Tags;

    uint32_t AssetCount;
    asset *Assets;

    asset_type AssetTypes[Asset_Count];

    u32 OperationLock;

    u32 InFlightGenerationCount;
    u32 InFlightGenerations[16];
};

inline void
BeginAssetLock(game_assets *Assets)
{
    for(;;)
    {
        if(AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0)
        {
            break;
        }
    }
}

inline void
EndAssetLock(game_assets *Assets)
{
    CompletePreviousWritesBeforeFutureWrites;
    Assets->OperationLock = 0;
}

inline void
InsertAssetHeaderAtFront(game_assets *Assets, asset_memory_header *Header)
{
    asset_memory_header *Sentinel = &Assets->LoadedAssetSentinel;

    Header->Prev = Sentinel;
    Header->Next = Sentinel->Next;

    Header->Next->Prev = Header;
    Header->Prev->Next = Header;
}

internal void
RemoveAssetHeaderFromList(asset_memory_header *Header)
{
    Header->Prev->Next = Header->Next;
    Header->Next->Prev = Header->Prev;

    Header->Next = Header->Prev = 0;
}

inline asset_memory_header *GetAsset(game_assets *Assets, u32 ID, u32 GenerationID)
{
    Assert(ID <= Assets->AssetCount);
    asset *Asset = Assets->Assets + ID;

    asset_memory_header *Result = 0;

    BeginAssetLock(Assets);

    if(Asset->State == AssetState_Loaded)
    {
        Result = Asset->Header;
        RemoveAssetHeaderFromList(Result);
        InsertAssetHeaderAtFront(Assets, Result);

        if(Asset->Header->GenerationID < GenerationID)
        {
            Asset->Header->GenerationID = GenerationID;
        }

        CompletePreviousWritesBeforeFutureWrites;
    }

    EndAssetLock(Assets);

    return(Result);
}

inline loaded_bitmap *
GetBitmap(game_assets *Assets, bitmap_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_bitmap *Result = Header ? &Header->Bitmap : 0;

    return(Result);
}

inline ga_bitmap *
GetBitmapInfo(game_assets *Assets, bitmap_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    ga_bitmap *Result = &Assets->Assets[ID.Value].GA.Bitmap;

    return(Result);
}

inline loaded_sound *
GetSound(game_assets *Assets, sound_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_sound *Result = Header ? &Header->Sound : 0;

    return(Result);
}

inline ga_sound *
GetSoundInfo(game_assets *Assets, sound_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    ga_sound *Result = &Assets->Assets[ID.Value].GA.Sound;

    return(Result);
}

inline loaded_font *
GetFont(game_assets *Assets, font_id ID, u32 GenerationID)
{
    asset_memory_header *Header = GetAsset(Assets, ID.Value, GenerationID);

    loaded_font *Result = Header ? &Header->Font : 0;

    return(Result);
}

inline ga_font *
GetFontInfo(game_assets *Assets, font_id ID)
{
    Assert(ID.Value <= Assets->AssetCount);
    ga_font *Result = &Assets->Assets[ID.Value].GA.Font;

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

internal void LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate);
inline void PrefetchBitmap(game_assets *Assets, bitmap_id ID) {LoadBitmap(Assets, ID, false);}
internal void LoadSound(game_assets *Assets, sound_id ID);
inline void PrefetchSound(game_assets *Assets, sound_id ID) {LoadSound(Assets, ID);}
internal void LoadFont(game_assets *Assets, font_id ID, b32 Immediate);
inline void PrefetchFont(game_assets *Assets, font_id ID) {LoadFont(Assets, ID, false);}

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

inline u32
BeginGeneration(game_assets *Assets)
{
    BeginAssetLock(Assets);
    
    Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
    u32 Result = Assets->NextGenerationID++;
    Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;

    EndAssetLock(Assets);

    return(Result);
}

inline void
EndGeneration(game_assets *Assets, u32 GenerationID)
{
    BeginAssetLock(Assets);

    for(u32 Index = 0; Index < Assets->InFlightGenerationCount; ++Index)
    {
        if(Assets->InFlightGenerations[Index] == GenerationID)
        {
            Assets->InFlightGenerations[Index] = Assets->InFlightGenerations[--Assets->InFlightGenerationCount];
            break;
        }
    }

    EndAssetLock(Assets);
}

#define GAME_ASSET_H
#endif

#if !defined(GAME_FILE_FORMATS_H)

#define GA_CODE(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#pragma pack(push, 1)
struct bitmap_id
{
    u32 Value;
};

struct sound_id
{
    u32 Value;
};

struct ga_header
{
#define GA_MAGIC_VALUE GA_CODE('g','a','f',' ')
    u32 MagicValue;

#define GA_VERSION 0
    u32 Version;

    u32 TagCount;
    u32 AssetTypeCount;
    u32 AssetCount;

    u64 Tags; // ga_tag[TagCount]
    u64 AssetTypes; // ga_asset_type[AssetTypeCount]
    u64 Assets; // ga_asset[AssetCount]

    // TODO: Primacy numbers for asset files.
    
    /* TODO:
       u32 FileGUID[8];
       u32 RemovalCount;
      
       struct ga_asset_removal
       {
           u32 FileGUID[8];
           u32 AssetIndex;
       };

     */
};

struct ga_tag
{
    u32 ID;
    r32 Value;
};

struct ga_asset_type
{
    u32 TypeID;
    u32 FirstAssetIndex;
    u32 OnePastLastAssetIndex;
};

struct ga_bitmap
{
    u32 Dim[2];
    r32 AlignPercentage[2];
};
struct ga_sound
{
    u32 SampleCount;
    u32 ChannelCount;
    sound_id NextIDToPlay;
};
struct ga_asset
{
    u64 DataOffset;
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
    union
    {
        ga_bitmap Bitmap;
        ga_sound Sound;
    };
};

#pragma pack(pop)

#define GAME_FILE_FORMATS_H
#endif

#if !defined(TEST_ASSET_BUILDER_H)

#include <stdio.h>
#include <stdlib.h>
#include "game_platform.h"
#include "game_asset_type_id.h"
#include "game_file_formats.h"

struct bitmap_id
{
    u32 Value;
};

struct sound_id
{
    u32 Value;
};

struct asset_bitmap_info
{
    char *Filename;
    r32 AlignPercentage[2];
};

struct asset_sound_info
{
    char *Filename;
    u32 FirstSampleIndex;
    u32 SampleCount;
    sound_id NextIDToPlay;
};

struct asset
{
    u64 DataOffset;
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
    union
    {
        asset_bitmap_info Bitmap;
        asset_sound_info Sound;
    };
};
struct asset_type
{
    u32 FirstAssetIndex;
    u32 OnePastLastAssetIndex;
};

struct bitmap_asset
{
    char *Filename;
    r32 Alignment[2];
};

#define VERY_LARGE_NUMBER 4096

struct game_assets
{
    u32 TagCount;
    ga_tag Tags[VERY_LARGE_NUMBER];

    u32 AssetTypeCount;
    ga_asset_type AssetTypes[Asset_Count];

    u32 AssetCount;
    asset Assets[VERY_LARGE_NUMBER];

    ga_asset_type *DEBUGAssetType;
    asset *DEBUGAsset;
};

#define TEST_ASSET_BUILDER_H
#endif

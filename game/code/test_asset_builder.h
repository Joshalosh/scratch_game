#if !defined(TEST_ASSET_BUILDER_H)

#include <stdio.h>
#include <stdlib.h>
#include "game_platform.h"
#include "game_asset_type_id.h"
#include "game_file_formats.h"
#include "game_intrinsics.h"
#include "game_math.h"

struct bitmap_id
{
    u32 Value;
};

struct sound_id
{
    u32 Value;
};

enum asset_type
{
    AssetType_Sound,
    AssetType_Bitmap,
};

struct asset_source
{
    asset_type Type;
    char *Filename;
    u32 FirstSampleIndex;
};

#define VERY_LARGE_NUMBER 4096

struct game_assets
{
    u32 TagCount;
    ga_tag Tags[VERY_LARGE_NUMBER];

    u32 AssetTypeCount;
    ga_asset_type AssetTypes[Asset_Count];

    u32 AssetCount;
    asset_source AssetSources[VERY_LARGE_NUMBER];
    ga_asset Assets[VERY_LARGE_NUMBER];

    ga_asset_type *DEBUGAssetType;
    u32 AssetIndex;
};

#define TEST_ASSET_BUILDER_H
#endif
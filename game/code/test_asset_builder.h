#if !defined(TEST_ASSET_BUILDER_H)

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "game_platform.h"
#include "game_file_formats.h"
#include "game_intrinsics.h"
#include "game_math.h"

enum asset_type
{
    AssetType_Sound,
    AssetType_Bitmap,
    AssetType_Font,
    AssetType_FontGlyph,
};

struct loaded_font;
struct asset_source_font
{
    loaded_font *Font;
};

struct asset_source_font_glyph
{
    loaded_font *Font;
    u32 Codepoint;
};

struct asset_source_bitmap
{
    char *Filename;
};

struct asset_source_sound
{
    char *Filename;
    u32 FirstSampleIndex;
};

struct asset_source
{
    asset_type Type;
    union
    {
        asset_source_bitmap Bitmap;
        asset_source_sound Sound;
        asset_source_font Font;
        asset_source_font_glyph Glyph;
    };
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

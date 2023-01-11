#if !defined(TEST_ASSET_BUILDER_H)

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "game_platform.h"
#include "game_file_formats.h"
#include "game_intrinsics.h"
#include "game_math.h"

#define USE_FONTS_FROM_WINDOWS 1

#if USE_FONTS_FROM_WINDOWS
#include <windows.h>

#define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024

global_variable VOID *GlobalFontBits;
global_variable HDC GlobalFontDeviceContext;

#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

struct loaded_bitmap
{
    int32_t Width;
    int32_t Height;
    int32_t Pitch;
    void *Memory;

    void *Free;
};

struct loaded_font
{
    HFONT Win32Handle;
    TEXTMETRIC TextMetric;
    r32 LineAdvance;

    ga_font_glyph *Glyphs;
    r32 *HorizontalAdvance;

    u32 MinCodepoint;
    u32 MaxCodepoint;

    u32 MaxGlyphCount;
    u32 GlyphCount;

    u32 *GlyphIndexFromCodepoint;
};

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

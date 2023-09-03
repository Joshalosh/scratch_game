#if !defined(GAME_FILE_FORMATS_H)

enum asset_font_type
{
    FontType_Default = 0,
    FontType_Debug = 10,
};

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    Tag_FacingDirection, // Angle in radians off of due right.
    Tag_UnicodeCodepoint,
    Tag_FontType, // 0 - Default Game Font, 10 - Debug font.

    Tag_ShotIndex,
    Tag_LayerIndex,

    Tag_Count,
};

enum asset_type_id
{
    Asset_None,

    //
    // Bitmaps
    //

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

    Asset_Font,
    Asset_FontGlyph,

    //
    // Sounds
    //

    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,
    
    //
    //
    //

    Asset_OpeningCutscene,

    Asset_Count,
};

#define GA_CODE(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#pragma pack(push, 1)

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

enum ga_sound_chain
{
    GASoundChain_None,
    GASoundChain_Loop,
    GASoundChain_Advance,
};
struct ga_bitmap
{
    u32 Dim[2];
    r32 AlignPercentage[2];
    /* Data is:
     
       u32 Pixels[Dim[1]][Dim[0]];
    */
};
struct ga_sound
{
    u32 SampleCount;
    u32 ChannelCount;
    u32 Chain; // ga_sound_chain
    /* Data is:

       s16 Channels[ChannelCount][SampleCount];
    */
};
struct ga_font_glyph
{
    u32 UnicodeCodepoint;
    bitmap_id BitmapID;
};
struct ga_font
{
    u32 OnePastHighestCodepoint;
    u32 GlyphCount;
    r32 AscenderHeight;
    r32 DescenderHeight;
    r32 ExternalLeading;
    /* Data is:
    
       ga_font_glyph Codepoints[GlyphCount];
       r32 HorizontalAdvance[GlyphCount][GlyphCount];
    */
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
        ga_font Font;
    };
};

#pragma pack(pop)

#define GAME_FILE_FORMATS_H
#endif

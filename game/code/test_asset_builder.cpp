
#include "test_asset_builder.h"

#define USE_FONTS_FROM_WINDOWS 1

#if USE_FONTS_FROM_WINDOWS
#include <windows.h>

#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024

global_variable VOID *GlobalFontBits;
global_variable HDC GlobalFontDeviceContext;

#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#pragma pack(push, 1)
struct bitmap_header
{
    uint16_t FileType;
    uint32_t FileSize;
    uint16_t Reserved1;
    uint16_t Reserved2;
    uint32_t BitmapOffset;
    uint32_t Size;
    int32_t Width;
    int32_t Height;
    uint16_t Planes;
    uint16_t BitsPerPixel;
    uint32_t Compression;
    uint32_t SizeOfBitmap;
    int32_t HorzResolution;
    int32_t VertResolution;
    uint32_t ColorsUsed;
    uint32_t ColorsImportant;

    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
};

struct WAVE_header
{
    uint32_t RIFFID;
    uint32_t Size;
    uint32_t WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
enum
{
    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};
struct WAVE_chunk
{
    uint32_t ID;
    uint32_t Size;
};

struct WAVE_fmt
{
    uint16_t wFormatTag;
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec;
    uint16_t nBlockAlign;
    uint16_t wBitsPerSample;
    uint16_t cbSize;
    uint16_t wValidBitsPerSample;
    uint32_t dwChannelMask;
    uint8_t SubFormat[16];
};

#pragma pack(pop)

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
    u32 CodepointCount;
    r32 LineAdvance;

    bitmap_id *BitmapIDs;
    r32 *HorizontalAdvance;
};

struct entire_file
{
    u32 ContentsSize;
    void *Contents;
};
entire_file
ReadEntireFile(char *Filename)
{
    entire_file Result = {};

    FILE *In = fopen(Filename, "rb");
    if(In)
    {
        fseek(In, 0, SEEK_END);
        Result.ContentsSize = ftell(In);
        fseek(In, 0, SEEK_SET);

        Result.Contents = malloc(Result.ContentsSize);
        fread(Result.Contents, Result.ContentsSize, 1, In);
        fclose(In);
    }
    else
    {
        printf("ERROR: Cannot open file %s.\n", Filename);
    }

    return(Result);
}

internal loaded_bitmap
LoadBMP(char *Filename)
{
    loaded_bitmap Result = {};
    
    entire_file ReadResult = ReadEntireFile(Filename);
    if(ReadResult.ContentsSize != 0)
    {
        Result.Free = ReadResult.Contents;

        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        Assert(Result.Height >= 0);
        Assert(Header->Compression == 3);

        uint32_t RedMask = Header->RedMask;
        uint32_t GreenMask = Header->GreenMask;
        uint32_t BlueMask = Header->BlueMask;
        uint32_t AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

        int32_t RedShiftDown = (int32_t)RedScan.Index;
        int32_t GreenShiftDown = (int32_t)GreenScan.Index;
        int32_t BlueShiftDown = (int32_t)BlueScan.Index;
        int32_t AlphaShiftDown = (int32_t)AlphaScan.Index;

        uint32_t *SourceDest = Pixels;
        for(int32_t Y = 0; Y < Header->Height; ++Y)
        {
            for(int32_t X = 0; X < Header->Width; ++X)
            {
                uint32_t C = *SourceDest;

                v4 Texel = {(real32)((C & RedMask) >> RedShiftDown),
                            (real32)((C & GreenMask) >> GreenShiftDown),
                            (real32)((C & BlueMask) >> BlueShiftDown),
                            (real32)((C & AlphaMask) >> AlphaShiftDown)};

                Texel = SRGB255ToLinear1(Texel);
#if 1
                Texel.rgb *= Texel.a;
#endif
                Texel = Linear1ToSRGB255(Texel);

                *SourceDest++ = (((uint32_t)(Texel.a + 0.5f) << 24) |
                                 ((uint32_t)(Texel.r + 0.5f) << 16) |
                                 ((uint32_t)(Texel.g + 0.5f) << 8) |
                                 ((uint32_t)(Texel.b + 0.5f) << 0));
            }
        }
    }

    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;

#if 0
    Result.Memory = (uint8_t *)Result.Memory + Result.Pitch*(Result.Height - 1);
    Result.Pitch = -Result.Pitch;
#endif

    return(Result);
}

internal loaded_font *
LoadFont(char *Filename, char *FontName, u32 CodepointCount)
{
    loaded_font *Font = (loaded_font *)malloc(sizeof(loaded_font));

    AddFontResourceExA(Filename, FR_PRIVATE, 0);
    int Height = 128; // I need to figure out how to specify pixels properly here.
    Font->Win32Handle = CreateFontA(Height, 0, 0, 0,
                                    FW_NORMAL, // Weight.
                                    FALSE, // Italic.
                                    FALSE, // Underline.
                                    FALSE, // Strikeout.
                                    DEFAULT_CHARSET, 
                                    OUT_DEFAULT_PRECIS,
                                    CLIP_DEFAULT_PRECIS, 
                                    ANTIALIASED_QUALITY,
                                    DEFAULT_PITCH|FF_DONTCARE,
                                    FontName);

    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);
    GetTextMetrics(GlobalFontDeviceContext, &Font->TextMetric);

    Font->CodepointCount = CodepointCount;
    Font->BitmapIDs = (bitmap_id *)malloc(sizeof(bitmap_id)*CodepointCount);
    size_t HorizontalAdvanceSize = sizeof(r32)*CodepointCount*CodepointCount;
    Font->HorizontalAdvance = (r32 *)malloc(HorizontalAdvanceSize);
    memset(Font->HorizontalAdvance, 0, HorizontalAdvanceSize);
            
    DWORD KerningPairCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
    KERNINGPAIR *KerningPairs = (KERNINGPAIR *)malloc(KerningPairCount*sizeof(KERNINGPAIR));
    GetKerningPairsW(GlobalFontDeviceContext, KerningPairCount, KerningPairs);
    for(DWORD KerningPairIndex = 0; KerningPairIndex < KerningPairCount; ++KerningPairIndex)
    {
        KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
        if((Pair->wFirst < Font->CodepointCount) &&
           (Pair->wSecond < Font->CodepointCount))
        {
            Font->HorizontalAdvance[Pair->wFirst*Font->CodepointCount + Pair->wSecond] += (r32)Pair->iKernAmount;
        }
    }

    free(KerningPairs);

    return(Font);
}

internal void
FreeFont(loaded_font *Font)
{
    DeleteObject(Font->Win32Handle);
    free(Font);
}

internal void
InitialiseFontDC(void)
{
    GlobalFontDeviceContext = CreateCompatibleDC(GetDC(0));

    BITMAPINFO Info = {};
    Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
    Info.bmiHeader.biWidth = MAX_FONT_WIDTH;
    Info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;
    Info.bmiHeader.biSizeImage = 0;
    Info.bmiHeader.biXPelsPerMeter = 0;
    Info.bmiHeader.biYPelsPerMeter = 0;
    Info.bmiHeader.biClrUsed = 0;
    Info.bmiHeader.biClrImportant = 0;
    HBITMAP Bitmap = CreateDIBSection(GlobalFontDeviceContext, &Info, DIB_RGB_COLORS, &GlobalFontBits, 0, 0);
    SelectObject(GlobalFontDeviceContext, Bitmap);
    SetBkColor(GlobalFontDeviceContext, RGB(0, 0, 0));
}

internal loaded_bitmap
LoadGlyphBitmap(loaded_font *Font, u32 Codepoint, ga_asset *Asset)
{
    loaded_bitmap Result = {};

#if USE_FONTS_FROM_WINDOWS

    SelectObject(GlobalFontDeviceContext, Font->Win32Handle);

    memset(GlobalFontBits, 0x00, MAX_FONT_WIDTH*MAX_FONT_HEIGHT*sizeof(u32));

    wchar_t CheesePoint = (wchar_t)Codepoint;

    SIZE Size;
    GetTextExtentPoint32W(GlobalFontDeviceContext, &CheesePoint, 1, &Size);

    int PreStepX = 128;

    int BoundWidth = Size.cx + 2*PreStepX;
    if(BoundWidth > MAX_FONT_WIDTH)
    {
        BoundWidth = MAX_FONT_WIDTH;
    }
    int BoundHeight = Size.cy;
    if(BoundHeight > MAX_FONT_HEIGHT)
    {
        BoundHeight = MAX_FONT_HEIGHT;
    }

//    PatBlt(DeviceContext, 0, 0, Width, Height, BLACKNESS);
//    SetBkMode(DeviceContext, TRANSPARENT);
    SetTextColor(GlobalFontDeviceContext, RGB(255, 255, 255));
    TextOutW(GlobalFontDeviceContext, PreStepX, 0, &CheesePoint, 1);

    s32 MinX = 10000;
    s32 MinY = 10000;
    s32 MaxX = -10000;
    s32 MaxY = -10000;

    u32 *Row = (u32 *)GlobalFontBits + (MAX_FONT_HEIGHT - 1)*MAX_FONT_WIDTH;
    for(s32 Y = 0; Y < BoundHeight; ++Y)
    {
        u32 *Pixel = Row;
        for(s32 X = 0; X < BoundWidth; ++X)
        {
#if 0
            COLORREF RefPixel = GetPixel(GlobalFontDeviceContext, X, Y);
            Assert(RefPixel == *Pixel);
#endif
            if(*Pixel != 0)
            {
                if(MinX > X)
                {
                    MinX = X;
                }

                if(MinY > Y)
                {
                    MinY = Y;
                }

                if(MaxX < X)
                {
                    MaxX = X;
                }

                if(MaxY < Y)
                {
                    MaxY = Y;
                }
            }

            ++Pixel;
        }
        Row -= MAX_FONT_WIDTH;
    }

    if(MinX <= MaxX)
    {
        int Width = (MaxX - MinX) + 1;
        int Height = (MaxY - MinY) + 1;

        Result.Width = Width + 2;
        Result.Height = Height + 2;
        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Memory = malloc(Result.Height*Result.Pitch);
        Result.Free = Result.Memory;

        memset(Result.Memory, 0, Result.Height*Result.Pitch);

        u8 *DestRow = (u8 *)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;
        u32 *SourceRow = (u32 *)GlobalFontBits + (MAX_FONT_HEIGHT - 1 - MinY)*MAX_FONT_WIDTH;
        for(s32 Y = MinY; Y <= MaxY; ++Y)
        {
            u32 *Source = (u32 *)SourceRow + MinX;
            u32 *Dest = (u32 *)DestRow + 1;
            for(s32 X = MinX; X <= MaxX; ++X)
            {
#if 0
                COLORREF Pixel = GetPixel(GlobalFontDeviceContext, X, Y);
                Assert(Pixel == *Source);
#else
                u32 Pixel = *Source;
#endif
                r32 Gray = (r32)(Pixel & 0xFF);
                v4 Texel = {255.0f, 255.0f, 255.0f, Gray};
                Texel = SRGB255ToLinear1(Texel);
                Texel.rgb *= Texel.a;
                Texel = Linear1ToSRGB255(Texel);

                *Dest++ = (((u32)(Texel.a + 0.5f) << 24) |
                           ((u32)(Texel.r + 0.5f) << 16) |
                           ((u32)(Texel.g + 0.5f) << 8) |
                           ((u32)(Texel.b + 0.5f) << 0));

                ++Source;
            }

            DestRow -= Result.Pitch;
            SourceRow -= MAX_FONT_WIDTH;
        }

#if 0
        ABC ThisABC;
        GetCharABCWidthsW(GlobalFontDeviceContext, Codepoint, Codepoint, &ThisABC);
        r32 CharAdvance = (r32)(ThisABC.abcA + ThisABC.abcB + ThisABC.abcC);
#else
        INT ThisWidth;
        GetCharWidth32W(GlobalFontDeviceContext, Codepoint, Codepoint, &ThisWidth);
        r32 CharAdvance = (r32)ThisWidth;
#endif

        r32 KerningChange = (r32)(MinX - PreStepX);
        for(u32 OtherCodepointIndex = 0; 
            OtherCodepointIndex < Font->CodepointCount;
            ++OtherCodepointIndex)
        {
            Font->HorizontalAdvance[Codepoint*Font->CodepointCount + OtherCodepointIndex] += CharAdvance - KerningChange;
            if(OtherCodepointIndex != 0)
            {
                Font->HorizontalAdvance[OtherCodepointIndex*Font->CodepointCount + Codepoint] += KerningChange;
            }
        }

        Asset->Bitmap.AlignPercentage[0] = (1.0f) / (r32)Result.Width;
        Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->TextMetric.tmDescent))) / (r32)Result.Height;
    }

#else

    entire_file TTFFile = ReadEntireFile(Filename);
    if(TTFFile.ContentsSize != 0)
    {
        stbtt_fontinfo Font;
        stbtt_InitFont(&Font, (u8 *)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8 *)TTFFile.Contents, 0));

        int Width, Height, XOffset, YOffset;
        u8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 128.0f),
                                                  Codepoint, &Width, &Height, &XOffset, &YOffset);

        Result.Width = Width;
        Result.Height = Height;
        Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
        Result.Memory = malloc(Height*Result.Pitch);
        Result.Free = Result.Memory;

        u8 *Source = MonoBitmap;
        u8 *DestRow = (u8 *)Result.Memory + (Height - 1)*Result.Pitch;
        for(s32 Y = 0; Y < Height; ++Y)
        {
            u32 *Dest = (u32 *)DestRow;
            for(s32 X = 0; X < Width; ++X)
            {
                u8 Gray = *Source++;
                u8 Alpha = 0xFF;
                *Dest++ = ((Alpha << 24) |
                           (Gray << 16) |
                           (Gray <<  8) |
                           (Gray <<  0));
            }

            DestRow -= Result.Pitch;
        }

        stbtt_FreeBitmap(MonoBitmap, 0);
        free(TTFFile.Contents);
    }
#endif
    
    return(Result);
}

struct riff_iterator
{
    u8 *At;
    u8 *Stop;
};

inline riff_iterator
ParseChunkAt(void *At, void *Stop)
{
    riff_iterator Iter;

    Iter.At = (u8 *)At;
    Iter.Stop = (u8 *)Stop;

    return(Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    u32 Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(WAVE_chunk) + Size;

    return(Iter);
}
             
inline bool32
IsValid(riff_iterator Iter)
{    
    b32 Result = (Iter.At < Iter.Stop);
    
    return(Result);
}

inline void *
GetChunkData(riff_iterator Iter)
{
    void *Result = (Iter.At + sizeof(WAVE_chunk));

    return(Result);
}

inline u32
GetType(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    u32 Result = Chunk->ID;

    return(Result);
}

inline u32
GetChunkDataSize(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    u32 Result = Chunk->Size;

    return(Result);
}

struct loaded_sound
{
    u32 SampleCount; // NOTE(casey): This is the sample count divided by 8
    u32 ChannelCount;
    s16 *Samples[2];

    void *Free;
};

internal loaded_sound
LoadWAV(char *FileName, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
{
    loaded_sound Result = {};
    
    entire_file ReadResult = ReadEntireFile(FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        Result.Free = ReadResult.Contents;
        
        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

        u32 ChannelCount = 0;
        u32 SampleDataSize = 0;
        s16 *SampleData = 0;
        for(riff_iterator Iter = ParseChunkAt(Header + 1, (u8 *)(Header + 1) + Header->Size - 4);
            IsValid(Iter);
            Iter = NextChunk(Iter))
        {
            switch(GetType(Iter))
            {
                case WAVE_ChunkID_fmt:
                {
                    WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iter);
                    Assert(fmt->wFormatTag == 1); // NOTE(casey): Only support PCM
                    Assert(fmt->nSamplesPerSec == 48000);
                    Assert(fmt->wBitsPerSample == 16);
                    Assert(fmt->nBlockAlign == (sizeof(s16)*fmt->nChannels));
                    ChannelCount = fmt->nChannels;
                } break;

                case WAVE_ChunkID_data:
                {
                    SampleData = (s16 *)GetChunkData(Iter);
                    SampleDataSize = GetChunkDataSize(Iter);
                } break;
            }
        }

        Assert(ChannelCount && SampleData);

        Result.ChannelCount = ChannelCount;
        u32 SampleCount = SampleDataSize / (ChannelCount*sizeof(s16));
        if(ChannelCount == 1)
        {
            Result.Samples[0] = SampleData;
            Result.Samples[1] = 0;
        }
        else if(ChannelCount == 2)
        {
            Result.Samples[0] = SampleData;
            Result.Samples[1] = SampleData + SampleCount;

#if 0
            for(u32 SampleIndex = 0;
                SampleIndex < SampleCount;
                ++SampleIndex)
            {
                SampleData[2*SampleIndex + 0] = (s16)SampleIndex;
                SampleData[2*SampleIndex + 1] = (s16)SampleIndex;
            }
#endif
            
            for(u32 SampleIndex = 0;
                SampleIndex < SampleCount;
                ++SampleIndex)
            {
                s16 Source = SampleData[2*SampleIndex];
                SampleData[2*SampleIndex] = SampleData[SampleIndex];
                SampleData[SampleIndex] = Source;
            }
        }
        else
        {
            Assert(!"Invalid channel count in WAV file");
        }

        // TODO(casey): Load right channels!
        b32 AtEnd = true;
        Result.ChannelCount = 1;
        if(SectionSampleCount)
        {
            Assert((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
            AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
            SampleCount = SectionSampleCount;
            for(u32 ChannelIndex = 0;
                ChannelIndex < Result.ChannelCount;
                ++ChannelIndex)
            {
                Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
            }
        }

        if(AtEnd)
        {
            for(u32 ChannelIndex = 0;
                ChannelIndex < Result.ChannelCount;
                ++ChannelIndex)
            {
                for(u32 SampleIndex = SampleCount;
                    SampleIndex < (SampleCount + 8);
                    ++SampleIndex)
                {
                    Result.Samples[ChannelIndex][SampleIndex] = 0;
                }
            }
        }

        Result.SampleCount = SampleCount;
    }

    return(Result);
}

internal void
BeginAssetType(game_assets *Assets, asset_type_id TypeID)
{
    Assert(Assets->DEBUGAssetType == 0);

    Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
    Assets->DEBUGAssetType->TypeID = TypeID;
    Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
    Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

struct added_asset
{
    u32 ID;
    ga_asset *GA;
    asset_source *Source;
};
internal added_asset
AddAsset(game_assets *Assets)
{
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

    u32 Index = Assets->DEBUGAssetType->OnePastLastAssetIndex++;
    asset_source *Source = Assets->AssetSources + Index;
    ga_asset *GA = Assets->Assets + Index;
    GA->FirstTagIndex = Assets->TagCount;
    GA->OnePastLastTagIndex = GA->FirstTagIndex;

    Assets->AssetIndex = Index;

    added_asset Result;
    Result.ID = Index;
    Result.GA = GA;
    Result.Source = Source;
    return(Result);
}

internal bitmap_id
AddBitmapAsset(game_assets *Assets, char *Filename, r32 AlignPercentageX = 0.5f, r32 AlignPercentageY = 0.5f)
{
    added_asset Asset = AddAsset(Assets);
    Asset.GA->Bitmap.AlignPercentage[0] = AlignPercentageX;
    Asset.GA->Bitmap.AlignPercentage[1] = AlignPercentageY;
    Asset.Source->Type = AssetType_Bitmap;
    Asset.Source->Bitmap.Filename = Filename;

    bitmap_id Result = {Asset.ID};
    return(Result);
}

internal bitmap_id
AddCharacterAsset(game_assets *Assets, loaded_font *Font, u32 Codepoint)
{
    added_asset Asset = AddAsset(Assets);
    Asset.GA->Bitmap.AlignPercentage[0] = 0.0f; // Set later by extraction.
    Asset.GA->Bitmap.AlignPercentage[1] = 0.0f; // Set later by extraction.
    Asset.Source->Type = AssetType_FontGlyph;
    Asset.Source->Glyph.Font = Font;
    Asset.Source->Glyph.Codepoint = Codepoint;

    bitmap_id Result = {Asset.ID};
    return(Result);
}

internal sound_id
AddSoundAsset(game_assets *Assets, char *Filename, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
{
    added_asset Asset = AddAsset(Assets);
    Asset.GA->Sound.SampleCount = SampleCount;
    Asset.GA->Sound.Chain = GASoundChain_None;
    Asset.Source->Type = AssetType_Sound;
    Asset.Source->Sound.Filename = Filename;
    Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;

    sound_id Result = {Asset.ID};
    return(Result);
}

internal font_id
AddFontAsset(game_assets *Assets, loaded_font *Font)
{
    added_asset Asset = AddAsset(Assets);
    Asset.GA->Font.CodepointCount = Font->CodepointCount;
    Asset.GA->Font.AscenderHeight = (r32)Font->TextMetric.tmAscent;
    Asset.GA->Font.DescenderHeight = (r32)Font->TextMetric.tmDescent;
    Asset.GA->Font.ExternalLeading = (r32)Font->TextMetric.tmExternalLeading;
    Asset.Source->Type = AssetType_Font;
    Asset.Source->Font.Font = Font;

    font_id Result = {Asset.ID};
    return(Result);
}

internal void
AddTag(game_assets *Assets, asset_tag_id ID, real32 Value)
{
    Assert(Assets->AssetIndex);

    ga_asset *GA = Assets->Assets + Assets->AssetIndex;
    ++GA->OnePastLastTagIndex;
    ga_tag *Tag = Assets->Tags + Assets->TagCount++;

    Tag->ID = ID;
    Tag->Value = Value;
}

internal void
EndAssetType(game_assets *Assets)
{
    Assert(Assets->DEBUGAssetType);
    Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;
}

internal void
WriteGA(game_assets *Assets, char *Filename)
{
    FILE *Out = fopen(Filename, "wb");
    if(Out)
    {
        ga_header Header = {};
        Header.MagicValue = GA_MAGIC_VALUE;
        Header.Version = GA_VERSION;
        Header.TagCount = Assets->TagCount;
        Header.AssetTypeCount = Asset_Count; // TODO: Do I really want to do this? Sparseness.
        Header.AssetCount = Assets->AssetCount;

        u32 TagArraySize = Header.TagCount*sizeof(ga_tag);
        u32 AssetTypeArraySize = Header.AssetTypeCount*sizeof(ga_asset_type);
        u32 AssetArraySize = Header.AssetCount*sizeof(ga_asset);

        Header.Tags = sizeof(Header);
        Header.AssetTypes = Header.Tags + TagArraySize;
        Header.Assets = Header.AssetTypes + AssetTypeArraySize;

        fwrite(&Header, sizeof(Header), 1, Out);
        fwrite(Assets->Tags, TagArraySize, 1, Out);
        fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
        fseek(Out, AssetArraySize, SEEK_CUR);
        for(u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex)
        {
            asset_source *Source = Assets->AssetSources + AssetIndex;
            ga_asset *Dest = Assets->Assets + AssetIndex;

            Dest->DataOffset = ftell(Out);

            if(Source->Type == AssetType_Sound)
            {
                loaded_sound WAV = LoadWAV(Source->Sound.Filename, 
                                           Source->Sound.FirstSampleIndex,
                                           Dest->Sound.SampleCount);

                Dest->Sound.SampleCount = WAV.SampleCount;
                Dest->Sound.ChannelCount = WAV.ChannelCount;

                for(u32 ChannelIndex = 0; ChannelIndex < WAV.ChannelCount; ++ChannelIndex)
                {
                    fwrite(WAV.Samples[ChannelIndex], Dest->Sound.SampleCount*sizeof(s16), 1, Out);
                }

                free(WAV.Free);
            }
            else if(Source->Type == AssetType_Font)
            {
                loaded_font *Font = Source->Font.Font;
                u32 HorizontalAdvanceSize = sizeof(r32)*Font->CodepointCount*Font->CodepointCount;
                u32 CodepointSize = Font->CodepointCount*sizeof(bitmap_id);
                fwrite(Font->BitmapIDs, CodepointSize, 1, Out);
                fwrite(Font->HorizontalAdvance, HorizontalAdvanceSize, 1, Out);
            }
            else
            {
                loaded_bitmap Bitmap;
                if(Source->Type == AssetType_FontGlyph)
                {
                    Bitmap = LoadGlyphBitmap(Source->Glyph.Font, Source->Glyph.Codepoint, Dest);
                }
                else
                {
                    Assert(Source->Type == AssetType_Bitmap);
                    Bitmap = LoadBMP(Source->Bitmap.Filename);
                }

                Dest->Bitmap.Dim[0] = Bitmap.Width;
                Dest->Bitmap.Dim[1] = Bitmap.Height;

                Assert((Bitmap.Width*4) == Bitmap.Pitch);
                fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);

                free(Bitmap.Free);
            }
        }
        fseek(Out, (u32)Header.Assets, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, Out);

        fclose(Out);
    }
    else
    {
        printf("ERROR: Couldn't open file :(\n");
    }
}

internal void
Initialise(game_assets *Assets)
{
    Assets->TagCount = 1;
    Assets->AssetCount = 1;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;

    Assets->AssetTypeCount = Asset_Count;
    memset(Assets->AssetTypes, 0, sizeof(Assets->AssetTypes));
}

internal void
WriteFonts(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialise(Assets);

    loaded_font *DebugFont = LoadFont("c:/Windows/Fonts/arial.ttf", "Arial", ('~' + 1));
//        AddCharacterAsset(Assets, "c:/Windows/Fonts/cour.ttf", "Courier New", Character);

    BeginAssetType(Assets, Asset_FontGlyph);
    for(u32 Character = '!'; Character <= '~'; ++Character)
    {
        DebugFont->BitmapIDs[Character] = AddCharacterAsset(Assets, DebugFont, Character);
    }
    EndAssetType(Assets);

    // TODO: This is kinda janky, because it means you have to get this
    // order right always.
    BeginAssetType(Assets, Asset_Font);
    AddFontAsset(Assets, DebugFont);
    EndAssetType(Assets);

    WriteGA(Assets, "testfonts.ga");
}

internal void
WriteHero(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialise(Assets);

    real32 AngleRight = 0.0f*Tau32;
    real32 AngleBack = 0.25f*Tau32;
    real32 AngleLeft = 0.5f*Tau32;
    real32 AngleFront = 0.75f*Tau32;

    r32 HeroAlign[] = {0.5f, 0.156682029f};

    BeginAssetType(Assets, Asset_Head);
    AddBitmapAsset(Assets, "test/test_hero_right_head.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_head.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_head.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_head.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Cape);
    AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Torso);
    AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp", HeroAlign[0], HeroAlign[1]);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    WriteGA(Assets, "test1.ga");
}

internal void
WriteNonHero(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialise(Assets);

    BeginAssetType(Assets, Asset_Shadow);
    AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", 0.5f, 0.156682029f);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tree);
    AddBitmapAsset(Assets, "test2/tree00.bmp", 0.493827164f, 0.295652181f);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Sword);
    AddBitmapAsset(Assets, "test2/rock03.bmp", 0.5f, 0.65625f);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Grass);
    AddBitmapAsset(Assets, "test2/grass00.bmp");
    AddBitmapAsset(Assets, "test2/grass01.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tuft);
    AddBitmapAsset(Assets, "test2/tuft00.bmp");
    AddBitmapAsset(Assets, "test2/tuft01.bmp");
    AddBitmapAsset(Assets, "test2/tuft02.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Stone);
    AddBitmapAsset(Assets, "test2/ground00.bmp");
    AddBitmapAsset(Assets, "test2/ground01.bmp");
    AddBitmapAsset(Assets, "test2/ground02.bmp");
    AddBitmapAsset(Assets, "test2/ground03.bmp");
    EndAssetType(Assets);

    WriteGA(Assets, "test2.ga");
}

internal void
WriteSounds(void)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;
    Initialise(Assets);

    BeginAssetType(Assets, Asset_Bloop);
    AddSoundAsset(Assets, "test3/bloop_00.wav");
    AddSoundAsset(Assets, "test3/bloop_01.wav");
    AddSoundAsset(Assets, "test3/bloop_02.wav");
    AddSoundAsset(Assets, "test3/bloop_03.wav");
    AddSoundAsset(Assets, "test3/bloop_04.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Crack);
    AddSoundAsset(Assets, "test3/crack_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Drop);
    AddSoundAsset(Assets, "test3/drop_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Glide);
    AddSoundAsset(Assets, "test3/glide_00.wav");
    EndAssetType(Assets);

    u32 OneMusicChunk = 10*48000;
    u32 TotalMusicSampleCount = 7468095;
    BeginAssetType(Assets, Asset_Music);
    for(u32 FirstSampleIndex = 0; FirstSampleIndex < TotalMusicSampleCount; FirstSampleIndex += OneMusicChunk)
    {
        u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        if(SampleCount > OneMusicChunk)
        {
            SampleCount = OneMusicChunk;
        }
        sound_id ThisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, SampleCount);
        if((FirstSampleIndex + OneMusicChunk) < TotalMusicSampleCount)
        {
            Assets->Assets[ThisMusic.Value].Sound.Chain = GASoundChain_Advance;
        }
    }
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "test3/puhp_00.wav");
    AddSoundAsset(Assets, "test3/puhp_01.wav");
    EndAssetType(Assets);

    WriteGA(Assets, "test3.ga");
}

int
main(int ArgCount, char **Args)
{
    InitialiseFontDC();

    WriteFonts();
    WriteNonHero();
    WriteHero();
    WriteSounds();
}

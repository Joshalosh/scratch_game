
#include "test_asset_builder.h"

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
    s32 Width;
    s32 Height;
    s32 Pitch;
    void *Memory;

    void *Free;
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
        printf("ERROR: Cannot open file %s. \n", Filename);
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

struct riff_iterator
{
    uint8_t *At;
    uint8_t *Stop;
};

inline riff_iterator
ParseChunkAt(void *At, void *Stop)
{
    riff_iterator Iter;

    Iter.At = (uint8_t *)At;
    Iter.Stop = (uint8_t *)Stop;

    return(Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    uint32_t Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(WAVE_chunk) + Size;

    return(Iter);
}

inline bool32
IsValid(riff_iterator Iter)
{
    bool32 Result = (Iter.At < Iter.Stop);

    return(Result);
}

inline void *
GetChunkData(riff_iterator Iter)
{
    void *Result = (Iter.At + sizeof(WAVE_chunk));

    return(Result);
}

inline uint32_t
GetType(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    uint32_t Result = Chunk->ID;

    return(Result);
}

inline uint32_t
GetChunkDataSize(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    uint32_t Result = Chunk->Size;

    return(Result);
}

struct loaded_sound
{
    u32 SampleCount;
    u32 ChannelCount;
    s16 *Samples[2];

    void *Free;
};

internal loaded_sound
LoadWAV(char *Filename, uint32_t SectionFirstSampleIndex, uint32_t SectionSampleCount)
{
    loaded_sound Result = {};

    entire_file ReadResult = ReadEntireFile(Filename);
    if(ReadResult.ContentsSize != 0)
    {
        Result.Free = ReadResult.Contents;

        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

        uint32_t ChannelCount = 0;
        uint32_t SampleDataSize = 0;
        int16_t *SampleData = 0;
        for(riff_iterator Iter = ParseChunkAt(Header + 1, (uint8_t *)(Header + 1) + Header->Size - 4);
            IsValid(Iter);
            Iter = NextChunk(Iter))
        {
            switch(GetType(Iter))
            {
                case WAVE_ChunkID_fmt:
                {
                    WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iter);
                    Assert(fmt->wFormatTag == 1);
                    Assert(fmt->nSamplesPerSec == 48000);
                    Assert(fmt->wBitsPerSample == 16);
                    Assert(fmt->nBlockAlign == (sizeof(int16_t)*fmt->nChannels));
                    ChannelCount = fmt->nChannels;
                } break;

                case WAVE_ChunkID_data:
                {
                    SampleData = (int16_t *)GetChunkData(Iter);
                    SampleDataSize = GetChunkDataSize(Iter);
                } break;
            }
        }

        Assert(ChannelCount && SampleData);

        Result.ChannelCount = ChannelCount;
        u32 SampleCount = SampleDataSize / (ChannelCount*sizeof(int16_t));
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
            for(uint32_t SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
            {
                SampleData[2*SampleIndex + 0] = (int16_t)SampleIndex;
                SampleData[2*SampleIndex + 1] = (int16_t)SampleIndex;
            }
#endif

            for(uint32_t SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
            {
                int16_t Source = SampleData[2*SampleIndex];
                SampleData[2*SampleIndex] = SampleData[SampleIndex];
                SampleData[SampleIndex] = Source;
            }
        }
        else
        {
            Assert(!"Invalid channel count in WAV file");
        }

        // TODO: Load right channels.
        b32 AtEnd = true;
        Result.ChannelCount = 1;
        if(SectionSampleCount)
        {
            Assert((SectionFirstSampleIndex + SectionSampleCount) <= SampleCount);
            AtEnd = ((SectionFirstSampleIndex + SectionSampleCount) == SampleCount);
            SampleCount = SectionSampleCount;
            for(uint32_t ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex)
            {
                Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
            }
        }

        if(AtEnd)
        {
            for(u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex)
            {
                for(u32 SampleIndex = SampleCount; SampleIndex < (SampleCount + 8); ++SampleIndex)
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

internal bitmap_id
AddBitmapAsset(game_assets *Assets, char *Filename, r32 AlignPercentageX = 0.5f, r32 AlignPercentageY = 0.5f)
{
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

    bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
    asset_source *Source = Assets->AssetSources + Result.Value;
    ga_asset *GA = Assets->Assets + Result.Value;
    GA->FirstTagIndex = Assets->TagCount;
    GA->OnePastLastTagIndex = GA->FirstTagIndex;
    GA->Bitmap.AlignPercentage[0] = AlignPercentageX;
    GA->Bitmap.AlignPercentage[1] = AlignPercentageY;

    Source->Type = AssetType_Bitmap;
    Source->Filename = Filename;

    Assets->AssetIndex = Result.Value;

    return(Result);
}

internal sound_id
AddSoundAsset(game_assets *Assets, char *Filename, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
{
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

    sound_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
    asset_source *Source = Assets->AssetSources + Result.Value;
    ga_asset *GA = Assets->Assets + Result.Value;
    GA->FirstTagIndex = Assets->TagCount;
    GA->OnePastLastTagIndex = GA->FirstTagIndex;
    GA->Sound.SampleCount = SampleCount;
    GA->Sound.NextIDToPlay = 0;

    Source->Type = AssetType_Sound;
    Source->Filename = Filename;
    Source->FirstSampleIndex = FirstSampleIndex;

    Assets->AssetIndex = Result.Value;

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

int
main(int ArgCount, char **Args)
{
    game_assets Assets_;
    game_assets *Assets = &Assets_;

    Assets->TagCount = 1;
    Assets->AssetCount = 1;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;

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

    //
    //
    //

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
    sound_id LastMusic = {0};
    for(u32 FirstSampleIndex = 0; FirstSampleIndex < TotalMusicSampleCount; FirstSampleIndex += OneMusicChunk)
    {
        u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        if(SampleCount > OneMusicChunk)
        {
            SampleCount = OneMusicChunk;
        }
        sound_id ThisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, OneMusicChunk);
        if(LastMusic.Value)
        {
            Assets->Assets[LastMusic.Value].Sound.NextIDToPlay = ThisMusic.Value;
        }
        LastMusic = ThisMusic;
    }
    EndAssetType(Assets);
    
    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "test3/puhp_00.wav");
    AddSoundAsset(Assets, "test3/puhp_01.wav");
    EndAssetType(Assets);


    FILE *Out = fopen("test.ga", "wb");
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
                loaded_sound WAV = LoadWAV(Source->Filename, Source->FirstSampleIndex, Dest->Sound.SampleCount);

                Dest->Sound.SampleCount = WAV.SampleCount;
                Dest->Sound.ChannelCount = WAV.ChannelCount;

                for(u32 ChannelIndex = 0; ChannelIndex < WAV.ChannelCount; ++ChannelIndex)
                {
                    fwrite(WAV.Samples[ChannelIndex], Dest->Sound.SampleCount*sizeof(s16), 1, Out);
                }

                free(WAV.Free);
            }
            else
            {
                Assert(Source->Type == AssetType_Bitmap);

                loaded_bitmap Bitmap = LoadBMP(Source->Filename);

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
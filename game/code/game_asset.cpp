
#if 0
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

internal loaded_bitmap
DEBUGLoadBMP(char *Filename, v2 AlignPercentage = V2(0.5f, 0.5f))
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(Filename);
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.AlignPercentage = AlignPercentage;
        Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);

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

internal loaded_sound
DEBUGLoadWAV(char *Filename, uint32_t SectionFirstSampleIndex, uint32_t SectionSampleCount)
{
    loaded_sound Result = {};

    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(Filename);
    if(ReadResult.ContentsSize != 0)
    {
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

#endif

struct load_bitmap_work
{
    game_assets *Assets;
    bitmap_id ID;
    task_with_memory *Task;
    loaded_bitmap *Bitmap;

    asset_state FinalState;
};
internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
{
    load_bitmap_work *Work = (load_bitmap_work *)Data;
    
    ga_asset *GAAsset = &Work->Assets->Assets[Work->ID.Value];
    ga_bitmap *Info = &GAAsset->Bitmap;
    loaded_bitmap *Bitmap = Work->Bitmap;

    Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
    Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
    Bitmap->Width = Info->Dim[0];
    Bitmap->Height = Info->Dim[1];
    Bitmap->Pitch = 4*Info->Dim[0];
    Bitmap->Memory = Work->Assets->GAContents + GAAsset->DataOffset;

    CompletePreviousWritesBeforeFutureWrites;

    Work->Assets->Slots[Work->ID.Value].Bitmap = Work->Bitmap;
    Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;

    EndTaskWithMemory(Work->Task);
}

internal void
LoadBitmap(game_assets *Assets, bitmap_id ID)
{
    if(ID.Value &&
       (AtomicCompareExchangeUInt32((uint32_t *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded))
    {
        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
        if(Task)
        {
            load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);

            Work->Assets = Assets;
            Work->ID = ID;
            Work->Task = Task;
            Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
            Work->FinalState = AssetState_Loaded;

            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
        }
        else 
        {
            Assets->Slots[ID.Value].State = AssetState_Unloaded;
        }
    }
}

struct load_sound_work
{
    game_assets *Assets;
    sound_id ID;
    task_with_memory *Task;
    loaded_sound *Sound;

    asset_state FinalState;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
{
    load_sound_work *Work = (load_sound_work *)Data;

    ga_asset *GAAsset = &Work->Assets->Assets[Work->ID.Value];
    ga_sound *Info = &GAAsset->Sound;
    loaded_sound *Sound = Work->Sound;
    
    Sound->SampleCount = Info->SampleCount;
    Sound->ChannelCount = Info->ChannelCount;
    Assert(Sound->ChannelCount < ArrayCount(Sound->Samples));
    u64 SampleDataOffset = GAAsset->DataOffset;
    for(u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex)
    {
        Sound->Samples[ChannelIndex] = (s16 *)(Work->Assets->GAContents + SampleDataOffset);
        SampleDataOffset += Sound->SampleCount*sizeof(s16);
    }
    
    CompletePreviousWritesBeforeFutureWrites;

    Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
    Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;

    EndTaskWithMemory(Work->Task);
}

internal void
LoadSound(game_assets *Assets, sound_id ID)
{
    if(ID.Value &&
       (AtomicCompareExchangeUInt32((uint32_t *)&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded))
    {
        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
        if(Task)
        {
            load_sound_work *Work = PushStruct(&Task->Arena, load_sound_work);

            Work->Assets = Assets;
            Work->ID = ID;
            Work->Task = Task;
            Work->Sound = PushStruct(&Assets->Arena, loaded_sound);
            Work->FinalState = AssetState_Loaded;

            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadSoundWork, Work);
        }
        else 
        {
            Assets->Slots[ID.Value].State = AssetState_Unloaded;
        }
    }
}

internal uint32_t
GetBestMatchAssetFrom(game_assets *Assets, asset_type_id TypeID,
                      asset_vector *MatchVector, asset_vector *WeightVector)
{
    uint32_t Result = 0;

    real32 BestDiff = Real32Maximum;
    asset_type *Type = Assets->AssetTypes + TypeID;
    for(uint32_t AssetIndex = Type->FirstAssetIndex;
        AssetIndex < Type->OnePastLastAssetIndex;
        ++AssetIndex)
    {
        ga_asset *Asset = Assets->Assets + AssetIndex;

        real32 TotalWeightedDiff = 0.0f;
        for(uint32_t TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePastLastTagIndex; ++TagIndex)
        {
            ga_tag *Tag = Assets->Tags + TagIndex;

            real32 A = MatchVector->E[Tag->ID];
            real32 B = Tag->Value;
            real32 D0 = AbsoluteValue(A - B);
            real32 D1 = AbsoluteValue((A - Assets->TagRange[Tag->ID]*SignOf(A)) - B);
            real32 Difference = Minimum(D0, D1);

            real32 Weighted = WeightVector->E[Tag->ID]*Difference;
            TotalWeightedDiff += Weighted;
        }

        if(BestDiff > TotalWeightedDiff)
        {
            BestDiff = TotalWeightedDiff;
            Result = AssetIndex;
        }
    }

    return(Result);
}

internal uint32_t
GetRandomSlotFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    uint32_t Result = 0;

    asset_type *Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
        uint32_t Count = (Type->OnePastLastAssetIndex - Type->FirstAssetIndex);
        uint32_t Choice = RandomChoice(Series, Count);
        Result = Type->FirstAssetIndex + Choice;
    }

    return(Result);
}

internal uint32_t
GetFirstSlotFrom(game_assets *Assets, asset_type_id TypeID)
{
    uint32_t Result = 0;

    asset_type *Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
        Result = Type->FirstAssetIndex;
    }

    return(Result);
}

inline bitmap_id
GetBestMatchBitmapFrom(game_assets *Assets, asset_type_id TypeID,
                       asset_vector *MatchVector, asset_vector *WeightVector)
{
    bitmap_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
    return(Result);
}

inline bitmap_id
GetFirstBitmapFrom(game_assets *Assets, asset_type_id TypeID)
{
    bitmap_id Result = {GetFirstSlotFrom(Assets, TypeID)};
    return(Result);
}

inline bitmap_id
GetRandomBitmapFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    bitmap_id Result = {GetRandomSlotFrom(Assets, TypeID, Series)};
    return(Result);
}

inline sound_id
GetBestMatchSoundFrom(game_assets *Assets, asset_type_id TypeID,
                      asset_vector *MatchVector, asset_vector *WeightVector)
{
    sound_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
    return(Result);
}

inline sound_id
GetFirstSoundFrom(game_assets *Assets, asset_type_id TypeID)
{
    sound_id Result = {GetFirstSlotFrom(Assets, TypeID)};
    return(Result);
}

inline sound_id
GetRandomSoundFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    sound_id Result = {GetRandomSlotFrom(Assets, TypeID, Series)};
    return(Result);
}

#if 0

internal void
BeginAssetType(game_assets *Assets, asset_type_id TypeID)
{
    Assert(Assets->DEBUGAssetType == 0);

    Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
    Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
    Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(game_assets *Assets, char *Filename, v2 AlignPercentage = V2(0.5f, 0.5f))
{
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

    bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
    asset *Asset = Assets->Assets + Result.Value;
    Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
    Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
    Asset->Bitmap.Filename = PushString(&Assets->Arena, Filename);
    Asset->Bitmap.AlignPercentage = AlignPercentage;

    Assets->DEBUGAsset = Asset;

    return(Result);
}

internal sound_id
AddSoundAsset(game_assets *Assets, char *Filename, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
{
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

    sound_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
    asset *Asset = Assets->Assets + Result.Value;
    Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
    Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
    Asset->Sound.Filename = PushString(&Assets->Arena, Filename);
    Asset->Sound.FirstSampleIndex = FirstSampleIndex;
    Asset->Sound.SampleCount = SampleCount;
    Asset->Sound.NextIDToPlay.Value = 0;

    Assets->DEBUGAsset = Asset;

    return(Result);
}

internal void
AddTag(game_assets *Assets, ga_tag_id ID, real32 Value)
{
    Assert(Assets->DEBUGAsset);

    ++Assets->DEBUGAsset->OnePastLastTagIndex;
    ga_tag *Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;

    Tag->ID = ID;
    Tag->Value = Value;
}

internal void
EndAssetType(game_assets *Assets)
{
    Assert(Assets->DEBUGAssetType);
    Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
    Assets->DEBUGAssetType = 0;
    Assets->DEBUGAsset = 0;
}

#endif

internal game_assets *
AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
{
    game_assets *Assets = PushStruct(Arena, game_assets);
    SubArena(&Assets->Arena, Arena, Size);
    Assets->TranState = TranState;

    for(uint32_t TagType = 0; TagType < Tag_Count; ++TagType)
    {
        Assets->TagRange[TagType] = 1000000.0f;
    }
    Assets->TagRange[Tag_FacingDirection] = Tau32;

    {
        platform_file_group FileGroup = PlatformGetAllFilesOfTypeBegin("ga");
        Assets->FileCount = FileGroup.FileCount;
        Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
        for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex;

            File->Handle = PlatformOpenFile(FileGroup, FileIndex);
            PlatformReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);

            if(Header->MagicValue != GA_MAGIC_VALUE)
            {
                PlatformFileError(File->Handle, "GA file has an invalid magic value.");
            }

            if(Header->Version > GA_VERSION)
            {
                PlatformFileError(File->Handle, "GA file is a later version.");
            }

            if(PlatformNoFileErrors(File->Handle))
            {
                Assets->TagCount += Header->TagCount;
                Assets->AssetCount += Header->AssetCount;
            }
            else
            {
                // TODO: Eventually, have some way of notifying users of fucked up files.
                InvalidCodePath;
            }
        }
        PlatformGetAllFilesOfTypeEnd(FileGroup);
    }

    Assets->Assets = PushArray(Arena, Assets->AssetCount, ga_asset);
    Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
    Assets->Tags = PushArray(Arena, Assets->TagCount, ga_tag);

    u32 AssetCount = 0;
    u32 TagCount = 0;
    for(u32 AssetTypeID = 0; AssetTypeID < Asset_Count; ++AssetTypeID)
    {
        for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex;
        }
    }

    Assert(AssetCount == Assets->AssetCount);
    Assert(TagCount == Assets->TagCount);

    debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile("test.ga");
    if(ReadResult.ContentsSize != 0)
    {
        ga_header *Header = (ga_header *)ReadResult.Contents;

        Assets->AssetCount = Header->AssetCount;
        Assets->Assets = (ga_asset *)((u8 *)ReadResult.Contents + Header->Assets);
        Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);

        Assets->TagCount = Header->TagCount;
        Assets->Tags = (ga_tag *)((u8 *)ReadResult.Contents + Header->Tags);

        ga_asset_type *GAAssetTypes = (ga_asset_type *)((u8 *)ReadResult.Contents + Header->AssetTypes);

        for(u32 Index = 0; Index < Header->AssetTypeCount; ++Index)
        {
            ga_asset_type *Source = GAAssetTypes + Index;

            if(Source->TypeID < Asset_Count)
            {
                asset_type *Dest = Assets->AssetTypes + Source->TypeID;
                // TODO: Support merging.
                Assert(Dest->FirstAssetIndex == 0);
                Assert(Dest->OnePastLastAssetIndex == 0);
                Dest->FirstAssetIndex = Source->FirstAssetIndex;
                Dest->OnePastLastAssetIndex = Source->OnePastLastAssetIndex;
            }
        }

        Assets->GAContents = (u8 *)ReadResult.Contents;
    }

#if 0
    Assets->DEBUGUsedAssetCount = 1;

    BeginAssetType(Assets, Asset_Shadow);
    AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", V2(0.5f, 0.156682029f));
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tree);
    AddBitmapAsset(Assets, "test2/tree00.bmp", V2(0.493827164f, 0.295652181f));
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Sword);
    AddBitmapAsset(Assets, "test2/rock03.bmp", V2(0.5f, 0.65625f));
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

    v2 HeroAlign = {0.5f, 0.156682029f};

    BeginAssetType(Assets, Asset_Head);
    AddBitmapAsset(Assets, "test/test_hero_right_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Cape);
    AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Torso);
    AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp", HeroAlign);
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
        if(IsValid(LastMusic))
        {
            Assets->Assets[LastMusic.Value].Sound.NextIDToPlay = ThisMusic;;
        }
        LastMusic = ThisMusic;
    }
    EndAssetType(Assets);
    
    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "test3/puhp_00.wav");
    AddSoundAsset(Assets, "test3/puhp_01.wav");
    EndAssetType(Assets);
#endif

    return(Assets);
}

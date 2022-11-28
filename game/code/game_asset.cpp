
struct load_asset_work
{
    task_with_memory *Task;
    asset_slot *Slot;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    asset_state FinalState;
};
internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
    load_asset_work *Work = (load_asset_work *)Data;

    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);

    CompletePreviousWritesBeforeFutureWrites;

    // TODO: Should I actually fill in random data here and set to final state anyway,
    // or continue trying to load?
    if(PlatformNoFileErrors(Work->Handle))
    {
        Work->Slot->State = Work->FinalState;
    }

    EndTaskWithMemory(Work->Task);
}

inline platform_file_handle *
GetFileHandleFor(game_assets *Assets, u32 FileIndex)
{
    Assert(FileIndex < Assets->FileCount);
    platform_file_handle *Result = Assets->Files[FileIndex].Handle;

    return(Result);
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
            asset *Asset = Assets->Assets + ID.Value;
            ga_bitmap *Info = &Asset->GA.Bitmap;
            loaded_bitmap *Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

            Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
            Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
            Bitmap->Width = Info->Dim[0];
            Bitmap->Height = Info->Dim[1];
            Bitmap->Pitch = 4*Info->Dim[0];
            u32 MemorySize = Bitmap->Pitch*Bitmap->Height;
            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

            load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Slot = Assets->Slots + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->GA.DataOffset;
            Work->Size = MemorySize;
            Work->Destination = Bitmap->Memory;
            Work->FinalState = AssetState_Loaded;
            Work->Slot->Bitmap = Bitmap;

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else
        {
            Assets->Slots[ID.Value].State = AssetState_Unloaded;
        }
    }
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
            asset *Asset = Assets->Assets + ID.Value;
            ga_sound *Info = &Asset->GA.Sound;

            loaded_sound *Sound = PushStruct(&Assets->Arena, loaded_sound);
            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;
            u32 ChannelSize = Sound->SampleCount*sizeof(s16);
            u32 MemorySize = Sound->ChannelCount*ChannelSize;

            void *Memory = PushSize(&Assets->Arena, MemorySize);

            s16 *SoundAt = (s16 *)Memory;
            for(u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex)
            {
                Sound->Samples[ChannelIndex] = SoundAt;
                SoundAt += ChannelSize;
            }

            load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Slot = Assets->Slots + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->GA.DataOffset;
            Work->Size = MemorySize;
            Work->Destination = Memory;
            Work->FinalState = AssetState_Loaded;
            Work->Slot->Sound = Sound;

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
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
        asset *Asset = Assets->Assets + AssetIndex;

        real32 TotalWeightedDiff = 0.0f;
        for(uint32_t TagIndex = Asset->GA.FirstTagIndex; TagIndex < Asset->GA.OnePastLastTagIndex; ++TagIndex)
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

    Assets->TagCount = 0;
    Assets->AssetCount = 0;

    {
        platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin("ga");
        Assets->FileCount = FileGroup.FileCount;
        Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
        for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex;

            File->TagBase = Assets->TagCount;

            u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(ga_asset_type);

            ZeroStruct(File->Header);
            File->Handle = Platform.OpenFile(FileGroup, FileIndex);
            Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
            File->AssetTypeArray = (ga_asset_type *)PushSize(Arena, AssetTypeArraySize);
            Platform.ReadDataFromFile(File->Handle, File->Header.AssetTypes,
                                     AssetTypeArraySize, File->AssetTypeArray);

            if(File->Header.MagicValue != GA_MAGIC_VALUE)
            {
                Platform.FileError(File->Handle, "GA file has an invalid magic value.");
            }

            if(File->Header.Version > GA_VERSION)
            {
                Platform.FileError(File->Handle, "GA file is a later version.");
            }

            if(PlatformNoFileErrors(File->Handle))
            {
                Assets->TagCount += File->Header.TagCount;
                Assets->AssetCount += File->Header.AssetCount;
            }
            else
            {
                // TODO: Eventually, have some way of notifying users of fucked up files.
                InvalidCodePath;
            }
        }
        Platform.GetAllFilesOfTypeEnd(FileGroup);
    }

    // Allocate all metadata space.
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
    Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
    Assets->Tags = PushArray(Arena, Assets->TagCount, ga_tag);

    // Load tags.
    for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
    {
        asset_file *File = Assets->Files + FileIndex;
        if(PlatformNoFileErrors(File->Handle))
        {
            u32 TagArraySize = sizeof(ga_tag)*File->Header.TagCount;
            Platform.ReadDataFromFile(File->Handle, File->Header.Tags, TagArraySize, Assets->Tags + File->TagBase);
        }
    }

    // TODO: How would I do this in a way that could scale gracefully to 
    // hundred of asset pack files.
    u32 AssetCount = 0;
    for(u32 DestTypeID = 0; DestTypeID < Asset_Count; ++DestTypeID)
    {
        asset_type *DestType = Assets->AssetTypes + DestTypeID;
        DestType->FirstAssetIndex = AssetCount;

        for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex;
            if(PlatformNoFileErrors(File->Handle))
            {
                for(u32 SourceIndex = 0; SourceIndex < File->Header.AssetTypeCount; ++SourceIndex)
                {
                    ga_asset_type *SourceType = File->AssetTypeArray + SourceIndex;

                    if(SourceType->TypeID == DestTypeID)
                    {
                        u32 AssetCountForType = (SourceType->OnePastLastAssetIndex - SourceType->FirstAssetIndex);

                        temporary_memory TempMem = BeginTemporaryMemory(&TranState->TranArena);
                        ga_asset *GAAssetArray = PushArray(&TranState->TranArena, AssetCountForType, ga_asset);
                        Platform.ReadDataFromFile(File->Handle,
                                                  File->Header.Assets + SourceType->FirstAssetIndex*sizeof(ga_asset),
                                                  AssetCountForType*sizeof(ga_asset),
                                                  GAAssetArray);
                        for(u32 AssetIndex = 0; AssetIndex < AssetCountForType; ++AssetIndex)
                        {
                            ga_asset *GAAsset = GAAssetArray + AssetIndex;

                            Assert(AssetCount < Assets->AssetCount);
                            asset *Asset = Assets->Assets + AssetCount++;

                            Asset->FileIndex = FileIndex;
                            Asset->GA = *GAAsset;
                            Asset->GA.FirstTagIndex += File->TagBase;
                            Asset->GA.OnePastLastTagIndex += File->TagBase;
                        }

                        EndTemporaryMemory(TempMem);
                    }
                }
            }
        }

        DestType->OnePastLastAssetIndex = AssetCount;
    }

    Assert(AssetCount == Assets->AssetCount);

#if 0
    debug_read_file_result ReadResult = Platform.DEBUGReadEntireFile("test.ga");
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
#endif

    return(Assets);
}

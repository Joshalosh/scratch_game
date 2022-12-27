
struct load_asset_work
{
    task_with_memory *Task;
    asset *Asset;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    u32 FinalState;
};
internal void
LoadAssetWorkDirectly(load_asset_work *Work)
{
    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);

    CompletePreviousWritesBeforeFutureWrites;

    if(!PlatformNoFileErrors(Work->Handle))
    {
        ZeroSize(Work->Size, Work->Destination);
    }

    Work->Asset->State = Work->FinalState;
}
internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
    load_asset_work *Work = (load_asset_work *)Data;

    LoadAssetWorkDirectly(Work);

    EndTaskWithMemory(Work->Task);
}

inline platform_file_handle *
GetFileHandleFor(game_assets *Assets, u32 FileIndex)
{
    Assert(FileIndex < Assets->FileCount);
    platform_file_handle *Result = &Assets->Files[FileIndex].Handle;

    return(Result);
}

internal asset_memory_block *
InsertBlock(asset_memory_block *Prev, u64 Size, void *Memory)
{
    Assert(Size > sizeof(asset_memory_block));
    asset_memory_block *Block = (asset_memory_block *)Memory;
    Block->Flags = 0;
    Block->Size = Size - sizeof(asset_memory_block);
    Block->Prev = Prev;
    Block->Next = Prev->Next;
    Block->Prev->Next = Block;
    Block->Next->Prev = Block;
    return(Block);
}

internal asset_memory_block *
FindBlockForSize(game_assets *Assets, memory_index Size)
{
    asset_memory_block *Result = 0;

    // TODO: Best match block.
    for(asset_memory_block *Block = Assets->MemorySentinel.Next;
        Block != &Assets->MemorySentinel;
        Block = Block->Next)
    {
        if(!(Block->Flags & AssetMemory_Used))
        {
            if(Block->Size >= Size)
            {
                Result = Block;
                break;
            }
        }
    }

    return(Result);
}

internal b32
MergeIfPossible(game_assets *Assets, asset_memory_block *First, asset_memory_block *Second)
{
    b32 Result = false;

    if((First != &Assets->MemorySentinel) &&
       (Second != &Assets->MemorySentinel))
    {
        if(!(First->Flags & AssetMemory_Used) &&
           !(Second->Flags & AssetMemory_Used))
        {
            u8 *ExpectedSecond = (u8 *)First + sizeof(asset_memory_block) + First->Size;
            if((u8 *)Second == ExpectedSecond)
            {
                Second->Next->Prev = Second->Prev;
                Second->Prev->Next = Second->Next;

                First->Size += sizeof(asset_memory_block) + Second->Size;

                Result = true;
            }
        }
    }

    return(Result);
}

internal asset_memory_header *
AcquireAssetMemory(game_assets *Assets, u32 Size, u32 Index) // Changed from the name 'AssetIndex' to comply with warning 4457.
{
    asset_memory_header *Result = 0;

    BeginAssetLock(Assets);

    asset_memory_block *Block = FindBlockForSize(Assets, Size);
    for(;;)
    {
        if(Block && (Size <=Block->Size))
        {
            Block->Flags |= AssetMemory_Used;

            Result = (asset_memory_header *)(Block + 1);

            memory_index RemainingSize = Block->Size - Size;
            memory_index BlockSplitThreshold = 4096; // TODO: Set this based on the smallest asset size.
            if(RemainingSize > BlockSplitThreshold)
            {
                Block->Size -= RemainingSize;
                InsertBlock(Block, RemainingSize, (u8 *)Result + Size);
            }
            else
            {
                // TODO: Actually record the unused portion of the memory in a 
                // block so that I can do the merge on blocks when neigbors are freed.
            }

            break;
        }
        else
        {
            for(asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                Header != &Assets->LoadedAssetSentinel;
                Header = Header->Prev)
            {
                asset *Asset = Assets->Assets + Header->AssetIndex;
                if(Asset->State >= AssetState_Loaded)
                {
                    u32 AssetIndex = Header->AssetIndex;
                    asset *Asset = Assets->Assets + AssetIndex;

                    Assert(Asset->State == AssetState_Loaded);

                    RemoveAssetHeaderFromList(Header);

                    Block = (asset_memory_block *)Asset->Header - 1;
                    Block->Flags &= ~AssetMemory_Used;

                    if(MergeIfPossible(Assets, Block->Prev, Block))
                    {
                        Block = Block->Prev;
                    }

                    MergeIfPossible(Assets, Block, Block->Next);

                    Asset->State = AssetState_Unloaded;
                    Asset->Header = 0;
                    break;
                }
            }
        }
    }

    if(Result)
    {
        Result->AssetIndex = Index; // This was originally 'AssetIndex' but it's been changed to index now.
        Result->TotalSize = Size;
        InsertAssetHeaderAtFront(Assets, Result);
    }

    EndAssetLock(Assets);

    return(Result);
}

struct asset_memory_size
{
    u32 Total;
    u32 Data;
    u32 Section;
};

internal void
LoadBitmap(game_assets *Assets, bitmap_id ID, b32 Immediate)
{
    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value &&
       (AtomicCompareExchangeUInt32((uint32_t *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded))
    {
        task_with_memory *Task = 0;

        if(!Immediate || Task)
        {
            Task = BeginTaskWithMemory(Assets->TranState);
        }

        if(Immediate || Task)
        {
            asset *Asset = Assets->Assets + ID.Value;
            ga_bitmap *Info = &Asset->GA.Bitmap;

            asset_memory_size Size = {};
            u32 Width = Info->Dim[0];
            u32 Height = Info->Dim[1];
            Size.Section = 4*Width;
            Size.Data = Height*Size.Section;
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value);

            loaded_bitmap *Bitmap = &Asset->Header->Bitmap;
            Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
            Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
            Bitmap->Width = Info->Dim[0];
            Bitmap->Height = Info->Dim[1];
            Bitmap->Pitch = Size.Section;
            Bitmap->Memory = (Asset->Header + 1);

            load_asset_work Work;
            Work.Task = Task;
            Work.Asset = Assets->Assets + ID.Value;
            Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work.Offset = Asset->GA.DataOffset;
            Work.Size = Size.Data;
            Work.Destination = Bitmap->Memory;
            Work.FinalState = AssetState_Loaded;
            if(Task)
            {
                load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work);
                *TaskWork = Work;
                Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
            }
            else
            {
                LoadAssetWorkDirectly(&Work);
            }
        }
        else
        {
            Asset->State = AssetState_Unloaded;
        }
    }
}

internal void
LoadSound(game_assets *Assets, sound_id ID)
{
    asset *Asset = Assets->Assets + ID.Value; 
    if(ID.Value &&
       (AtomicCompareExchangeUInt32((uint32_t *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded))
    {
        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
        if(Task)
        {
            asset *Asset = Assets->Assets + ID.Value;
            ga_sound *Info = &Asset->GA.Sound;

            asset_memory_size Size = {};
            Size.Section = Info->SampleCount*sizeof(s16);
            Size.Data = Info->ChannelCount*Size.Section;
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = (asset_memory_header *)AcquireAssetMemory(Assets, Size.Total, ID.Value);
            loaded_sound *Sound = &Asset->Header->Sound;

            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;
            u32 ChannelSize = Size.Section;

            void *Memory = (Asset->Header + 1);
            s16 *SoundAt = (s16 *)Memory;
            for(u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex)
            {
                Sound->Samples[ChannelIndex] = SoundAt;
                SoundAt += ChannelSize;
            }

            load_asset_work *Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Task = Task;
            Work->Asset = Assets->Assets + ID.Value;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->GA.DataOffset;
            Work->Size = Size.Data;
            Work->Destination = Memory;
            Work->FinalState = (AssetState_Loaded);

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else
        {
            Assets->Assets[ID.Value].State = AssetState_Unloaded;
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
GetRandomAssetFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
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
GetFirstAssetFrom(game_assets *Assets, asset_type_id TypeID)
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
    bitmap_id Result = {GetFirstAssetFrom(Assets, TypeID)};
    return(Result);
}

inline bitmap_id
GetRandomBitmapFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    bitmap_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
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
    sound_id Result = {GetFirstAssetFrom(Assets, TypeID)};
    return(Result);
}

inline sound_id
GetRandomSoundFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    sound_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
    return(Result);
}

internal game_assets *
AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
{
    game_assets *Assets = PushStruct(Arena, game_assets);

    Assets->MemorySentinel.Flags = 0;
    Assets->MemorySentinel.Size = 0;
    Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
    Assets->MemorySentinel.Next = &Assets->MemorySentinel;

    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size));

    Assets->TranState = TranState;

    Assets->LoadedAssetSentinel.Next = Assets->LoadedAssetSentinel.Prev = &Assets->LoadedAssetSentinel;

    for(uint32_t TagType = 0; TagType < Tag_Count; ++TagType)
    {
        Assets->TagRange[TagType] = 1000000.0f;
    }
    Assets->TagRange[Tag_FacingDirection] = Tau32;

    Assets->TagCount = 1;
    Assets->AssetCount = 1;

    {
        platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin(PlatformFileType_AssetFile);
        Assets->FileCount = FileGroup.FileCount;
        Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
        for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex;

            File->TagBase = Assets->TagCount;

            ZeroStruct(File->Header);
            File->Handle = Platform.OpenNextFile(&FileGroup);
            Platform.ReadDataFromFile(&File->Handle, 0, sizeof(File->Header), &File->Header);

            u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(ga_asset_type);
            File->AssetTypeArray = (ga_asset_type *)PushSize(Arena, AssetTypeArraySize);
            Platform.ReadDataFromFile(&File->Handle, File->Header.AssetTypes,
                                      AssetTypeArraySize, File->AssetTypeArray);

            if(File->Header.MagicValue != GA_MAGIC_VALUE)
            {
                Platform.FileError(&File->Handle, "GA file has an invalid magic value.");
            }

            if(File->Header.Version > GA_VERSION)
            {
                Platform.FileError(&File->Handle, "GA file is a later version.");
            }

            if(PlatformNoFileErrors(&File->Handle))
            {
                // The first asset and tag slot in every GA is a null (reserved)
                // sow we don't count it as something we will need space for.
                Assets->TagCount += (File->Header.TagCount - 1);
                Assets->AssetCount += (File->Header.AssetCount - 1);
            }
            else
            {
                // TODO: Eventually, have some way of notifying users of fucked up files.
                InvalidCodePath;
            }
        }
        Platform.GetAllFilesOfTypeEnd(&FileGroup);
    }

    // Allocate all metadata space.
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
    Assets->Tags = PushArray(Arena, Assets->TagCount, ga_tag);

    // Reserve one null asset at the beginning.
    ZeroStruct(Assets->Tags[0]);

    // Load tags.
    for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
    {
        asset_file *File = Assets->Files + FileIndex;
        if(PlatformNoFileErrors(&File->Handle))
        {
            u32 TagArraySize = sizeof(ga_tag)*(File->Header.TagCount - 1);
            Platform.ReadDataFromFile(&File->Handle, File->Header.Tags + sizeof(ga_tag),
                                      TagArraySize, Assets->Tags + File->TagBase);
        }
    }

    // Reserve one null asset at the beginning.
    u32 AssetCount = 0;
    ZeroStruct(*(Assets->Assets + AssetCount));
    ++AssetCount;

    // TODO: How would I do this in a way that could scale gracefully to 
    // hundred of asset pack files.
    for(u32 DestTypeID = 0; DestTypeID < Asset_Count; ++DestTypeID)
    {
        asset_type *DestType = Assets->AssetTypes + DestTypeID;
        DestType->FirstAssetIndex = AssetCount;

        for(u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex)
        {
            asset_file *File = Assets->Files + FileIndex;
            if(PlatformNoFileErrors(&File->Handle))
            {
                for(u32 SourceIndex = 0; SourceIndex < File->Header.AssetTypeCount; ++SourceIndex)
                {
                    ga_asset_type *SourceType = File->AssetTypeArray + SourceIndex;

                    if(SourceType->TypeID == DestTypeID)
                    {
                        u32 AssetCountForType = (SourceType->OnePastLastAssetIndex - SourceType->FirstAssetIndex);

                        temporary_memory TempMem = BeginTemporaryMemory(&TranState->TranArena);
                        ga_asset *GAAssetArray = PushArray(&TranState->TranArena, AssetCountForType, ga_asset);
                        Platform.ReadDataFromFile(&File->Handle,
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
                            if(Asset->GA.FirstTagIndex == 0)
                            {
                                Asset->GA.FirstTagIndex = Asset->GA.OnePastLastTagIndex = 0;
                            }
                            else
                            {
                                Asset->GA.FirstTagIndex += (File->TagBase - 1);
                                Asset->GA.OnePastLastTagIndex += (File->TagBase - 1);
                            }
                        }

                        EndTemporaryMemory(TempMem);
                    }
                }
            }
        }

        DestType->OnePastLastAssetIndex = AssetCount;
    }

    Assert(AssetCount == Assets->AssetCount);

    return(Assets);
}

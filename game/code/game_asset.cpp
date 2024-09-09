

enum finalise_asset_operation
{
    FinaliseAsset_None,
    FinaliseAsset_Font,
    FinaliseAsset_Bitmap,
};
struct load_asset_work
{
    task_with_memory *Task;
    asset *Asset;

    platform_file_handle *Handle;
    u64 Offset;
    u64 Size;
    void *Destination;

    finalise_asset_operation FinaliseOperation;
    u32 FinalState;
};
internal void
LoadAssetWorkDirectly(load_asset_work *Work)
{
    TIMED_FUNCTION();

    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
    if(PlatformNoFileErrors(Work->Handle))
    {
        switch(Work->FinaliseOperation)
        {
            case FinaliseAsset_None:
            {
                // Nothing to do.
            } break;

            case FinaliseAsset_Font:
            {
                loaded_font *Font = &Work->Asset->Header->Font;
                ga_font *GA = &Work->Asset->GA.Font;
                for(u32 GlyphIndex = 1; GlyphIndex < GA->GlyphCount; ++GlyphIndex)
                {
                    ga_font_glyph *Glyph = Font->Glyphs + GlyphIndex;

                    Assert(Glyph->UnicodeCodepoint < GA->OnePastHighestCodepoint);
                    Assert((u32)(u16)GlyphIndex == GlyphIndex);
                    Font->UnicodeMap[Glyph->UnicodeCodepoint] = (u16)GlyphIndex;
                }
            } break;

            case FinaliseAsset_Bitmap:
            {
                loaded_bitmap *Bitmap = &Work->Asset->Header->Bitmap;
#if 0
                Bitmap->TextureHandle = Platform.AllocateTexture(Bitmap->Width, Bitmap->Height, Bitmap->Memory);
#endif
            } break;
        }
    }

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

inline asset_file *
GetFile(game_assets *Assets, u32 FileIndex)
{
    Assert(FileIndex < Assets->FileCount);
    asset_file *Result = Assets->Files + FileIndex;

    return(Result);
}

inline platform_file_handle *
GetFileHandleFor(game_assets *Assets, u32 FileIndex)
{
    platform_file_handle *Result = &GetFile(Assets, FileIndex)->Handle;

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

internal b32
GenerationHasCompleted(game_assets *Assets, u32 CheckID)
{
    b32 Result = true;

    for(u32 Index = 0; Index < Assets->InFlightGenerationCount; ++Index)
    {
        if(Assets->InFlightGenerations[Index] == CheckID)
        {
            Result = false;
            break;
        }
    }

    return(Result);
}

internal asset_memory_header *
AcquireAssetMemory(game_assets *Assets, u32 Size, u32 Index, asset_header_type AssetType) // Changed from the name 'AssetIndex' to comply with warning 4457.
{
    TIMED_FUNCTION();

    asset_memory_header *Result = 0;

    BeginAssetLock(Assets);

    asset_memory_block *Block = FindBlockForSize(Assets, Size);
    for(;;)
    {
        if(Block && (Size <= Block->Size))
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
                if((Asset->State >= AssetState_Loaded) &&
                   (GenerationHasCompleted(Assets, Asset->Header->GenerationID)))
                {
                    u32 AssetIndex = Header->AssetIndex;
                    asset *Asset = Assets->Assets + AssetIndex;

                    Assert(Asset->State == AssetState_Loaded);

                    RemoveAssetHeaderFromList(Header);

                    if(Asset->Header->AssetType == AssetType_Bitmap)
                    {
#if 0
                        Platform.DeallocateTexture(Asset->Header->Bitmap.TextureHandle);
#endif
                    }

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
        Result->AssetType = AssetType;
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
    TIMED_FUNCTION();

    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value)
    {
        if(AtomicCompareExchangeUInt32((uint32_t *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
           AssetState_Unloaded)
        {
            task_with_memory *Task = 0;

            if(!Immediate)
            {
                Task = BeginTaskWithMemory(Assets->TranState, false);
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

                Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value, AssetType_Bitmap);

                loaded_bitmap *Bitmap = &Asset->Header->Bitmap;
                Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
                Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
                Bitmap->Width = Info->Dim[0];
                Bitmap->Height = Info->Dim[1];
                Bitmap->Pitch = Size.Section;
                Bitmap->TextureHandle = 0;
                Bitmap->Memory = (Asset->Header + 1);

                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->GA.DataOffset;
                Work.Size = Size.Data;
                Work.Destination = Bitmap->Memory;
                Work.FinaliseOperation = FinaliseAsset_Bitmap;
                Work.FinalState = AssetState_Loaded;
                if(Task)
                {
                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work, NoClear());
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
        else if(Immediate)
        {
            // TODO: Do I wasnt to have a more coherent story for what happens
            // when two force-load people hit the storage at the same time?
            asset_state volatile *State = (asset_state volatile *)&Asset->State;
            while(*State == AssetState_Queued) {}
        }
    }
}

internal void
LoadSound(game_assets *Assets, sound_id ID)
{
    TIMED_FUNCTION();

    asset *Asset = Assets->Assets + ID.Value; 
    if(ID.Value &&
       (AtomicCompareExchangeUInt32((u32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
        AssetState_Unloaded))
    {
        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState, false);
        if(Task)
        {
            asset *Asset = Assets->Assets + ID.Value;
            ga_sound *Info = &Asset->GA.Sound;

            asset_memory_size Size = {};
            Size.Section = Info->SampleCount*sizeof(s16);
            Size.Data = Info->ChannelCount*Size.Section;
            Size.Total = Size.Data + sizeof(asset_memory_header);

            Asset->Header = (asset_memory_header *)AcquireAssetMemory(Assets, Size.Total, ID.Value, AssetType_Sound);
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
            Work->FinaliseOperation = FinaliseAsset_None;
            Work->FinalState = (AssetState_Loaded);

            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else
        {
            Assets->Assets[ID.Value].State = AssetState_Unloaded;
        }
    }
}

internal void
LoadFont(game_assets *Assets, font_id ID, b32 Immediate)
{
    TIMED_FUNCTION();

    // TODO: Merge all this boilerplate because it's the same between
    // LoadBitmap, LoadSound, and LoadFont.
    asset *Asset = Assets->Assets + ID.Value;
    if(ID.Value)
    {
        if(AtomicCompareExchangeUInt32((u32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
           AssetState_Unloaded)
        {
            task_with_memory *Task = 0;

            if(!Immediate)
            {
                Task = BeginTaskWithMemory(Assets->TranState, false);
            }

            if(Immediate || Task)
            {
                asset *Asset = Assets->Assets + ID.Value;
                ga_font *Info = &Asset->GA.Font;

                u32 HorizontalAdvanceSize = sizeof(r32)*Info->GlyphCount*Info->GlyphCount;
                u32 GlyphsSize = Info->GlyphCount*sizeof(ga_font_glyph);
                u32 UnicodeMapSize = sizeof(u16)*Info->OnePastHighestCodepoint;
                u32 SizeData = GlyphsSize + HorizontalAdvanceSize;
                u32 SizeTotal = SizeData + sizeof(asset_memory_header) + UnicodeMapSize;

                Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value, AssetType_Font);

                loaded_font *Font = &Asset->Header->Font;
                Font->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->FontBitmapIDOffset;
                Font->Glyphs = (ga_font_glyph *)(Asset->Header + 1);
                Font->HorizontalAdvance = (r32 *)((u8 *)Font->Glyphs + GlyphsSize);
                Font->UnicodeMap = (u16 *)((u8 *)Font->HorizontalAdvance + HorizontalAdvanceSize);

                ZeroSize(UnicodeMapSize, Font->UnicodeMap);

                load_asset_work Work;
                Work.Task = Task;
                Work.Asset = Assets->Assets + ID.Value;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Offset = Asset->GA.DataOffset;
                Work.Size = SizeData;
                Work.Destination = Font->Glyphs;
                Work.FinaliseOperation = FinaliseAsset_Font;
                Work.FinalState = AssetState_Loaded;
                if(Task)
                {
                    load_asset_work *TaskWork = PushStruct(&Task->Arena, load_asset_work, NoClear());
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
        else if(Immediate)
        {
            // TODO: Do I wasnt to have a more coherent story for what happens
            // when two force-load people hit the storage at the same time?
            asset_state volatile *State = (asset_state volatile *)&Asset->State;
            while(*State == AssetState_Queued) {}
        }
    }
}

internal u32
GetBestMatchAssetFrom(game_assets *Assets, asset_type_id TypeID,
                      asset_vector *MatchVector, asset_vector *WeightVector)
{
    TIMED_FUNCTION();

    u32 Result = 0;

    real32 BestDiff = Real32Maximum;
    asset_type *Type = Assets->AssetTypes + TypeID;
    for(u32 AssetIndex = Type->FirstAssetIndex;
        AssetIndex < Type->OnePastLastAssetIndex;
        ++AssetIndex)
    {
        asset *Asset = Assets->Assets + AssetIndex;

        real32 TotalWeightedDiff = 0.0f;
        for(u32 TagIndex = Asset->GA.FirstTagIndex; TagIndex < Asset->GA.OnePastLastTagIndex; ++TagIndex)
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

internal u32
GetRandomAssetFrom(game_assets *Assets, asset_type_id TypeID, random_series *Series)
{
    TIMED_FUNCTION();

    u32 Result = 0;

    asset_type *Type = Assets->AssetTypes + TypeID;
    if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
        u32 Count = (Type->OnePastLastAssetIndex - Type->FirstAssetIndex);
        u32 Choice = RandomChoice(Series, Count);
        Result = Type->FirstAssetIndex + Choice;
    }

    return(Result);
}

internal u32
GetFirstAssetFrom(game_assets *Assets, asset_type_id TypeID)
{
    TIMED_FUNCTION();

    u32 Result = 0;

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

internal font_id
GetBestMatchFontFrom(game_assets *Assets, asset_type_id TypeID, asset_vector *MatchVector, asset_vector *WeightVector)
{
    font_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
    return(Result);
}

internal game_assets *
AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
{
    TIMED_FUNCTION();

    game_assets *Assets = PushStruct(Arena, game_assets);

    Assets->NextGenerationID = 0;
    Assets->InFlightGenerationCount = 0;

    Assets->MemorySentinel.Flags = 0;
    Assets->MemorySentinel.Size = 0;
    Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
    Assets->MemorySentinel.Next = &Assets->MemorySentinel;

    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size, NoClear()));

    Assets->TranState = TranState;

    Assets->LoadedAssetSentinel.Next = Assets->LoadedAssetSentinel.Prev = &Assets->LoadedAssetSentinel;

    for(u32 TagType = 0; TagType < Tag_Count; ++TagType)
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

            File->FontBitmapIDOffset = 0;
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
            // Skip the first tag, since it's null.
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
                        if(SourceType->TypeID == Asset_FontGlyph)
                        {
                            File->FontBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
                        }

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

inline u32
GetGlyphFromCodepoint(ga_font *Info, loaded_font *Font, u32 Codepoint)
{
    u32 Result = 0;
    if(Codepoint < Info->OnePastHighestCodepoint)
    {
        Result = Font->UnicodeMap[Codepoint];
        Assert(Result < Info->GlyphCount);
    }

    return(Result);
}

internal r32
GetHorizontalAdvanceForPair(ga_font *Info, loaded_font *Font, u32 DesiredPrevCodepoint, u32 DesiredCodepoint)
{
    u32 PrevGlyph = GetGlyphFromCodepoint(Info, Font, DesiredPrevCodepoint);
    u32 Glyph = GetGlyphFromCodepoint(Info, Font, DesiredCodepoint);

    r32 Result = Font->HorizontalAdvance[PrevGlyph*Info->GlyphCount + Glyph];

    return(Result);
}

internal bitmap_id
GetBitmapForGlyph(game_assets *Assets, ga_font *Info, loaded_font *Font, u32 DesiredCodepoint)
{
    u32 Glyph = GetGlyphFromCodepoint(Info, Font, DesiredCodepoint);
    bitmap_id Result = Font->Glyphs[Glyph].BitmapID;
    Result.Value += Font->BitmapIDOffset;

    return(Result);
}

internal r32
GetLineAdvanceFor(ga_font *Info)
{
    r32 Result = Info->AscenderHeight + Info->DescenderHeight + Info->ExternalLeading;

    return(Result);
}

internal r32
GetStartingBaselineY(ga_font *Info)
{
    r32 Result = Info->AscenderHeight;

    return(Result);
}


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
#pragma pack(pop)

inline v2
TopDownAlign(loaded_bitmap *Bitmap, v2 Align)
{
    Align.y = (real32)(Bitmap->Height - 1) - Align.y;

    Align.x = SafeRatio0(Align.x, (real32)Bitmap->Width);
    Align.y = SafeRatio0(Align.y, (real32)Bitmap->Height);

    return(Align);
}

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename,
             int32_t AlignX, int32_t TopDownAlignY)
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.AlignPercentage = TopDownAlign(&Result, V2i(AlignX, TopDownAlignY));
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

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
    loaded_bitmap Result = DEBUGLoadBMP(Thread, ReadEntireFile, Filename, 0, 0);
    Result.AlignPercentage = V2(0.5f, 0.5f);
    return(Result);
}

struct load_bitmap_work
{
    game_assets *Assets;
    char *Filename;
    bitmap_id ID;
    task_with_memory *Task;
    loaded_bitmap *Bitmap;

    bool32 HasAlignment;
    int32_t AlignX;
    int32_t TopDownAlignY;

    asset_state FinalState;
};
internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
{
    load_bitmap_work *Work = (load_bitmap_work *)Data;
    
    // TODO: Get rid of this thread thingamabob when I load through
    // a queue instead of the debug call.
    thread_context *Thread = 0;

    if(Work->HasAlignment)
    {
        *Work->Bitmap = DEBUGLoadBMP(Thread, Work->Assets->ReadEntireFile, Work->Filename,
                                     Work->AlignX, Work->TopDownAlignY);
    }
    else
    {
        *Work->Bitmap = DEBUGLoadBMP(Thread, Work->Assets->ReadEntireFile, Work->Filename);
    }

    CompletePreviousWritesBeforeFutureWrites;

    Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
    Work->Assets->Bitmaps[Work->ID.Value].State = Work->FinalState;

    EndTaskWithMemory(Work->Task);
}

internal void
LoadBitmap(game_assets *Assets, bitmap_id ID)
{
    if(AtomicCompareExchangeUInt32((uint32_t *)&Assets->Bitmaps[ID.Value].State, AssetState_Unloaded, AssetState_Queued) ==
       AssetState_Unloaded)
    {
        task_with_memory *Task = BeginTaskWithMemory(Assets->TranState);
        if(Task)
        {
            debug_platform_read_entire_file *ReadEntireFile = Assets->ReadEntireFile;
            
            load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);

            Work->Assets = Assets;
            Work->Filename = "";
            Work->ID = ID;
            Work->Task = Task;
            Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
            Work->HasAlignment = false;
            Work->FinalState = AssetState_Loaded;

            thread_context *Thread = 0;
            switch(ID.Value)
            {
                case Asset_Backdrop:
                {
                    Work->Filename = "test/test_background.bmp";
                } break;

                case Asset_Shadow:
                {
                    Work->Filename = "test/test_hero_shadow.bmp";
                    Work->HasAlignment = true;
                    Work->AlignX = 72;
                    Work->TopDownAlignY = 182;
                } break;

                case Asset_Tree:
                {
                    Work->Filename = "test2/tree00.bmp";
                    Work->HasAlignment = true;
                    Work->AlignX = 72;
                    Work->TopDownAlignY = 80;
                } break;

                case Asset_Stairwell:
                {
                    Work->Filename = "test2/rock02.bmp";
                } break;

                case Asset_Sword:
                {
                    Work->Filename = "test2/rock03.bmp";
                    Work->HasAlignment = 29;
                    Work->AlignX = 72;
                    Work->TopDownAlignY = 10;
                } break;
            }

            PlatformAddEntry(Assets->TranState->LowPriorityQueue, LoadBitmapWork, Work);
        }
    }
}

internal void
LoadSound(game_assets *Assets, uint32_t ID)
{
}

internal bitmap_id
GetFirstBitmapID(game_assets *Assets, asset_type_id TypeID)
{
    // TODO: I should actually look
    bitmap_id Result = {TypeID};

    return(Result);
}

internal game_assets *
AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState,
                   debug_platform_read_entire_file *ReadEntireFile)
{
    game_assets *Assets = PushStruct(Arena, game_assets);
    SubArena(&Assets->Arena, Arena, Size);
    Assets->ReadEntireFile = ReadEntireFile;
    Assets->TranState = TranState;

    return(Assets);
}

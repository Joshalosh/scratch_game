
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

internal void
SetTopDownAlign(hero_bitmaps *Bitmap, v2 Align)
{
    Align = TopDownAlign(&Bitmap->Head, Align);

    Bitmap->Head.AlignPercentage = Align;
    Bitmap->Cape.AlignPercentage = Align;
    Bitmap->Torso.AlignPercentage = Align;
}

internal loaded_bitmap
DEBUGLoadBMP(char *Filename, int32_t AlignX, int32_t TopDownAlignY)
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
DEBUGLoadBMP(char *Filename)
{
    loaded_bitmap Result = DEBUGLoadBMP(Filename, 0, 0);
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
    
    if(Work->HasAlignment)
    {
        *Work->Bitmap = DEBUGLoadBMP(Work->Filename, Work->AlignX, Work->TopDownAlignY);
    }
    else
    {
        *Work->Bitmap = DEBUGLoadBMP(Work->Filename);
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
            load_bitmap_work *Work = PushStruct(&Task->Arena, load_bitmap_work);

            Work->Assets = Assets;
            Work->Filename = "";
            Work->ID = ID;
            Work->Task = Task;
            Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
            Work->HasAlignment = false;
            Work->FinalState = AssetState_Loaded;

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
AllocateGameAssets(memory_arena *Arena, memory_index Size, transient_state *TranState)
{
    game_assets *Assets = PushStruct(Arena, game_assets);
    SubArena(&Assets->Arena, Arena, Size);
    Assets->TranState = TranState;

    Assets->BitmapCount = Asset_Count;
    Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

    Assets->SoundCount = 1;
    Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

    Assets->Grass[0] = DEBUGLoadBMP("test2/grass00.bmp");
    Assets->Grass[1] = DEBUGLoadBMP("test2/grass01.bmp");

    Assets->Tuft[0] = DEBUGLoadBMP("test2/tuft00.bmp");
    Assets->Tuft[1] = DEBUGLoadBMP("test2/tuft01.bmp");
    Assets->Tuft[2] = DEBUGLoadBMP("test2/tuft02.bmp");

    Assets->Stone[0] = DEBUGLoadBMP("test2/ground00.bmp");
    Assets->Stone[1] = DEBUGLoadBMP("test2/ground01.bmp");
    Assets->Stone[2] = DEBUGLoadBMP("test2/ground02.bmp");
    Assets->Stone[3] = DEBUGLoadBMP("test2/ground03.bmp");

    hero_bitmaps *Bitmap;

    Bitmap = Assets->HeroBitmaps;
    Bitmap->Head = DEBUGLoadBMP("test/test_hero_right_head.bmp");
    Bitmap->Cape = DEBUGLoadBMP("test/test_hero_right_cape.bmp");
    Bitmap->Torso = DEBUGLoadBMP("test/test_hero_right_torso.bmp");
    SetTopDownAlign(Bitmap, V2(72, 182));
    ++Bitmap;

    Bitmap->Head = DEBUGLoadBMP("test/test_hero_back_head.bmp");
    Bitmap->Cape = DEBUGLoadBMP("test/test_hero_back_cape.bmp");
    Bitmap->Torso = DEBUGLoadBMP("test/test_hero_back_torso.bmp");
    SetTopDownAlign(Bitmap, V2(72, 182));
    ++Bitmap;

    Bitmap->Head = DEBUGLoadBMP("test/test_hero_left_head.bmp");
    Bitmap->Cape = DEBUGLoadBMP("test/test_hero_left_cape.bmp");
    Bitmap->Torso = DEBUGLoadBMP("test/test_hero_left_torso.bmp");
    SetTopDownAlign(Bitmap, V2(72, 182));
    ++Bitmap;

    Bitmap->Head = DEBUGLoadBMP("test/test_hero_front_head.bmp");
    Bitmap->Cape = DEBUGLoadBMP("test/test_hero_front_cape.bmp");
    Bitmap->Torso = DEBUGLoadBMP("test/test_hero_front_torso.bmp");
    SetTopDownAlign(Bitmap, V2(72, 182));
    ++Bitmap;

    return(Assets);
}

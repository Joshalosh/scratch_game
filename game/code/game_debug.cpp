
// TODO: Stop using stdio
#include <stdio.h>

internal void RestartCollation(debug_state *DebugState, u32 InvalidEventArrayIndex);

inline debug_state *
DEBUGGetState(game_memory *Memory)
{
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    Assert(DebugState->Initialised);

    return(DebugState);
}

inline debug_state *
DEBUGGetState(void)
{
    debug_state *Result = DEBUGGetState(DebugGlobalMemory);

    return(Result);
}

internal void
DEBUGStart(game_assets *Assets, u32 Width, u32 Height)
{
    TIMED_FUNCTION();

    debug_state *DebugState = (debug_state *)DebugGlobalMemory->DebugStorage;
    if(DebugState)
    {
        if(!DebugState->Initialised)
        {
            DebugState->HighPriorityQueue = DebugGlobalMemory->HighPriorityQueue;

            InitialiseArena(&DebugState->DebugArena, DebugGlobalMemory->DebugStorageSize - sizeof(debug_state),
                            DebugState + 1);
            DebugState->RenderGroup = AllocateRenderGroup(Assets, &DebugState->DebugArena, 
                                                          Megabytes(16), false);

            DebugState->Paused = false;
            DebugState->ScopeToRecord = 0;

            DebugState->Initialised = true;

            SubArena(&DebugState->CollateArena, &DebugState->DebugArena, Megabytes(32), 4);
            DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

            RestartCollation(DebugState, 0);
        }

        BeginRender(DebugState->RenderGroup);
        DebugState->DebugFont = PushFont(DebugState->RenderGroup, DebugState->FontID);
        DebugState->DebugFontInfo = GetFontInfo(DebugState->RenderGroup->Assets, DebugState->FontID);

        DebugState->GlobalWidth = (r32)Width;
        DebugState->GlobalHeight = (r32)Height;

        asset_vector MatchVector = {};
        asset_vector WeightVector = {};
        MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
        WeightVector.E[Tag_FontType] = 1.0f;
        DebugState->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);

        DebugState->FontScale = 1.0f;
        Orthographic(DebugState->RenderGroup, Width, Height, 1.0f);
        DebugState->LeftEdge = -0.5f*Width;

        ga_font *Info = GetFontInfo(Assets, DebugState->FontID);
        DebugState->AtY = 0.5f*Height - DebugState->FontScale*GetStartingBaselineY(Info);
    }
}

inline b32
IsHex(char Char)
{
    b32 Result = (((Char >= '0') && (Char <= '9')) ||
                  ((Char >= 'A') && (Char <= 'F')));

    return(Result);
}

inline u32
GetHex(char Char)
{
    u32 Result = 0;

    if((Char >= '0') && (Char <= '9'))
    {
        Result = Char - '0';
    }
    else if((Char >= 'A') && (Char <= 'F'))
    {
        Result = 0xA + (Char - 'A');
    }

    return(Result);
}

internal rectangle2
DEBUGTextOp(debug_state *DebugState, debug_text_op Op, v2 P, char *String)
{
    rectangle2 Result = InvertedInfinityRectangle2();
    if(DebugState && DebugState->DebugFont)
    {
        render_group *RenderGroup = DebugState->RenderGroup;
        loaded_font *Font = DebugState->DebugFont;
        ga_font *Info = DebugState->DebugFontInfo;

        u32 PrevCodepoint = 0;
        r32 CharScale = DebugState->FontScale;
        v4 Color = V4(1, 1, 1, 1);
        r32 AtY = P.y;
        r32 AtX = P.x;
        for(char *At = String; *At;)
        {
            if((At[0] == '\\') &&
               (At[1] == '#') &&
               (At[2] != 0) &&
               (At[3] != 0) &&
               (At[4] != 0))
            {
                r32 CScale = 1.0f / 9.0f;
                Color = V4(Clamp01(CScale*(r32)(At[2] - '0')),
                           Clamp01(CScale*(r32)(At[3] - '0')),
                           Clamp01(CScale*(r32)(At[4] - '0')),
                           1.0f);
                At += 5;
            }
            else if((At[0] == '\\') &&
                    (At[1] == '^') &&
                    (At[2] != 0))
            {
                r32 CScale = 1.0f / 9.0f;
                CharScale = DebugState->FontScale*Clamp01(CScale*(r32)(At[2] - '0'));
                At += 3;
            }
            else
            {
                u32 Codepoint = *At;
                if((At[0] == '\\') &&
                   (IsHex(At[1])) &&
                   (IsHex(At[2])) &&
                   (IsHex(At[3])) &&
                   (IsHex(At[4])))
                {
                    Codepoint = ((GetHex(At[1]) << 12) |
                                 (GetHex(At[2]) << 8) |
                                 (GetHex(At[3]) << 4) |
                                 (GetHex(At[4]) << 0));
                    At += 4;
                }

                r32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodepoint, Codepoint);
                AtX += AdvanceX;

                if(Codepoint != ' ')
                {
                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, Codepoint);
                    ga_bitmap *Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);

                    r32 BitmapScale = CharScale*(r32)Info->Dim[1];
                    v3 BitmapOffset = V3(AtX, AtY, 0);
                    if(Op == DEBUGTextOp_DrawText)
                    {
                        PushBitmap(RenderGroup, BitmapID, BitmapScale, BitmapOffset, Color);
                    }
                    else 
                    {
                        Assert(Op == DEBUGTextOp_SizeText);

                        loaded_bitmap *Bitmap = GetBitmap(RenderGroup->Assets, BitmapID, RenderGroup->GenerationID);
                        if(Bitmap)
                        {
                            used_bitmap_dim Dim = GetBitmapDim(RenderGroup, Bitmap, BitmapScale, BitmapOffset);
                            rectangle2 GlyphDim = RectMinDim(V2(Dim.P), Dim.Size);
                            Result = Union(Result, GlyphDim);
                        }
                    }
                }

                PrevCodepoint = Codepoint;

                ++At;
            }
        }
    }

    return(Result);
}

internal void
DEBUGTextOutAt(v2 P, char *String)
{
    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        render_group *RenderGroup = DebugState->RenderGroup;
        DEBUGTextOp(DebugState, DEBUGTextOp_DrawText, P, String);
    }
}

internal rectangle2
DEBUGGetTextSize(debug_state *DebugState, char *String)
{
    rectangle2 Result = DEBUGTextOp(DebugState, DEBUGTextOp_SizeText, V2(0, 0), String);

    return(Result);
}

internal void
DEBUGTextLine(char *String)
{
    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        render_group *RenderGroup = DebugState->RenderGroup;

        loaded_font *Font = PushFont(RenderGroup, DebugState->FontID);
        if(Font)
        {
            ga_font *Info = GetFontInfo(RenderGroup->Assets, DebugState->FontID);

            DEBUGTextOutAt(V2(DebugState->LeftEdge, DebugState->AtY), String);

            DebugState->AtY -= GetLineAdvanceFor(Info)*DebugState->FontScale;
        }
    }
}

struct debug_statistic
{
    r64 Min;
    r64 Max;
    r64 Avg;
    u32 Count;
};
inline void
BeginDebugStatistic(debug_statistic *Stat)
{
    Stat->Min = Real32Maximum;
    Stat->Max = -Real32Maximum;
    Stat->Avg = 0.0f;
    Stat->Count = 0;
}

inline void
AccumulateDebugStatistic(debug_statistic *Stat, r64 Value)
{
    ++Stat->Count;

    if(Stat->Min > Value)
    {
        Stat->Min = Value;
    }

    if(Stat->Max < Value)
    {
        Stat->Max = Value;
    }

    Stat->Avg += Value;
}

inline void
EndDebugStatistic(debug_statistic *Stat)
{
    if(Stat->Count)
    {
        Stat->Avg /= (r64)Stat->Count;
    }
    else
    {
        Stat->Min = 0.0f;
        Stat->Max = 0.0f;
    }
}

internal void
DrawDebugMainMenu(debug_state *DebugState, render_group *RenderGroup)
{
    // SHOWCASE: How to initialise an array of strings.
    char *MenuItems[] =
    {
        "Toggle Profile Graph",
        "Toggle Framerate Counter",
        "Mark Loop Point",
        "Toggle Entity Bounds",
        "Toggle World Chunk Bounds",
    };

    r32 MenuRadius = 200.0f;
    r32 AngleStep = Tau32 / (r32)ArrayCount(MenuItems);
    for(u32 MenuItemIndex = 0; MenuItemIndex < ArrayCount(MenuItems); ++MenuItemIndex)
    {
        char *Text = MenuItems[MenuItemIndex];

        r32 Angle = (r32)MenuItemIndex*AngleStep;
        v2 TextP = MenuRadius*Arm2(Angle);

        rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);
        PushRect(RenderGroup, Offset(TextBounds, TextP), 0.0f, V4(0, 0, 0.5f, 1));

        PushRect(RenderGroup, RectCenterHalfDim(TextP, V2(1.5f, 1.5f)), 0.0f, V4(1, 1, 1, 1));

        DEBUGTextOutAt(TextP, Text);
    }
}

internal void
DEBUGEnd(game_input *Input, loaded_bitmap *DrawBuffer)
{
    TIMED_FUNCTION();

    debug_state *DebugState = DEBUGGetState();
    if(DebugState)
    {
        render_group *RenderGroup = DebugState->RenderGroup;

        debug_record *HotRecord = 0;

        v2 MouseP = V2(Input->MouseX, Input->MouseY);
        if(WasPressed(Input->MouseButtons[PlatformMouseButton_Right]))
        {
            DebugState->Paused = !DebugState->Paused;
        }

        DrawDebugMainMenu(DebugState, RenderGroup);

        // TODO: Layout / cached font info / etc. for real debug display
        loaded_font *Font = PushFont(RenderGroup, DebugState->FontID);
        if(Font)
        {
            ga_font *Info = GetFontInfo(RenderGroup->Assets, DebugState->FontID);

#if 0
            for(u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex)
            {
                debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;

                debug_statistic HitCount, CycleCount, CycleOverHit;
                BeginDebugStatistic(&HitCount);
                BeginDebugStatistic(&CycleCount);
                BeginDebugStatistic(&CycleOverHit);
                for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
                {
                    AccumulateDebugStatistic(&HitCount, Counter->Snapshots[SnapshotIndex].HitCount);
                    AccumulateDebugStatistic(&CycleCount, (u32)Counter->Snapshots[SnapshotIndex].CycleCount);

                    r64 COH = 0.0f;
                    if(Counter->Snapshots[SnapshotIndex].HitCount)
                    {
                        COH = ((r64)Counter->Snapshots[SnapshotIndex].CycleCount /
                               (r64)Counter->Snapshots[SnapshotIndex].HitCount);
                    }
                    AccumulateDebugStatistic(&CycleOverHit, COH);
                }
                EndDebugStatistic(&HitCount);
                EndDebugStatistic(&CycleCount);
                EndDebugStatistic(&CycleOverHit);

                if(Counter->BlockName)
                {
                    if(CycleCount.Max > 0.0f)
                    {
                        r32 BarWidth = 4.0f;
                        r32 ChartLeft = 0.0f;
                        r32 ChartMinY = AtY;
                        r32 ChartHeight = Info->AscenderHeight*FontScale;
                        r32 Scale = 1.0f / (r32)CycleCount.Max;
                        for(u32 SnapshotIndex = 0; SnapshotIndex < DEBUG_SNAPSHOT_COUNT; ++SnapshotIndex)
                        {
                            r32 ThisProportion = Scale*(r32)Counter->Snapshots[SnapshotIndex].CycleCount;
                            r32 ThisHeight = ChartHeight*ThisProportion;
                            PushRect(RenderGroup, V3(ChartLeft + BarWidth*(r32)SnapshotIndex + 0.5f*BarWidth, ChartMinY + 0.5f*ThisHeight, 0.0f), V2(BarWidth, ThisHeight), V4(ThisProportion, 1, 0.0f, 1));
                        }
                    }

#if 1
                    char TextBuffer[256];
                    _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                "%32s(%4d): %20ucy %16uh %20ucy/h",
                                Counter->BlockName,
                                Counter->LineNumber,
                                (u32)CycleCount.Avg,
                                (u32)HitCount.Avg,
                                (u32)CycleOverHit.Avg);
                    DEBUGTextLine(TextBuffer);
#endif
                }
            }
#endif

            if(DebugState->FrameCount)
            {
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "Last frame time: %.02fms",
                            DebugState->Frames[DebugState->FrameCount - 1].WallSecondsElapsed * 1000.0f);
                DEBUGTextLine(TextBuffer);
            }

            if(DebugState->ProfileOn)
            {
                Orthographic(DebugState->RenderGroup,
                             (s32)DebugState->GlobalWidth, 
                             (s32)DebugState->GlobalHeight, 1.0f);

                DebugState->ProfileRect = RectMinMax(V2(50.0f, 50.0f), V2(200.0f, 200.0f));
                PushRect(DebugState->RenderGroup, DebugState->ProfileRect, 0.0f, V4(0, 0, 0, 0.25f));

                r32 BarSpacing = 4.0f;
                r32 LaneHeight = 0.0f;
                u32 LaneCount = DebugState->FrameBarLaneCount;

                u32 MaxFrame = DebugState->FrameCount;
                if(MaxFrame > 10)
                {
                    MaxFrame = 10;
                }

                if((LaneCount > 0) && (MaxFrame > 0))
                {
                    r32 PixelsPerFramePlusSpacing = GetDim(DebugState->ProfileRect).y / (r32)MaxFrame;
                    r32 PixelsPerFrame = PixelsPerFramePlusSpacing - BarSpacing;
                    LaneHeight = PixelsPerFrame / (r32)LaneCount;
                }

                r32 BarHeight = LaneHeight*LaneCount;
                r32 BarsPlusSpacing = BarHeight + BarSpacing;
                r32 ChartLeft = DebugState->ProfileRect.Min.x;
                r32 ChartHeight = BarsPlusSpacing*(r32)MaxFrame;
                r32 ChartWidth = GetDim(DebugState->ProfileRect).x;
                r32 ChartTop = DebugState->ProfileRect.Max.y;
                r32 Scale = ChartWidth*DebugState->FrameBarScale;

                v3 Colors[] =
                {
                    {1, 0, 0},
                    {0, 1, 0},
                    {0, 0, 1},
                    {1, 1, 0},
                    {0, 1, 1},
                    {1, 0, 1},
                    {1, 0.5f, 0},
                    {1, 0, 0.5f},
                    {0.5f, 1, 0},
                    {0, 1, 0.5f},
                    {0.5f, 0, 1},
                    {0, 0.5f, 1},
                };

                for(u32 FrameIndex = 0; FrameIndex < MaxFrame; ++FrameIndex)
                {
                    debug_frame *Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);
                    r32 StackX = ChartLeft;
                    r32 StackY = ChartTop - BarsPlusSpacing*(r32)FrameIndex;
                    for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
                    {
                        debug_frame_region *Region = Frame->Regions + RegionIndex;

    //                    v3 Color = Colors[RegionIndex%ArrayCount(Colors)];
                        v3 Color = Colors[Region->ColorIndex%ArrayCount(Colors)];
                        r32 ThisMinX = StackX + Scale*Region->MinT;
                        r32 ThisMaxX = StackX + Scale*Region->MaxT;

                        rectangle2 RegionRect = RectMinMax(V2(ThisMinX, StackY - LaneHeight*(Region->LaneIndex + 1)),
                                                           V2(ThisMaxX, StackY - LaneHeight*Region->LaneIndex));

                        PushRect(RenderGroup, RegionRect, 0.0f, V4(Color, 1));

                        if(IsInRectangle(RegionRect, MouseP))
                        {
                            debug_record *Record = Region->Record;
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "%s: %10ucy [%s(%d)]",
                                        Record->BlockName,
                                        (u32)Region->CycleCount,
                                        Record->Filename,
                                        Record->LineNumber);
                            DEBUGTextOutAt(MouseP + V2(0.0f, 10.0f), TextBuffer);

                            HotRecord = Record;
                        }
                    }
                }
#if 0
                PushRect(RenderGroup, V3(ChartLeft + 0.5f*ChartWidth, ChartMinY + ChartHeight, 0.0f),
                         V2(ChartWidth, 1.0f), V4(1, 1, 1, 1));
#endif
            }
        }

        if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
        {
            if(HotRecord)
            {
                DebugState->ScopeToRecord = HotRecord;
            }
            else
            {
                DebugState->ScopeToRecord = 0;
            }
            RefreshCollation(DebugState);
        }

        TiledRenderGroupToOutput(DebugState->HighPriorityQueue, DebugState->RenderGroup, DrawBuffer);
        EndRender(DebugState->RenderGroup);
    }
}

#define DebugRecords_Main_Count __COUNTER__
extern u32 DebugRecords_Optimised_Count;

global_variable debug_table GlobalDebugTable_;
debug_table *GlobalDebugTable = &GlobalDebugTable_;

inline u32
GetLaneFromThreadIndex(debug_state *DebugState, u32 ThreadIndex)
{
    u32 Result = 0;

    // TODO: Implement thread ID lookup.

    return(Result);
}

internal debug_thread *
GetDebugThread(debug_state *DebugState, u32 ThreadID)
{
    debug_thread *Result = 0;
    for(debug_thread *Thread = DebugState->FirstThread; Thread; Thread = Thread->Next)
    {
        if(Thread->ID == ThreadID)
        {
            Result = Thread;
            break;
        }
    }

    if(!Result)
    {
        Result = PushStruct(&DebugState->CollateArena, debug_thread);
        Result->ID = ThreadID;
        Result->LaneIndex = DebugState->FrameBarLaneCount++;
        Result->FirstOpenBlock = 0;
        Result->Next = DebugState->FirstThread;
        DebugState->FirstThread = Result;
    }

    return(Result);
}

debug_frame_region *
AddRegion(debug_state *DebugState, debug_frame *CurrentFrame)
{
    Assert(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME);
    debug_frame_region *Result = CurrentFrame->Regions + CurrentFrame->RegionCount++;

    return(Result);
}

internal void
RestartCollation(debug_state *DebugState, u32 InvalidEventArrayIndex)
{
    EndTemporaryMemory(DebugState->CollateTemp);
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

    DebugState->FirstThread = 0;
    DebugState->FirstFreeBlock = 0;

    DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
    DebugState->FrameBarLaneCount = 0;
    DebugState->FrameCount = 0;
    DebugState->FrameBarScale = 1.0f / 60000000.0f;

    DebugState->CollationArrayIndex = InvalidEventArrayIndex + 1;
    DebugState->CollationFrame = 0;
}

inline debug_record *
GetRecordFrom(open_debug_block *Block)
{
    debug_record *Result = Block ? Block->Source : 0;

    return(Result);
}

internal void
CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex)
{
    for(; ; ++DebugState->CollationArrayIndex)
    {
        if(DebugState->CollationArrayIndex == MAX_DEBUG_EVENT_ARRAY_COUNT)
        {
            DebugState->CollationArrayIndex = 0;
        }

        u32 EventArrayIndex = DebugState->CollationArrayIndex;
        if(EventArrayIndex == InvalidEventArrayIndex)
        {
            break;
        }

        for(u32 EventIndex = 0; EventIndex < GlobalDebugTable->EventCount[EventArrayIndex]; ++EventIndex)
        {
            debug_event *Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;
            debug_record *Source = (GlobalDebugTable->Records[Event->TranslationUnit] +
                                    Event->DebugRecordIndex);

            if(Event->Type == DebugEvent_FrameMarker)
            {
                if(DebugState->CollationFrame)
                {
                    DebugState->CollationFrame->EndClock = Event->Clock;
                    DebugState->CollationFrame->WallSecondsElapsed = Event->SecondsElapsed;
                    ++DebugState->FrameCount;

                    r32 ClockRange = (r32)(DebugState->CollationFrame->EndClock - DebugState->CollationFrame->BeginClock);
#if 0
                    if(ClockRange > 0.0f)
                    {
                        r32 FrameBarScale = 1.0f / ClockRange;
                        if(DebugState->FrameBarScale > FrameBarScale)
                        {
                            DebugState->FrameBarScale = FrameBarScale;
                        }
                    }
#endif
                }

                DebugState->CollationFrame = DebugState->Frames + DebugState->FrameCount;
                DebugState->CollationFrame->BeginClock = Event->Clock;
                DebugState->CollationFrame->EndClock = 0;
                DebugState->CollationFrame->RegionCount = 0;
                DebugState->CollationFrame->Regions = PushArray(&DebugState->CollateArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
                DebugState->CollationFrame->WallSecondsElapsed = 0.0f;
            }
            else if(DebugState->CollationFrame)
            {
                u32 FrameIndex = DebugState->FrameCount - 1;
                debug_thread *Thread = GetDebugThread(DebugState, Event->TC.ThreadID);
                u64 RelativeClock = Event->Clock - DebugState->CollationFrame->BeginClock;

                if(StringsAreEqual(Source->BlockName, "DrawRectangle"))
                {
                    int x = 5;
                }

                if(Event->Type == DebugEvent_BeginBlock)
                {
                    open_debug_block *DebugBlock = DebugState->FirstFreeBlock;
                    if(DebugBlock)
                    {
                        DebugState->FirstFreeBlock = DebugBlock->NextFree;
                    }
                    else
                    {
                        DebugBlock = PushStruct(&DebugState->CollateArena, open_debug_block);
                    }

                    DebugBlock->StartingFrameIndex = FrameIndex;
                    DebugBlock->OpeningEvent = Event;
                    DebugBlock->Parent = Thread->FirstOpenBlock;
                    DebugBlock->Source = Source;
                    Thread->FirstOpenBlock = DebugBlock;
                    DebugBlock->NextFree = 0;
                }
                else if(Event->Type == DebugEvent_EndBlock)
                {
                    if(Thread->FirstOpenBlock)
                    {
                        open_debug_block *MatchingBlock = Thread->FirstOpenBlock;
                        debug_event *OpeningEvent = MatchingBlock->OpeningEvent;
                        if((OpeningEvent->TC.ThreadID == Event->TC.ThreadID) &&
                           (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
                           (OpeningEvent->TranslationUnit == Event->TranslationUnit))
                        {
                            if(MatchingBlock->StartingFrameIndex == FrameIndex)
                            {
                                if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ScopeToRecord)
                                {
                                    r32 MinT = (r32)(OpeningEvent->Clock - DebugState->CollationFrame->BeginClock);
                                    r32 MaxT = (r32)(Event->Clock - DebugState->CollationFrame->BeginClock);
                                    r32 ThresholdT = 0.01f;
                                    if((MaxT - MinT) > ThresholdT)
                                    {
                                        debug_frame_region *Region = AddRegion(DebugState, DebugState->CollationFrame);
                                        Region->Record = Source;
                                        Region->CycleCount = (Event->Clock - OpeningEvent->Clock);
                                        Region->LaneIndex = (u16)Thread->LaneIndex;
                                        Region->MinT = MinT;
                                        Region->MaxT = MaxT;
                                        Region->ColorIndex = (u16)OpeningEvent->DebugRecordIndex;
                                    }
                                }
                            }
                            else
                            {
                                // TODO: Record all frames in between and begin/end spans.
                            }

                            Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
                            DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
                            Thread->FirstOpenBlock = MatchingBlock->Parent;
                        }
                        else
                        {
                            // TODO: Record span that goes to the beginning of the frame series.
                        }
                    }
                }
                else
                {
                    Assert(!"Invalid event type");
                }
            }
        }
    }
}

internal void
RefreshCollation(debug_state *DebugState)
{
    RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
    CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
}

extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
    GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;
    GlobalDebugTable->RecordCount[1] = DebugRecords_Optimised_Count;

    ++GlobalDebugTable->CurrentEventArrayIndex;
    if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
    {
        GlobalDebugTable->CurrentEventArrayIndex = 0;
    }

    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex,
                                                  (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);

    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
    GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;

    debug_state *DebugState = DEBUGGetState(Memory);
    if(DebugState)
    {
        if(!DebugState->Paused)
        {
            if(DebugState->FrameCount >= MAX_DEBUG_EVENT_ARRAY_COUNT*4)
            {
                RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
            }
            CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
        }
    }

    return(GlobalDebugTable);
}

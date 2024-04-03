
struct sort_entry
{
    r32 SortKey;
    u32 Index;
};

inline u32
SortKeyToU32(r32 SortKey)
{
    // NOTE: I need to turn our 32-bit floating point value 
    // into some strictly ascending 32-bit unsigned integer value
    u32 Result = *(u32 *)&SortKey;
    if(Result & 0x80000000)
    {
        Result = ~Result;
    }
    else
    {
        Result |= 0x80000000;
    }
    return(Result);
}

inline void
Swap(sort_entry *A, sort_entry *B)
{
    sort_entry Temp = *B;
    *B = *A;
    *A = Temp;
}

internal void
MergeSort(u32 Count, sort_entry *First, sort_entry *Temp)
{
    if(Count == 1)
    {
        // NOTE: Nothing to do
    }
    else if(Count == 2)
    {
        sort_entry *EntryA = First;
        sort_entry *EntryB = First + 1;
        if(EntryA->SortKey > EntryB->SortKey)
        {
            Swap(EntryA, EntryB);
        }
    }
    else
    {
        u32 Half0 = Count / 2;
        u32 Half1 = Count - Half0;

        Assert(Half0 >= 1);
        Assert(Half1 >= 1);

        sort_entry *InHalf0 = First;
        sort_entry *InHalf1 = First + Half0;
        sort_entry *End = First + Count;

        MergeSort(Half0, InHalf0, Temp);
        MergeSort(Half1, InHalf1, Temp);

        sort_entry *ReadHalf0 = InHalf0;
        sort_entry *ReadHalf1 = InHalf1;

        sort_entry *Out = Temp;
        for(u32 Index = 0; Index < Count; ++Index)
        {
            if(ReadHalf0 == InHalf1)
            {
                *Out++ = *ReadHalf1++;
            }
            else if(ReadHalf1 == End)
            {
                *Out++ = *ReadHalf0++;
            }
            else if(ReadHalf0->SortKey < ReadHalf1->SortKey)
            {
                *Out++ = *ReadHalf0++;
            }
            else
            {
                *Out++ = *ReadHalf1++;
            }
        }
        Assert(Out == (Temp + Count));
        Assert(ReadHalf0 == InHalf1);
        Assert(ReadHalf1 == End);

        // TODO: Not necessary if we ping-pong
        for(u32 Index = 0; Index < Count; ++Index)
        {
            First[Index] = Temp[Index];
        }

#if 0
        sort_entry *ReadHalf0 = First;
        sort_entry *ReadHalf1 = First + Half0;
        sort_entry *End = First + Count;

        // NOTE: Step 1 - Find the first out of order pair
        while((ReadHalf0 != ReadHalf1) && (ReadHalf0->SortKey < ReadHalf1->SortKey))
        {
            ++ReadHalf0;
        }

        // NOTE: Step 2 - Swap as many Half1 items as needed
        if(ReadHalf0 != ReadHalf1)
        {
            sort_entry CompareWith = *ReadHalf0;
            while((ReadHalf1 != End) && (ReadHalf1->SortKey < CompareWith.SortKey))
            {
                Swap(ReadHalf0++, ReadHalf1++);
            }

            ReadHalf1 = InHalf1;
        }
#endif
    }
}

internal void
BubbleSort(u32 Count, sort_entry *First, sort_entry *Temp)
{
    // NOTE: This is O(n^2) bubble sort 
    for(u32 Outer = 0; Outer < Count; ++Outer)
    {
        b32 ListIsSorted = true;
        // TODO: Early Out
        for(u32 Inner = 0; Inner < (Count - 1); ++Inner)
        {
            sort_entry *EntryA = First + Inner;
            sort_entry *EntryB = EntryA + 1;

            if(EntryA->SortKey > EntryB->SortKey)
            {
                Swap(EntryA, EntryB);
                ListIsSorted = false;
            }
        }

        if(ListIsSorted)
        {
            break;
        }
    }
}

internal void
RadixSort(u32 Count, sort_entry *First, sort_entry *Temp)
{
    sort_entry *Source = First;
    sort_entry *Dest = Temp;
    for(u32 ByteIndex = 0; ByteIndex < 32; ByteIndex += 8)
    {
        u32 SortKeyOffsets[256] = {};

        // NOTE: First pass - count how many of each key 
        for(u32 Index = 0; Index < Count; ++Index)
        {
            u32 RadixValue = SortKeyToU32(Source[Index].SortKey);
            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
            ++SortKeyOffsets[RadixPiece];
        }

        // NOTE: Change counts to offsets 
        u32 Total = 0;
        for(u32 SortKeyIndex = 0; SortKeyIndex < ArrayCount(SortKeyOffsets); ++SortKeyIndex)
        {
            u32 Count = SortKeyOffsets[SortKeyIndex];
            SortKeyOffsets[SortKeyIndex] = Total;
            Total += Count;
        }

        // NOTE: Second pass - place elements into the right location
        for(u32 Index = 0; Index < Count; ++Index)
        {
            u32 RadixValue = SortKeyToU32(Source[Index].SortKey);
            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
            Dest[SortKeyOffsets[RadixPiece]++] = Source[Index];
        }

        sort_entry *SwapTemp = Dest;
        Dest = Source;
        Source = SwapTemp;
    }
}

struct sprite_bound 
{
    r32 YMin;
    r32 YMax;
    r32 ZMax;
};

struct sprite_edge
{
    sprite_edge *NextEdgeWithSameFront;
    u32 Front;
    u32 Behind;
};

enum sprite_flag
{
    Sprite_Visited = 0x1,
    Sprite_Drawn = 0x2,
};
struct sort_sprite_bound
{
    sprite_edge *FirstEdgeWithMeAsFront;
    rectangle2 ScreenArea;
    sprite_bound SortKey;
    u32 Index;
    u32 Flags;
};

inline b32
IsInFrontOf(sprite_bound A, sprite_bound B)
{
    b32 BothZSprites = ((A.YMin != A.YMax) && (B.YMin != B.YMax));
    b32 AIncludesB = ((B.YMin >= A.YMin) && (B.YMin < A.YMax));
    b32 BIncludesA = ((A.YMin >= B.YMin) && (A.YMin < B.YMax));

    b32 SortByZ = (BothZSprites || AIncludesB || BIncludesA);

    b32 Result = (SortByZ ? (A.ZMax > B.ZMax) : (A.YMin < B.YMin));
    return(Result);
}

inline void
Swap(sort_sprite_bound *A, sort_sprite_bound *B)
{
    sort_sprite_bound Temp = *B;
    *B = *A;
    *A = Temp;
}

internal void
MergeSort(u32 Count, sort_sprite_bound *First, sort_sprite_bound *Temp)
{
    if(Count == 1)
    {
        // NOTE: Nothing to do
    }
    else if(Count == 2)
    {
        sort_sprite_bound *EntryA = First;
        sort_sprite_bound *EntryB = First + 1;
        if(IsInFrontOf(EntryA->SortKey, EntryB->SortKey))
        {
            Swap(EntryA, EntryB);
        }
    }
    else
    {
        u32 Half0 = Count / 2;
        u32 Half1 = Count - Half0;

        Assert(Half0 >= 1);
        Assert(Half1 >= 1);

        sort_sprite_bound *InHalf0 = First;
        sort_sprite_bound *InHalf1 = First + Half0;
        sort_sprite_bound *End = First + Count;

        MergeSort(Half0, InHalf0, Temp);
        MergeSort(Half1, InHalf1, Temp);

        sort_sprite_bound *ReadHalf0 = InHalf0;
        sort_sprite_bound *ReadHalf1 = InHalf1;

        sort_sprite_bound *Out = Temp;
        for(u32 Index = 0; Index < Count; ++Index)
        {
            if(ReadHalf0 == InHalf1)
            {
                *Out++ = *ReadHalf1++;
            }
            else if(ReadHalf1 == End)
            {
                *Out++ = *ReadHalf0++;
            }
            else if(IsInFrontOf(ReadHalf1->SortKey, ReadHalf0->SortKey))
            {
                *Out++ = *ReadHalf0++;
            }
            else
            {
                *Out++ = *ReadHalf1++;
            }
        }
        Assert(Out == (Temp + Count));
        Assert(ReadHalf0 == InHalf1);
        Assert(ReadHalf1 == End);

        // TODO: Not necessary if we ping-pong
        for(u32 Index = 0; Index < Count; ++Index)
        {
            First[Index] = Temp[Index];
        }
    }
}

inline b32
IsZSprite(sprite_bound Bound)
{
    b32 Result = (Bound.YMin != Bound.YMax);
    return(Result);
}

internal void 
VerifyBuffer(u32 Count, sort_sprite_bound *Buffer, b32 ZSprite)
{
    for(u32 Index = 0; Index < Count; ++Index)
    {
        Assert(IsZSprite(Buffer[Index].SortKey) == ZSprite);
        if(Index > 0)
        {
            Assert(IsInFrontOf(Buffer[Index].SortKey, Buffer[Index - 1].SortKey));
        }
    }
}

internal void
SeperatedSort(u32 Count, sort_sprite_bound *First, sort_sprite_bound *Temp)
{
    u32 YCount = 0;
    u32 ZCount = 0;
    for(u32 Index = 0; Index < Count; ++Index)
    {
        sort_sprite_bound *This = First + Index;
        if(IsZSprite(This->SortKey))
        {
            Temp[ZCount++] = *This;
        }
        else 
        {
            First[YCount++] = *This;
        }
    }

#if GAME_SLOW 
    VerifyBuffer(YCount, First, false);
    VerifyBuffer(ZCount, Temp, true);
#endif

    MergeSort(YCount, First, Temp + ZCount);
    MergeSort(ZCount, Temp, First + YCount);
    if(YCount == 1)
    {
        Temp[ZCount] = First[0];
    }
    else if(YCount == 2)
    {
        Temp[ZCount] = First[0];
        Temp[ZCount + 1] = First[1];
    }

    sort_sprite_bound *InHalf0 = Temp;
    sort_sprite_bound *InHalf1 = Temp + ZCount;

#if GAME_SLOW 
    VerifyBuffer(YCount, InHalf1, false);
    VerifyBuffer(ZCount, InHalf0, true);
#endif

    sort_sprite_bound *End = InHalf1 + YCount;
    sort_sprite_bound *ReadHalf0 = InHalf0;
    sort_sprite_bound *ReadHalf1 = InHalf1;

    sort_sprite_bound *Out = First;
    for(u32 Index = 0; Index < Count; ++Index)
    {
        if(ReadHalf0 == InHalf1)
        {
            *Out++ = *ReadHalf1++;
        }
        else if(ReadHalf1 == End)
        {
            *Out++ = *ReadHalf0++;
        }
        //TODO: This merge comparison can be simpler now since we know
        // which sprite is a Z sprite and which is a Y sprite
        else if(IsInFrontOf(ReadHalf1->SortKey, ReadHalf0->SortKey))
        {
            *Out++ = *ReadHalf0++;
        }
        else
        {
            *Out++ = *ReadHalf1++;
        }
    }
    Assert(Out == (First + Count));
    Assert(ReadHalf0 == InHalf1);
    Assert(ReadHalf1 == End);
}

inline sort_sprite_bound *
GetSortEntries(game_render_commands *Commands)
{
    sort_sprite_bound *SortEntries = (sort_sprite_bound *)(Commands->PushBufferBase + Commands->SortEntryAt);
    return(SortEntries);
}

inline umm
GetSortTempMemorySize(game_render_commands *Commands)
{
    umm NeededSortMemorySize = Commands->PushBufferElementCount * sizeof(sort_sprite_bound);
    return(NeededSortMemorySize);
}

internal void 
BuildSpriteGraph(u32 InputNodeCount, sort_sprite_bound *InputNodes, memory_arena *Arena)
{
    if(InputNodeCount)
    {
        for(u32 NodeIndexA = 0; NodeIndexA < (InputNodeCount - 1); ++NodeIndexA)
        {
            sort_sprite_bound *A = InputNodes + NodeIndexA;
            Assert(A->Flags == 0);

            for(u32 NodeIndexB = NodeIndexA; NodeIndexB < InputNodeCount; ++NodeIndexB)
            {
                sort_sprite_bound *B = InputNodes + NodeIndexB;

                if(RectanglesIntersect(A->ScreenArea, B->ScreenArea))
                {
                    u32 FrontIndex = NodeIndexA;
                    u32 BackIndex = NodeIndexB;
                    if(IsInFrontOf(B->SortKey, A->SortKey))
                    {
                        u32 Temp = FrontIndex;
                        FrontIndex = BackIndex;
                        BackIndex = Temp;
                    }

                    // TODO: Reenable the push
                    sprite_edge *Edge = 0; //PushStruct(Arena, sprite_edge);
                    sort_sprite_bound *Front = InputNodes + FrontIndex;
                    Edge->Front = FrontIndex;
                    Edge->Behind = BackIndex;

                    Edge->NextEdgeWithSameFront = Front->FirstEdgeWithMeAsFront;
                    Front->FirstEdgeWithMeAsFront = Edge;
                }
            }
        }
    }
}

struct sprite_graph_walk
{
    sort_sprite_bound *InputNodes;
    u32 *OutIndex;
};

internal void
RecursiveFromToBack(sprite_graph_walk *Walk, u32 AtIndex)
{
    sort_sprite_bound *At = Walk->InputNodes + AtIndex;
    if(!(At->Flags & Sprite_Visited))
    {
        At->Flags |= Sprite_Visited;

        for(sprite_edge *Edge = At->FirstEdgeWithMeAsFront; Edge; Edge = Edge->NextEdgeWithSameFront)
        {
            Assert(Edge->Front == AtIndex);
            RecursiveFromToBack(Walk, Edge->Behind);
        }

        *Walk->OutIndex++ = AtIndex;
    }
}

internal void
WalkSpriteGraph(u32 InputNodeCount, sort_sprite_bound *InputNodes, u32 *OutIndexArray)
{
    sprite_graph_walk Walk = {};
    Walk.InputNodes = InputNodes;
    Walk.OutIndex = OutIndexArray;
    for(u32 NodeIndexA = 0; NodeIndexA < InputNodeCount; ++NodeIndexA)
    {
        RecursiveFromToBack(&Walk, NodeIndexA);
    }
}

internal void
SortEntries(game_render_commands *Commands, memory_arena *TempArena, u32 *OutIndexArray)
{
    TIMED_FUNCTION();

    u32 Count = Commands->PushBufferElementCount;
    sort_sprite_bound *Entries = GetSortEntries(Commands);

    BuildSpriteGraph(Count, Entries, TempArena);
    WalkSpriteGraph(Count, Entries, OutIndexArray);

#if GAME_SLOW
    if(Count)
    {
        for(u32 Index = 0; Index < (Count - 1); ++Index)
        {
#if 0
            // NOTE: This is the O(n) partial ordering check - only neighbors are verified
            u32 IndexB = Index + 1;
            {
#else 
            // NOTE: This is the O(n^2) total ordering check - all pairs verified
            for(u32 IndexB = Index + 1; IndexB < Count; ++IndexB)
            {
#endif
                sort_sprite_bound *EntryA = Entries + Index;
                sort_sprite_bound *EntryB = Entries + IndexB;

                if(IsInFrontOf(EntryA->SortKey, EntryB->SortKey))
                {
                    Assert((EntryA->SortKey.YMin == EntryB->SortKey.YMin) &&
                           (EntryA->SortKey.YMax == EntryB->SortKey.YMax) &&
                           (EntryA->SortKey.ZMax == EntryB->SortKey.ZMax));
                }
            }
        }
    }
#endif
}



struct entity;

//
// NOTE: Brain Types
//

struct brain_hero
{
    entity *Head;
    entity *Body;
    entity *Glove;
};

struct brain_monster
{
    entity *Body;
};

struct brain_familiar
{
    entity *Head;
};

struct brain_snake
{
    entity *Segments[16];
};

//
// NOTE: Brain
//

enum brain_type
{
    Type_brain_hero,

    // NOTE: Test Brains
    Type_brain_snake,
    Type_brain_familiar,
    Type_brain_floaty_thing_for_now,
    Type_brain_monster,

    // NOTE: These are special types which are used to mark special entities,
    // but these entities do not get actual brains (the brain ID value is set to 0)
    Type_brain_room,

    Type_brain_count,
};

struct brain_slot
{
    u16 Type;
    u16 Index;
};
inline b32
IsType(brain_slot Slot, brain_type Type)
{
    b32 Result = ((Slot.Index != 0) && (Slot.Type == Type));
    return(Result);
}

inline brain_slot
SpecialBrainSlot(brain_type Type)
{
    brain_slot Result = {(u16)Type, 0};
    return(Result);
};

#define BrainSlotFor(type, Member) BrainSlotFor_(Type_##type, &(((type *)0)->Member) - (entity **)0)
#define IndexedBrainSlotFor(type, Member, Index) BrainSlotFor_(Type_##type, (u16)(Index) + (u16)((((type *)0)->Member) - (entity **)0))
inline brain_slot
BrainSlotFor_(brain_type Type, u16 PackValue)
{
    brain_slot Result = {(u16)Type, PackValue};

    return(Result);
}

struct brain_id
{
    u32 Value;
};
inline brain_id NoBrain() {brain_id Result = {}; return(Result);}

struct brain
{
    brain_id ID;
    brain_type Type;

    union 
    {
        entity *Array;
        brain_hero Hero;
        brain_monster Monster;
        brain_familiar Familiar;
        brain_snake Snake;
    };
};
#define MAX_BRAIN_SLOT_COUNT ((sizeof(brain) - OffsetOf(brain, Array)) / sizeof(entity *))
inline entity *
GetEntityInSlot(brain *Brain, u32 SlotIndex)
{
    Assert(SlotIndex < MAX_BRAIN_SLOT_COUNT);
    entity *Result = (&Brain->Array)[SlotIndex];
    return(Result);
}

enum reserved_brain_id
{
    ReservedBrainID_FirstHero = 1,
    ReservedBrainID_LastHero = (ReservedBrainID_FirstHero + MAX_CONTROLLER_COUNT - 1),

    ReservedBrainID_FirstFree,
};

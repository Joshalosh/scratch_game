
struct entity;

//
// NOTE: Brain Types
//

struct brain_hero
{
    entity *Head;
    entity *Body;
};

struct brain_monster
{
    entity *Body;
};

struct brain_familiar
{
    entity *Head;
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

    Type_brain_count,
};

struct brain_slot
{
    u16 Type;
    u16 Index;
};
#define BrainSlotFor(type, Member) BrainSlotFor_(Type_##type, &(((type *)0)->Member) - (entity **)0)
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
        brain_hero Hero;
        brain_monster Monster;
        brain_familiar Familiar;

        entity *Array[16];
    };
};

enum reserved_brain_id
{
    ReservedBrainID_FirstHero = 1,
    ReservedBrainID_LastHero = (ReservedBrainID_FirstHero + MAX_CONTROLLER_COUNT - 1),

    ReservedBrainID_FirstFree,
};


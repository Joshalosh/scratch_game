#if !defined(GAME_ASSET_TYPE_ID_H)

enum asset_tag_id
{
    Tag_Smoothness,
    Tag_Flatness,
    Tag_FacingDirection, // Angle in radians off of due right.

    Tag_Count,
};

enum asset_type_id
{
    Asset_None,

    //
    // Bitmaps
    //

    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
//    Asset_Stairwell,
    Asset_Rock,

    Asset_Grass,
    Asset_Tuft,
    Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

    //
    // Sounds
    //

    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,
    
    //
    //
    //

    Asset_Count,
};

#define GAME_ASSET_TYPE_ID_H
#endif
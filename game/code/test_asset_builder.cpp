
#include <stdio.h>
#include <stdlib.h>
#include "game_platform.h"
#include "game_asset_type_id.h"

FILE *Out = 0;

struct asset_bitmap_info
{
    char *Filename;
    r32 AlignPercentage[2];
};

struct asset_sound_info
{
    char *Filename;
    u32 FirstSampleIndex;
    u32 SampleCount;
    u32 NextIDToPlay;
};

struct asset_tag
{
    u32 ID; // Tag ID.
    real32 Value;
};
struct asset
{
    u64 DataOffset;
    u32 FirstTagIndex;
    u32 OnePastLastTagIndex;
};
struct asset_type
{
    u32 FirstAssetIndex;
    u32 OnePastLastAssetIndex;
};

struct bitmap_asset
{
    char *Filename;
    r32 Alignment[2];
};

#define VERY_LARGE_NUMBER 4096

uint32_t BitmapCount;
uint32_t SoundCount;
uint32_t TagCount;
uint32_t AssetCount;
asset_bitmap_info BitmapInfos[VERY_LARGE_NUMBER];
asset_sound_info SoundInfos[VERY_LARGE_NUMBER];
asset_tag Tags[VERY_LARGE_NUMBER];
asset Assets[VERY_LARGE_NUMBER];
asset_type AssetTypes[Asset_Count];

u32 DEBUGUsedBitmapCount;
u32 DEBUGUsedSoundCount;
u32 DEBUGUsedAssetCount;
u32 DEBUGUsedTagCount;
asset_type *DEBUGAssetType;
asset *DEBUGAsset;

internal void
BeginAssetType(asset_type_id TypeID)
{
    Assert(DEBUGAssetType == 0);

    DEBUGAssetType = AssetTypes + TypeID;
    DEBUGAssetType->FirstAssetIndex = DEBUGUsedAssetCount;
    DEBUGAssetType->OnePastLastAssetIndex = DEBUGAssetType->FirstAssetIndex;
}

internal void
AddBitmapAsset(char *Filename, r32 AlignPercentageX, r32 AlignPercentageY)
{
    Assert(DEBUGAssetType);
    Assert(DEBUGAssetType->OnePastLastAssetIndex < AssetCount);

    asset *Asset = Assets + DEBUGAssetType->OnePastLastAssetIndex++;
    Asset->FirstTagIndex = DEBUGUsedTagCount;
    Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
    Asset->SlotID = DEBUGAddBitmapInfo(Assets, Filename, AlignPercentageX, AlignPercentageY).Value;

    /*
    internal bitmap_id
    DEBUGAddBitmapInfo(char *Filename, v2 AlignPercentage)
    {
        Assert(Assets->DEBUGUsedBitmapCount < Assets->BitmapCount);
        bitmap_id ID = {Assets->DEBUGUsedBitmapCount++};

        asset_bitmap_info *Info = Assets->BitmapInfos + ID.Value;
        Info->Filename = PushString(&Assets->Arena, Filename);
        Info->AlignPercentage = AlignPercentage;

        return(ID);
    }
    */

    Assets->DEBUGAsset = Asset;
}

internal asset *
AddSoundAsset(game_assets *Assets, char *Filename, u32 FirstSampleIndex = 0, u32 SampleCount = 0)
{
    Assert(Assets->DEBUGAssetType);
    Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

    asset *Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
    Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
    Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
    Asset->SlotID = DEBUGAddSoundInfo(Assets, Filename, FirstSampleIndex, SampleCount).Value;

    /*
    internal sound_id
    DEBUGAddSoundInfo(char *Filename, u32 FirstSampleIndex, u32 SampleCount)
    {
        Assert(Assets->DEBUGUsedSoundCount < Assets->SoundCount);
        sound_id ID = {Assets->DEBUGUsedSoundCount++};

        asset_sound_info *Info = Assets->SoundInfos + ID.Value;
        Info->Filename = PushString(&Assets->Arena, Filename);
        Info->FirstSampleIndex = FirstSampleIndex;
        Info->SampleCount = SampleCount;
        Info->NextIDToPlay.Value = 0;

        return(ID);
    }
    */

    Assets->DEBUGAsset = Asset;

    return(Asset);
}

internal void
AddTag(game_assets *Assets, asset_tag_id ID, real32 Value)
{
    Assert(Assets->DEBUGAsset);

    ++Assets->DEBUGAsset->OnePastLastTagIndex;
    asset_tag *Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;

    Tag->ID = ID;
    Tag->Value = Value;
}

internal void
EndAssetType(game_assets *Assets)
{
    Assert(Assets->DEBUGAssetType);
    Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
    Assets->DEBUGAssetType = 0;
    Assets->DEBUGAsset = 0;
}

int
main(int ArgCount, char **Args)
{
    BeginAssetType(Assets, Asset_Shadow);
    AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", V2(0.5f, 0.156682029f));
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tree);
    AddBitmapAsset(Assets, "test2/tree00.bmp", V2(0.493827164f, 0.295652181f));
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Sword);
    AddBitmapAsset(Assets, "test2/rock03.bmp", V2(0.5f, 0.65625f));
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Grass);
    AddBitmapAsset(Assets, "test2/grass00.bmp");
    AddBitmapAsset(Assets, "test2/grass01.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Tuft);
    AddBitmapAsset(Assets, "test2/tuft00.bmp");
    AddBitmapAsset(Assets, "test2/tuft01.bmp");
    AddBitmapAsset(Assets, "test2/tuft02.bmp");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Stone);
    AddBitmapAsset(Assets, "test2/ground00.bmp");
    AddBitmapAsset(Assets, "test2/ground01.bmp");
    AddBitmapAsset(Assets, "test2/ground02.bmp");
    AddBitmapAsset(Assets, "test2/ground03.bmp");
    EndAssetType(Assets);

    real32 AngleRight = 0.0f*Tau32;
    real32 AngleBack = 0.25f*Tau32;
    real32 AngleLeft = 0.5f*Tau32;
    real32 AngleFront = 0.75f*Tau32;

    v2 HeroAlign = {0.5f, 0.156682029f};

    BeginAssetType(Assets, Asset_Head);
    AddBitmapAsset(Assets, "test/test_hero_right_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_head.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Cape);
    AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Torso);
    AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleRight);
    AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleBack);
    AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleLeft);
    AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp", HeroAlign);
    AddTag(Assets, Tag_FacingDirection, AngleFront);
    EndAssetType(Assets);

    //
    //
    //

    BeginAssetType(Assets, Asset_Bloop);
    AddSoundAsset(Assets, "test3/bloop_00.wav");
    AddSoundAsset(Assets, "test3/bloop_01.wav");
    AddSoundAsset(Assets, "test3/bloop_02.wav");
    AddSoundAsset(Assets, "test3/bloop_03.wav");
    AddSoundAsset(Assets, "test3/bloop_04.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Crack);
    AddSoundAsset(Assets, "test3/crack_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Drop);
    AddSoundAsset(Assets, "test3/drop_00.wav");
    EndAssetType(Assets);

    BeginAssetType(Assets, Asset_Glide);
    AddSoundAsset(Assets, "test3/glide_00.wav");
    EndAssetType(Assets);

    u32 OneMusicChunk = 10*48000;
    u32 TotalMusicSampleCount = 7468095;
    BeginAssetType(Assets, Asset_Music);
    asset *LastMusic = 0;
    for(u32 FirstSampleIndex = 0; FirstSampleIndex < TotalMusicSampleCount; FirstSampleIndex += OneMusicChunk)
    {
        u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
        if(SampleCount > OneMusicChunk)
        {
            SampleCount = OneMusicChunk;
        }
        asset *ThisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, OneMusicChunk);
        if(LastMusic)
        {
            Assets->SoundInfos[LastMusic->SlotID].NextIDToPlay.Value = ThisMusic->SlotID;
        }
        LastMusic = ThisMusic;
    }
    EndAssetType(Assets);
    
    BeginAssetType(Assets, Asset_Puhp);
    AddSoundAsset(Assets, "test3/puhp_00.wav");
    AddSoundAsset(Assets, "test3/puhp_01.wav");
    EndAssetType(Assets);

    Out = fopen("test.ga", "wb");
    if(Out)
    {
        fclose(Out);
    }
}

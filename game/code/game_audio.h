#if !defined(GAME_AUDIO_H)

struct playing_sound
{
    real32 Volume[2];
    sound_id ID;
    int32_t SamplesPlayed;
    playing_sound *Next;
};

struct audio_state
{
    memory_arena *PermArena;
    playing_sound *FirstPlayingSound;
    playing_sound *FirstFreePlayingSound;
};

#define GAME_AUDIO_H
#endif

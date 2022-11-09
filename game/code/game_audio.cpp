
internal void
OutputTestSineWave(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16_t ToneVolume = 3000; 
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples; 
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 1
        real32 SineValue = sinf(GameState->tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
#else
        int16_t SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

#if 1
        GameState->tSine += Tau32*1.0f/(real32)WavePeriod;
        if(GameState->tSine > Tau32)
        {
            GameState->tSine -= Tau32;
        }
#endif
    }
}

internal playing_sound *
PlaySound(audio_state *AudioState, sound_id SoundID)
{
    if(!AudioState->FirstFreePlayingSound)
    {
        AudioState->FirstFreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);
        AudioState->FirstFreePlayingSound->Next = 0;
    }

    playing_sound *PlayingSound = AudioState->FirstFreePlayingSound;
    AudioState->FirstFreePlayingSound = PlayingSound->Next;

    PlayingSound->SamplesPlayed = 0;
    // TODO: Should these default to 0.5f/0.5f for centred?
    PlayingSound->CurrentVolume = PlayingSound->TargetVolume = V2(1.0f, 1.0f);
    PlayingSound->dCurrentVolume = V2(0, 0);
    PlayingSound->ID = SoundID; 
    PlayingSound->dSample = 1.0f;

    PlayingSound->Next = AudioState->FirstPlayingSound;
    AudioState->FirstPlayingSound = PlayingSound;

    return(PlayingSound);
}

internal void
ChangeVolume(audio_state *AudioState, playing_sound *Sound, real32 FadeDurationInSeconds, v2 Volume)
{
    if(FadeDurationInSeconds <= 0.0f)
    {
        Sound->CurrentVolume = Sound->TargetVolume = Volume;
    }
    else
    {
        real32 OneOverFade = 1.0f / FadeDurationInSeconds;
        Sound->TargetVolume = Volume;
        Sound->dCurrentVolume = OneOverFade*(Sound->TargetVolume - Sound->CurrentVolume);
    }
}

internal void
ChangePitch(audio_state *AudioState, playing_sound *Sound, real32 dSample)
{
    Sound->dSample = dSample;
}

internal void
OutputPlayingSounds(audio_state *AudioState,
                    game_sound_output_buffer *SoundBuffer, game_assets *Assets,
                    memory_arena *TempArena)
{
    temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

    Assert((SoundBuffer->SampleCount & 7) == 0);
    u32 SampleCount8 = SoundBuffer->SampleCount / 8;
    u32 SampleCount4 = SoundBuffer->SampleCount / 4;

    __m128 *RealChannel0 = PushArray(TempArena, SampleCount4, __m128, 16);
    __m128 *RealChannel1 = PushArray(TempArena, SampleCount4, __m128, 16);

    real32 SecondsPerSample = 1.0f / (real32)SoundBuffer->SamplesPerSecond;
#define AudioStateOutputChannelCount 2

    // Clear out the mixer channels.
    __m128 Zero = _mm_set1_ps(0.0f);
    {
        __m128 *Dest0 = RealChannel0;
        __m128 *Dest1 = RealChannel1;
        for(u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
        {
            _mm_store_ps((float *)Dest0++, Zero);
            _mm_store_ps((float *)Dest1++, Zero);
        }
    }

    // Sum all sounds.
    for(playing_sound **PlayingSoundPtr = &AudioState->FirstPlayingSound; *PlayingSoundPtr;)
    {
        playing_sound *PlayingSound = *PlayingSoundPtr;
        bool32 SoundFinished = false;

        u32 TotalSamplesToMix8 = SampleCount8;
        real32 *Dest0 = (r32 *)RealChannel0;
        real32 *Dest1 = (r32 *)RealChannel1;

        while(TotalSamplesToMix8 && !SoundFinished)
        {
            loaded_sound *LoadedSound = GetSound(Assets, PlayingSound->ID);
            if(LoadedSound)
            {
                asset_sound_info *Info = GetSoundInfo(Assets, PlayingSound->ID);
                PrefetchSound(Assets, Info->NextIDToPlay);

                v2 Volume = PlayingSound->CurrentVolume;;
                v2 dVolume8 = 8.0f*SecondsPerSample*PlayingSound->dCurrentVolume;
                real32 dSample8 = 8.0f*PlayingSound->dSample;

                Assert(PlayingSound->SamplesPlayed >= 0);

                s32 FirstSampleIndex = RoundReal32ToInt32(PlayingSound->SamplesPlayed);
                u32 SamplesToMix8 = TotalSamplesToMix8;
                r32 RealSamplesRemainingInSound8 = (LoadedSound->SampleCount - 
                                                   RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSample8;
                u32 SamplesRemainingInSound8 = RoundReal32ToInt32(RealSamplesRemainingInSound8);
                if(SamplesToMix8 > SamplesRemainingInSound8)
                {
                    SamplesToMix8 = SamplesRemainingInSound8;
                }

                b32 VolumeEnded[AudioStateOutputChannelCount] = {};
                for(u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEnded); ++ChannelIndex)
                {
                    // TODO: Fix the "both volumes end at the same time" bug
                    if(dVolume8.E[ChannelIndex] != 0.0f)
                    {
                        real32 DeltaVolume = (PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex]);
                        u32 VolumeSampleCount8 = (u32)(((DeltaVolume / dVolume8.E[ChannelIndex]) + 0.5f));
                        if(SamplesToMix8 > VolumeSampleCount8)
                        {
                            SamplesToMix8 = VolumeSampleCount8;
                            VolumeEnded[ChannelIndex] = true;
                        }
                    }
                }

                // TODO: Handle stero.
                real32 SamplePosition = PlayingSound->SamplesPlayed;
                for(u32 LoopIndex = 0; LoopIndex < SamplesToMix8; ++LoopIndex)
                {
                    for(u32 SampleOffset = 0; SampleOffset < 8; ++SampleOffset)
                    {
#if 1
                        u32 SampleIndex = FloorReal32ToInt32(SamplePosition);
                        r32 Frac = SamplePosition - (r32)SampleIndex;

                        SampleIndex += SampleOffset;

                        r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
                        r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
                        r32 SampleValue = Lerp(Sample0, Frac, Sample1);
#else
                        u32 SampleIndex = RoundReal32ToInt32(SamplePosition) + SampleOffset;
                        r32 SampleValue = LoadedSound->Samples[0][SampleIndex];
#endif

                        *Dest0++ += AudioState->MasterVolume.E[0]*Volume.E[0]*SampleValue;
                        *Dest1++ += AudioState->MasterVolume.E[1]*Volume.E[1]*SampleValue;
                    }

                    Volume += dVolume8;
                    SamplePosition += dSample8;
                }

                PlayingSound->CurrentVolume = Volume;

                // TODO: Need to truncate loop to make this correct.
                for(u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEnded); ++ChannelIndex)
                {
                    if(VolumeEnded[ChannelIndex])
                    {
                        PlayingSound->CurrentVolume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
                        PlayingSound->dCurrentVolume.E[ChannelIndex] = 0.0f;
                    }
                }

                PlayingSound->SamplesPlayed = SamplePosition;
                Assert(TotalSamplesToMix8 >= SamplesToMix8);
                TotalSamplesToMix8 -= SamplesToMix8;

                if((uint32_t)PlayingSound->SamplesPlayed >= LoadedSound->SampleCount)
                {
                    if(IsValid(Info->NextIDToPlay))
                    {
                        PlayingSound->ID = Info->NextIDToPlay;
                        PlayingSound->SamplesPlayed = 0;
                    }
                    else
                    {
                        SoundFinished = true;
                    }
                }
            }
            else
            {
                LoadSound(Assets, PlayingSound->ID);
                break;
            }
        }

        if(SoundFinished)
        {
            *PlayingSoundPtr = PlayingSound->Next;
            PlayingSound->Next = AudioState->FirstFreePlayingSound;
            AudioState->FirstFreePlayingSound = PlayingSound;
        }
        else 
        {
            PlayingSoundPtr = &PlayingSound->Next;
        }
    }

    // Convert to 16-bit.
    {
        __m128 *Source0 = RealChannel0;
        __m128 *Source1 = RealChannel1;

        __m128i *SampleOut = (__m128i *)SoundBuffer->Samples;
        for(u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex)
        {
              __m128 S0 = _mm_load_ps((float *)Source0++);
              __m128 S1 = _mm_load_ps((float *)Source1++);

              __m128i L = _mm_cvtps_epi32(S0);
              __m128i R = _mm_cvtps_epi32(S1);

              __m128i LR0 = _mm_unpacklo_epi32(L, R);
              __m128i LR1 = _mm_unpackhi_epi32(L, R);

              __m128i S01 = _mm_packs_epi32(LR0, LR1);

              *SampleOut++ = S01;
        }
    }

    EndTemporaryMemory(MixerMemory);
}

internal void
InitialiseAudioState(audio_state *AudioState, memory_arena *PermArena)
{
    AudioState->PermArena = PermArena;
    AudioState->FirstPlayingSound = 0;
    AudioState->FirstFreePlayingSound = 0;

    AudioState->MasterVolume = V2(1.0f, 1.0f);
}

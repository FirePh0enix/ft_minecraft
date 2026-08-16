#pragma once

#include "Audio/AudioClip.hpp"
#include "SDL3_mixer/SDL_mixer.h"
#include "World/Biome.hpp"
#include <array>
#include <cstddef>

class MusicPlayer
{
public:
    explicit MusicPlayer(MIX_Mixer& mixer);
    ~MusicPlayer();

    void update(float delta);

    void play(AudioClip *clip, bool loop = true);
    void stop(float fade_duration = 0.0f);

    void crossfade_to(AudioClip *clip, float delay, float duration);

    void set_volume(float volume);

    AudioClip& get_biome_music(Biome biome)
    {
        return m_audio_clips.at(static_cast<size_t>(biome));
    }

private:
    MIX_Mixer& m_mixer;

    MIX_Track *m_tracks[2]{};
    size_t m_current_track = 0;

    AudioClip *m_current_clip = nullptr;
    AudioClip *m_next_clip = nullptr;

    float m_transition_delay = 0.0f;
    float m_transition_duration = 0.0f;

    float m_volume = 0.1f;

    std::array<AudioClip, 7> m_audio_clips;
};

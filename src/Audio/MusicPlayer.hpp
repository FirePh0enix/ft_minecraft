#pragma once

#include "Audio/AudioClip.hpp"
#include "SDL3_mixer/SDL_mixer.h"
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

    AudioClip *get_clip(size_t index)
    {
        return &m_audio_clips.at(index);
    }

    const AudioClip *get_clip(size_t index) const
    {
        return &m_audio_clips.at(index);
    }

private:
    MIX_Mixer& m_mixer;

    MIX_Track *m_tracks[2]{};
    size_t m_current_track = 0;

    AudioClip *m_current_clip = nullptr;
    AudioClip *m_next_clip = nullptr;

    float m_transition_delay = 0.0f;
    float m_transition_duration = 0.0f;

    float m_volume = 1.0f;

    std::array<AudioClip, 2> m_audio_clips;
};

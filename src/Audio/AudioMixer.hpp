#pragma once

#include "Audio/AudioListener.hpp"
#include <SDL3_mixer/SDL_mixer.h>
#include <array>
#include <cstddef>

constexpr size_t TRACKS_POOL_SIZE = 32;
constexpr size_t MUSIC_TRACKS_POOL_SIZE = 2;

class AudioMixer
{
public:
    AudioMixer();
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    MIX_Mixer *get_audio_mixer() const { return m_mixer; }
    AudioListener& get_audio_listener() { return m_audio_listener; }
    MIX_Track *get_available_track();

    inline const std::array<MIX_Track *, MUSIC_TRACKS_POOL_SIZE>& get_music_tracks_pool() const
    {
        return m_music_tracks_pool;
    }

private:
    MIX_Mixer *m_mixer = nullptr;
    AudioListener m_audio_listener;
    std::array<MIX_Track *, TRACKS_POOL_SIZE> m_tracks_pool{};
    std::array<MIX_Track *, MUSIC_TRACKS_POOL_SIZE> m_music_tracks_pool{};
};

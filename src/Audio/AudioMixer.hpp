#pragma once

#include "Audio/AudioListener.hpp"
#include "Core/Error.hpp"
#include <SDL3_mixer/SDL_mixer.h>
#include <array>
#include <cstddef>
#include <expected>
#include <memory>

constexpr size_t TRACKS_POOL_SIZE = 32;
constexpr size_t MUSIC_TRACKS_POOL_SIZE = 2;

class AudioMixer
{
public:
    static std::expected<std::shared_ptr<AudioMixer>, Error> create();

    AudioMixer();
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    MIX_Mixer *get_audio_mixer() const { return m_mixer; }
    bool is_valid() const { return m_valid; }
    AudioListener& get_audio_listener() { return m_audio_listener; }
    MIX_Track *get_available_track();
    float effects_volume() const { return m_effects_volume; }
    void set_effects_volume(float volume);
    void set_effects_track_gain(MIX_Track *track, float gain);

    inline const std::array<MIX_Track *, MUSIC_TRACKS_POOL_SIZE>& get_music_tracks_pool() const
    {
        return m_music_tracks_pool;
    }

private:
    MIX_Mixer *m_mixer = nullptr;
    bool m_valid = false;
    float m_effects_volume = 1.0f;
    std::array<float, TRACKS_POOL_SIZE> m_effects_gains{};
    AudioListener m_audio_listener;
    std::array<MIX_Track *, TRACKS_POOL_SIZE> m_tracks_pool{};
    std::array<MIX_Track *, MUSIC_TRACKS_POOL_SIZE> m_music_tracks_pool{};
};

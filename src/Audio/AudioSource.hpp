#pragma once

#include "Audio/AudioClip.hpp"
#include "Audio/AudioListener.hpp"
#include "Core/Math.hpp"

#include <SDL3_mixer/SDL_mixer.h>

class AudioSource
{
public:
    AudioSource(MIX_Mixer& mixer, AudioListener& listener);

    ~AudioSource()
    {
        if (m_track)
            MIX_DestroyTrack(m_track);
    }

    AudioSource(const AudioSource&) = delete;
    AudioSource& operator=(const AudioSource&) = delete;

    inline void set_clip(AudioClip *clip) { m_clip = clip; }

    void play_one_shot();
    void stop();
    void update();

    void set_volume(float volume);
    void set_position(const glm::vec3& position);

private:
    MIX_Track *m_track = nullptr;
    AudioClip *m_clip = nullptr;
    AudioListener& m_listener;

    float m_volume = 1.0f;
    glm::vec3 m_position{};
};

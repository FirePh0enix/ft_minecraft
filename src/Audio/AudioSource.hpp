#pragma once

#include "Audio/AudioClip.hpp"
#include "Audio/AudioMixer.hpp"
#include "Core/Math.hpp"

class AudioSource
{
public:
    explicit AudioSource(AudioMixer& mixer);

    AudioSource(const AudioSource&) = delete;
    AudioSource& operator=(const AudioSource&) = delete;

    void play_one_shot(AudioClip *clip, float volume);
    void play();
    void stop();
    void update();

    void set_volume(float volume);
    void set_position(const glm::vec3& position);
    inline void set_clip(AudioClip *clip) { m_clip = clip; }

private:
    AudioMixer& m_mixer;
    MIX_Track *m_track = nullptr;
    AudioClip *m_clip = nullptr;

    float m_volume = 1.0f;
    glm::vec3 m_position{};
};

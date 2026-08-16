#pragma once

#include "Audio/AudioListener.hpp"
#include <SDL3_mixer/SDL_mixer.h>

class AudioMixer
{
public:
    AudioMixer();
    ~AudioMixer();

    AudioMixer(const AudioMixer&) = delete;
    AudioMixer& operator=(const AudioMixer&) = delete;

    MIX_Mixer *get_audio_mixer() const { return m_mixer; }
    AudioListener& get_audio_listener() { return m_audio_listener; }

private:
    MIX_Mixer *m_mixer = nullptr;
    AudioListener m_audio_listener;
};

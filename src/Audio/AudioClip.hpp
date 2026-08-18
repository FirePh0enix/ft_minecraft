#pragma once

#include <SDL3_mixer/SDL_mixer.h>

#include <filesystem>

class AudioClip
{
public:
    AudioClip(MIX_Mixer &mixer, const std::filesystem::path& path);
    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;

    ~AudioClip()
    {
        if (m_audio)
            MIX_DestroyAudio(m_audio);
    }

    MIX_Audio *get_audio() const { return m_audio; }

private:
    MIX_Audio *m_audio = nullptr;
};

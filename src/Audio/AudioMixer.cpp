#include "AudioMixer.hpp"
#include "Core/Logger.hpp"

AudioMixer::AudioMixer() : m_audio_listener()
{
    if (!MIX_Init())
    {
        error("MIX_Init() failed: {}", SDL_GetError());
        return;
    }

    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

    if (!m_mixer)
    {
        error("MIX_CreateMixerDevice() failed: {}", SDL_GetError());
        MIX_Quit();
        return;
    }
}

AudioMixer::~AudioMixer()
{
    if (m_mixer)
        MIX_DestroyMixer(m_mixer);

    MIX_Quit();
}

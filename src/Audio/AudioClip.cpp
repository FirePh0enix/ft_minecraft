#include "AudioClip.hpp"

#include "Core/Logger.hpp"

AudioClip::AudioClip(MIX_Mixer &mixer, const std::filesystem::path& path)
{
    m_audio = MIX_LoadAudio(&mixer, path.string().c_str(), true);
    if (!m_audio)
        error("MIX_LoadAudio('{}') failed: {}", path.string(), SDL_GetError());
}


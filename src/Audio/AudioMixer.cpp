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

    for (size_t i = 0; i < TRACKS_POOL_SIZE; i++)
    {
        m_tracks_pool[i] = MIX_CreateTrack(m_mixer);
        if (!m_tracks_pool[i])
        {
            error("MIX_CreateTrack() failed: {} (tracks)", SDL_GetError());
            return;
        }
    }

    for (size_t i = 0; i < MUSIC_TRACKS_POOL_SIZE; i++)
    {
        m_music_tracks_pool[i] = MIX_CreateTrack(m_mixer);
        if (!m_tracks_pool[i])
        {
            error("MIX_CreateTrack() failed: {} (music)", SDL_GetError());
            return;
        }
    }
}

MIX_Track *AudioMixer::get_available_track()
{
    for (MIX_Track *track : m_tracks_pool)
    {
        if (track && !MIX_TrackPlaying(track))
            return track;
    }

    return nullptr;
}

AudioMixer::~AudioMixer()
{

    if (m_mixer)
        MIX_DestroyMixer(m_mixer);

    MIX_Quit();
}

#include "AudioSource.hpp"
#include "Audio/AudioMixer.hpp"
#include "Core/Logger.hpp"

AudioSource::AudioSource(AudioMixer& mixer) : m_mixer(mixer)
{
}

void AudioSource::play_one_shot(AudioClip *clip, float volume)
{
    if (!clip)
        return;

    MIX_Track *track = m_mixer.get_available_track();

    if (!track)
        return;

    if (!MIX_SetTrackAudio(track, clip->get_audio()))
    {
        error("MIX_SetTrackAudio() failed: {}", SDL_GetError());
        return;
    }

    if (!MIX_SetTrackGain(track, volume))
    {
        error("MIX_SetTrackGain() failed: {}", SDL_GetError());
        return;
    }

    if (!MIX_PlayTrack(track, 0))
    {
        error("MIX_PlayTrack() failed: {}", SDL_GetError());
        return;
    }
}

void AudioSource::play()
{
    if (!m_clip || (m_track && MIX_TrackPlaying(m_track)))
        return;

    m_track = m_mixer.get_available_track();

    if (!m_track)
        return;

    if (!MIX_SetTrackAudio(m_track, m_clip->get_audio()))
    {
        error("MIX_SetTrackAudio() failed: {}", SDL_GetError());
        m_track = nullptr;
        return;
    }

    set_volume(m_volume);

    SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);

    if (!MIX_PlayTrack(m_track, props))
    {
        error("MIX_PlayTrack() failed: {}", SDL_GetError());
        SDL_DestroyProperties(props);
        m_track = nullptr;
        return;
    }

    SDL_DestroyProperties(props);
}

void AudioSource::stop()
{
    if (!m_track)
        return;

    if (!MIX_StopTrack(m_track, 0))
        error("MIX_StopTrack() failed: {}", SDL_GetError());

    m_track = nullptr;
}

void AudioSource::update()
{
    if (!m_track)
        return;

    const auto& listener = m_mixer.get_audio_listener();

    const glm::vec3 pos = m_position - listener.get_position();
    const glm::vec3 forward = glm::normalize(listener.get_forward());
    const glm::vec3 up = glm::normalize(listener.get_up());
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));

    MIX_Point3D point{
        .x = glm::dot(pos, right),
        .y = glm::dot(pos, up),
        .z = -glm::dot(pos, forward),
    };

    if (!MIX_SetTrack3DPosition(m_track, &point))
        error("MIX_SetTrack3DPosition() failed: {}", SDL_GetError());
}

void AudioSource::set_volume(float volume)
{
    if (!m_track)
        return;

    if (!MIX_SetTrackGain(m_track, volume))
        error("MIX_SetTrackGain() failed: {}", SDL_GetError());

    m_volume = volume;
}

void AudioSource::set_position(const glm::vec3& position)
{
    m_position = position;
    update();
}

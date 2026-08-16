#include "AudioSource.hpp"

#include "Core/Logger.hpp"

AudioSource::AudioSource(MIX_Mixer& mixer, AudioListener& listener) : m_listener(listener)
{

    m_track = MIX_CreateTrack(&mixer);

    if (!m_track)
        error("MIX_CreateTrack() failed: {}", SDL_GetError());
}

void AudioSource::play_one_shot()
{
    if (!m_track || !m_clip)
        return;

    if (!MIX_SetTrackAudio(m_track, m_clip->get_audio()))
    {
        error("MIX_SetTrackAudio() failed: {}", SDL_GetError());
        return;
    }

    if (!MIX_PlayTrack(m_track, 0))
        error("MIX_PlayTrack() failed: {}", SDL_GetError());
}

void AudioSource::stop()
{
    if (!m_track)
        return;

    if (!MIX_StopTrack(m_track, 0))
        error("MIX_StopTrack() failed: {}", SDL_GetError());
}

void AudioSource::update()
{
    if (!m_track)
        return;

    const glm::vec3 pos = m_position - m_listener.get_position();
    const glm::vec3 forward = glm::normalize(m_listener.get_forward());
    const glm::vec3 up = glm::normalize(m_listener.get_up());
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

    m_volume = volume;
    if (!MIX_SetTrackGain(m_track, volume))
        error("MIX_SetTrackGain() failed: {}", SDL_GetError());
}

void AudioSource::set_position(const glm::vec3& position)
{
    m_position = position;
    update();
}

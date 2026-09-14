#include "AudioSource.hpp"
#include "Audio/AudioMixer.hpp"
#include "Core/Logger.hpp"

AudioSource::AudioSource(AudioMixer& mixer) : m_mixer(mixer)
{
}

AudioSource::~AudioSource()
{
    stop();
}

void AudioSource::play_one_shot(AudioClip *clip, float volume)
{
    if (!clip)
        return;

    auto *track = m_mixer.get_available_track();

    if (!track)
        return;

    if (!MIX_SetTrackAudio(track, clip->get_audio()))
    {
        error("MIX_SetTrackAudio() failed: {}", SDL_GetError());
        return;
    }

    MIX_SetTrackGain(track, volume);
    update_track_position(track, volume);
    MIX_PlayTrack(track, 0);
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
    update_track_position(m_track, m_volume);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_PlayTrack(m_track, props);

    SDL_DestroyProperties(props);
}

void AudioSource::stop()
{
    if (!m_track)
        return;

    MIX_StopTrack(m_track, 0);
    MIX_SetTrackAudio(m_track, nullptr);
    m_track = nullptr;
}

void AudioSource::update()
{
    update_track_position(m_track, m_volume);
}

void AudioSource::set_volume(float volume)
{
    m_volume = volume;

    if (!m_track)
        return;

    MIX_SetTrackGain(m_track, m_volume);
}

void AudioSource::set_position(const glm::vec3& position)
{
    m_position = position;
    update();
}

void AudioSource::update_track_position(MIX_Track *track, float volume)
{
    if (!track)
        return;

    const auto& listener = m_mixer.get_audio_listener();

    const glm::vec3 pos = m_position - listener.get_position();
    const float gain = glm::length2(pos) > AUDIO_MAX_DISTANCE * AUDIO_MAX_DISTANCE ? 0.0f : volume;
    MIX_SetTrackGain(track, gain);

    const glm::vec3& forward = listener.get_forward();
    const glm::vec3& up = listener.get_up();
    const glm::vec3 right = glm::cross(forward, up);

    MIX_Point3D point{
        .x = glm::dot(pos, right),
        .y = glm::dot(pos, up),
        .z = -glm::dot(pos, forward),
    };

    MIX_SetTrack3DPosition(track, &point);
}

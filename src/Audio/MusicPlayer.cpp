#include "Audio/MusicPlayer.hpp"
#include "SDL3/SDL_stdinc.h"

#include <SDL3/SDL.h>

MusicPlayer::MusicPlayer(MIX_Mixer& mixer) : m_mixer(mixer), m_audio_clips{
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Plains.wav")),
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Plains.wav")),
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Plains.wav")),
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Beach.wav")),
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Plains.wav")),
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Plains.wav")),
                                                                 AudioClip(mixer, std::filesystem::absolute("assets/audio/music/Plains.wav"))}
{
    m_tracks[0] = MIX_CreateTrack(&m_mixer);
    m_tracks[1] = MIX_CreateTrack(&m_mixer);
}

MusicPlayer::~MusicPlayer()
{
    if (m_tracks[0])
        MIX_DestroyTrack(m_tracks[0]);

    if (m_tracks[1])
        MIX_DestroyTrack(m_tracks[1]);
}

void MusicPlayer::play(AudioClip *clip, bool loop)
{
    if (!clip)
        return;

    MIX_Track *track = m_tracks[m_current_track];

    MIX_StopTrack(track, 0);

    if (!MIX_SetTrackAudio(track, clip->get_audio()))
        return;

    MIX_SetTrackGain(track, m_volume);

    SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loop ? -1 : 0);

    if (!MIX_PlayTrack(track, props))
    {
        SDL_DestroyProperties(props);
        return;
    }

    SDL_DestroyProperties(props);
    m_current_clip = clip;
}

void MusicPlayer::stop(float fade_duration)
{
    for (MIX_Track *track : m_tracks)
    {
        if (!MIX_TrackPlaying(track))
            continue;

        Sint64 fade_frames = 0;

        if (fade_duration > 0.0f)
        {
            const Sint64 fade_ms = static_cast<Sint64>(fade_duration * 1000.0f);
            fade_frames = MIX_TrackMSToFrames(track, fade_ms);
        }

        MIX_StopTrack(track, fade_frames);
    }

    m_current_clip = nullptr;
    m_next_clip = nullptr;
    m_transition_delay = 0.0f;
    m_transition_duration = 0.0f;
}

void MusicPlayer::update(float delta)
{
    if (!m_next_clip)
        return;

    m_transition_delay -= delta;

    if (m_transition_delay > 0.0f)
        return;

    AudioClip *clip = m_next_clip;
    const float duration = m_transition_duration;

    m_next_clip = nullptr;
    m_transition_delay = 0.0f;
    m_transition_duration = 0.0f;

    MIX_Track *old_track = m_tracks[m_current_track];
    MIX_Track *new_track = m_tracks[1 - m_current_track];

    MIX_StopTrack(new_track, 0);

    if (!MIX_SetTrackAudio(new_track, clip->get_audio()))
        return;

    MIX_SetTrackGain(new_track, m_volume);

    const Sint64 fade_ms = static_cast<Sint64>(duration * 1000.0f);

    SDL_PropertiesID props = SDL_CreateProperties();

    SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_START_GAIN_FLOAT, 0.0);
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fade_ms);
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);

    if (!MIX_PlayTrack(new_track, props))
    {
        SDL_DestroyProperties(props);
        return;
    }

    SDL_DestroyProperties(props);

    const Sint64 fade_frames = MIX_TrackMSToFrames(new_track, fade_ms);

    MIX_StopTrack(old_track, fade_frames);

    m_current_track = 1 - m_current_track;
    m_current_clip = clip;
}

void MusicPlayer::crossfade_to(AudioClip *clip, float delay, float duration)
{
    if (!clip || delay < 0.0f || duration <= 0.0f)
        return;

    if (!MIX_TrackPlaying(m_tracks[m_current_track]))
    {
        play(clip, true);
        return;
    }

    m_next_clip = clip;
    m_transition_delay = delay;
    m_transition_duration = duration;
}

void MusicPlayer::set_volume(float volume)
{
    m_volume = volume;

    MIX_SetTrackGain(m_tracks[m_current_track], m_volume);
    MIX_SetTrackGain(m_tracks[1 - m_current_track], m_volume);
}

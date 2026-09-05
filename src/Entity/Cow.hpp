#pragma once

#include "Audio/AudioClip.hpp"
#include "Audio/AudioSource.hpp"
#include "Entity/Entity.hpp"
#include "Mob.hpp"

class Cow : public Mob
{
public:
    Cow()
        : Mob(3)
    {
        m_aabb = AABBd(-glm::vec3(0.35, 0.9, 0.35), glm::vec3(0.35, 0.9, 0.35));
    }

    virtual void on_damage(int damage, EntityId damage_source) override;

    void start() override;
    void tick(float delta) override;
    void on_ready() override;

protected:
    void flee_from(int radius);

    std::shared_ptr<Entity> m_threat_entity;
    std::optional<AudioClip> m_walking_clip;
    std::optional<AudioClip> m_swimming_clip;
    std::optional<AudioSource> m_audio_source;
};

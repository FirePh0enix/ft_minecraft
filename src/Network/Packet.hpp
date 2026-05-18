#pragma once

#include "Core/Containers/Vector.hpp"
#include "Entity/Entity.hpp"
#include "World/Biome.hpp"
#include "World/Block.hpp"
#include "World/Chunk.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>

enum class PacketType : uint32_t
{
    /**
     * This packet is send by the server to any client connecting to send basic informations about the
     * world.
     */
    Init,
    /**
     * Send the player transform to the server.
     */
    SendPlayerTransform,
    /**
     * Send by the server to clients to indicate a new entity was added to the world.
     */
    AddEntity,
    /**
     * Send by the server to clients to indicate an entity was removed from the world.
     */
    RemoveEntity,
    /**
     * Send by the server, update the entity transform.
     */
    UpdateEntity,
    /**
     * Call a remote procedure for an entity.
     */
    RpcCall,
    /**
     * Send by the server, contains the data of a chunk.
     */
    ChunkData,
};

class DataBuffer
{
public:
    DataBuffer()
    {
    }

    DataBuffer(char *data, size_t size)
        : m_ro_data(data), m_ro_data_size(size)
    {
    }

    template <typename T>
    Result<void> write(T value)
    {
        TRY(m_data.resize(m_data.size() + sizeof(T)));
        std::memcpy(m_data.data() + m_data.size() - sizeof(T), &value, sizeof(T));
        return Result<void>();
    }

    Result<void> write(String value)
    {
        TRY(m_data.resize(m_data.size() + value.size()));
        std::memcpy(m_data.data() + m_data.size() - value.size(), value.data(), value.size());
        TRY(m_data.append(0));
        return Result<void>();
    }

    template <typename T>
    Result<void> write_array(Vector<T> vec)
    {
        const size_t byte_len = vec.size() * sizeof(T);

        TRY(m_data.resize(m_data.size() + byte_len));
        std::memcpy(m_data.data() + m_data.size() - byte_len, vec.data(), byte_len);
        return Result<void>();
    }

    template <typename T>
    Result<T> read()
    {
        T value;
        std::memcpy(&value, m_ro_data + m_cursor, sizeof(T));
        m_cursor += sizeof(T);
        return value;
    }

    Result<String> read()
    {
        size_t len = 0;
        while (len < m_ro_data_size && m_ro_data[m_cursor + len] != 0)
            len++;

        String s;
        s.resize(len);
        std::memcpy(s.data(), m_ro_data + m_cursor, len);
        m_cursor += len + 1;
        return s;
    }

    template <typename T>
    Result<Vector<T>> read_array(size_t len)
    {
        Vector<T> values;
        TRY(values.resize(len));

        const size_t byte_len = len / sizeof(T);

        ASSERT_V(m_ro_data_size - m_cursor >= byte_len, "");

        std::memcpy(values.data(), m_ro_data + m_cursor, sizeof(T) * byte_len);
        m_cursor += sizeof(T) * len;
        return values;
    }

    Vector<char> data() const { return m_data; }

private:
    Vector<char> m_data;
    char *m_ro_data = nullptr;
    size_t m_ro_data_size = 0;
    size_t m_cursor = 0;
};

struct InitPacket
{
    uint64_t seed = 0;
    EntityId id;
    glm::vec3 position;

    static constexpr PacketType type = PacketType::Init;
};
inline Result<void> serialize(DataBuffer& buffer, const InitPacket& p)
{
    TRY(buffer.write(p.seed));
    TRY(buffer.write(p.id));
    TRY(buffer.write(p.position.x));
    TRY(buffer.write(p.position.y));
    TRY(buffer.write(p.position.z));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, InitPacket& p)
{
    p.seed = TRY(buffer.read<uint64_t>());
    p.id = TRY(buffer.read<EntityId>());
    p.position = glm::vec3(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    return Result<void>();
}

struct SendPlayerTransformPacket
{
    EntityId id;
    glm::vec3 position;
    glm::quat rotation;

    static constexpr PacketType type = PacketType::SendPlayerTransform;
};
inline Result<void> serialize(DataBuffer& buffer, const SendPlayerTransformPacket& p)
{
    TRY(buffer.write(p.id));
    TRY(buffer.write(p.position.x));
    TRY(buffer.write(p.position.y));
    TRY(buffer.write(p.position.z));
    TRY(buffer.write(p.rotation.x));
    TRY(buffer.write(p.rotation.y));
    TRY(buffer.write(p.rotation.z));
    TRY(buffer.write(p.rotation.w));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, SendPlayerTransformPacket& p)
{
    p.id = TRY(buffer.read<EntityId>());
    p.position = glm::vec3(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    p.rotation = glm::quat(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    return Result<void>();
}

struct AddEntityPacket
{
    glm::vec3 position;
    glm::quat rotation;
    EntityId id;
    ClassHashCode class_id;

    static constexpr PacketType type = PacketType::AddEntity;
};
inline Result<void> serialize(DataBuffer& buffer, const AddEntityPacket& p)
{
    TRY(buffer.write(p.position.x));
    TRY(buffer.write(p.position.y));
    TRY(buffer.write(p.position.z));
    TRY(buffer.write(p.rotation.x));
    TRY(buffer.write(p.rotation.y));
    TRY(buffer.write(p.rotation.z));
    TRY(buffer.write(p.rotation.w));
    TRY(buffer.write(p.id));
    TRY(buffer.write(p.class_id));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, AddEntityPacket& p)
{
    p.position = glm::vec3(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    p.rotation = glm::quat(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    p.id = TRY(buffer.read<EntityId>());
    p.class_id = TRY(buffer.read<ClassHashCode>());
    return Result<void>();
}

struct RemoveEntityPacket
{
    EntityId id;

    static constexpr PacketType type = PacketType::RemoveEntity;
};
inline Result<void> serialize(DataBuffer& buffer, const RemoveEntityPacket& p)
{
    TRY(buffer.write(p.id));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, RemoveEntityPacket& p)
{
    p.id = TRY(buffer.read<EntityId>());
    return Result<void>();
}

struct UpdateEntityPacket
{
    EntityId id;
    glm::vec3 position;
    glm::quat rotation;

    static constexpr PacketType type = PacketType::UpdateEntity;
};
inline Result<void> serialize(DataBuffer& buffer, const UpdateEntityPacket& p)
{
    TRY(buffer.write(p.id));
    TRY(buffer.write(p.position.x));
    TRY(buffer.write(p.position.y));
    TRY(buffer.write(p.position.z));
    TRY(buffer.write(p.rotation.x));
    TRY(buffer.write(p.rotation.y));
    TRY(buffer.write(p.rotation.z));
    TRY(buffer.write(p.rotation.w));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, UpdateEntityPacket& p)
{
    p.id = TRY(buffer.read<EntityId>());
    p.position = glm::vec3(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    p.rotation = glm::quat(TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()), TRY(buffer.read<float>()));
    return Result<void>();
}

struct RpcCallPacket
{
    EntityId id;
    String name;
    std::vector<Variant> args;

    static constexpr PacketType type = PacketType::RpcCall;
};
inline Result<void> serialize(DataBuffer& buffer, const RpcCallPacket& p)
{
    TRY(buffer.write(p.id));
    TRY(buffer.write(p.name));

    nlohmann::json j = p.args;
    std::string s = j.dump();
    TRY(buffer.write(s));

    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, RpcCallPacket& p)
{
    p.id = TRY(buffer.read<EntityId>());
    p.name = TRY(buffer.read());

    String s = TRY(buffer.read());
    println("`{}`", s);
    // FIXME
    // p.args = nlohmann::json::parse(std::string(s.data(), s.size()));

    return Result<void>();
}

struct ChunkDataPacket
{
    int64_t x;
    int64_t z;
    Vector<BlockState> blocks;
    Vector<Biome> biomes;

    static constexpr PacketType type = PacketType::ChunkData;
};
inline Result<void> serialize(DataBuffer& buffer, const ChunkDataPacket& p)
{
    TRY(buffer.write(p.x));
    TRY(buffer.write(p.z));
    TRY(buffer.write_array(p.blocks));
    TRY(buffer.write_array(p.biomes));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, ChunkDataPacket& p)
{
    p.x = TRY(buffer.read<int64_t>());
    p.z = TRY(buffer.read<int64_t>());
    p.blocks = TRY(buffer.read_array<BlockState>(Chunk::block_count));
    p.biomes = TRY(buffer.read_array<Biome>(Chunk::block_count));
    return Result<void>();
}

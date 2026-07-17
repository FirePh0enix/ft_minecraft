#pragma once

#include "Block/Block.hpp"
#include "Core/Containers/Vector.hpp"
#include "Core/IO.hpp"
#include "Entity/Entity.hpp"
#include "World/Biome.hpp"
#include "World/Chunk.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>

enum class PacketType : uint32_t
{
    /**
     * Send by a client right after connecting with information about themself.
     */
    Bonjour,
    /**
     * Send by the server to indicate a client is not welcome.
     */
    Refused,
    /**
     * Send by the server to any client after receiving a Bonjour to send basic informations about the world.
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
     * Send by the client to request data for a specific chunk. The server is expected to respond with a `ChunkData` packet.
     */
    RequestChunk,
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
    void write(T value)
    {
        m_data.resize(m_data.size() + sizeof(T));
        std::memcpy(m_data.data() + m_data.size() - sizeof(T), &value, sizeof(T));
    }

    template <typename T>
    void write_array(View<T> vec)
    {
        const size_t byte_len = vec.size() * sizeof(T);

        m_data.resize(m_data.size() + byte_len);
        std::memcpy(m_data.data() + m_data.size() - byte_len, vec.data(), byte_len);
    }

    template <typename T>
    T read()
    {
        T value;
        std::memcpy(&value, m_ro_data + m_cursor, sizeof(T));
        m_cursor += sizeof(T);
        return value;
    }

    template <typename T>
    Vector<T> read_array(size_t len)
    {
        Vector<T> values;
        values.resize(len);

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

struct BonjourPacket {
    String username;

    static constexpr PacketType type = PacketType::Bonjour;
};
inline Result<void> serialize(DataBuffer& buffer, const BonjourPacket& p) {
    uint32_t size = p.username.size();
    buffer.write(size);
    buffer.write_array(View<char>(p.username.data(), p.username.size()));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, BonjourPacket& p) {
    uint32_t size = buffer.read<uint32_t>();
    Vector<char> string_buf = buffer.read_array<char>(size);
    p.username.append(string_buf.data(), string_buf.size());
    return Result<void>();
}

struct RefusedPacket {
    String message;

    static constexpr PacketType type = PacketType::Refused;
};
inline Result<void> serialize(DataBuffer& buffer, const RefusedPacket& p) {
    uint32_t size = p.message.size();
    buffer.write(size);
    buffer.write_array(View<char>(p.message.data(), p.message.size()));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, RefusedPacket& p) {
    uint32_t size = buffer.read<uint32_t>();
    Vector<char> string_buf = buffer.read_array<char>(size);
    p.message.append(string_buf.data(), string_buf.size());
    return Result<void>();
}

struct InitPacket
{
    uint64_t seed = 0;
    EntityId id;
    glm::vec3 position;

    static constexpr PacketType type = PacketType::Init;
};
inline Result<void> serialize(DataBuffer& buffer, const InitPacket& p)
{
    buffer.write(p.seed);
    buffer.write(p.id);
    buffer.write(p.position.x);
    buffer.write(p.position.y);
    buffer.write(p.position.z);
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, InitPacket& p)
{
    p.seed = buffer.read<uint64_t>();
    p.id = buffer.read<EntityId>();
    p.position = glm::vec3(buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
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
    buffer.write(p.id);
    buffer.write(p.position.x);
    buffer.write(p.position.y);
    buffer.write(p.position.z);
    buffer.write(p.rotation.x);
    buffer.write(p.rotation.y);
    buffer.write(p.rotation.z);
    buffer.write(p.rotation.w);
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, SendPlayerTransformPacket& p)
{
    p.id = buffer.read<EntityId>();
    p.position = glm::vec3(buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
    p.rotation = glm::quat(buffer.read<float>(), buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
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
    buffer.write(p.position.x);
    buffer.write(p.position.y);
    buffer.write(p.position.z);
    buffer.write(p.rotation.x);
    buffer.write(p.rotation.y);
    buffer.write(p.rotation.z);
    buffer.write(p.rotation.w);
    buffer.write(p.id);
    buffer.write(p.class_id);
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, AddEntityPacket& p)
{
    p.position = glm::vec3(buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
    p.rotation = glm::quat(buffer.read<float>(), buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
    p.id = buffer.read<EntityId>();
    p.class_id = buffer.read<ClassHashCode>();
    return Result<void>();
}

struct RemoveEntityPacket
{
    EntityId id;

    static constexpr PacketType type = PacketType::RemoveEntity;
};
inline Result<void> serialize(DataBuffer& buffer, const RemoveEntityPacket& p)
{
    buffer.write(p.id);
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, RemoveEntityPacket& p)
{
    p.id = buffer.read<EntityId>();
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
    buffer.write(p.id);
    buffer.write(p.position.x);
    buffer.write(p.position.y);
    buffer.write(p.position.z);
    buffer.write(p.rotation.x);
    buffer.write(p.rotation.y);
    buffer.write(p.rotation.z);
    buffer.write(p.rotation.w);
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, UpdateEntityPacket& p)
{
    p.id = buffer.read<EntityId>();
    p.position = glm::vec3(buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
    p.rotation = glm::quat(buffer.read<float>(), buffer.read<float>(), buffer.read<float>(), buffer.read<float>());
    return Result<void>();
}

struct RpcCallPacket
{
    EntityId id;
    String name;
    Vector<Variant> args;

    static constexpr PacketType type = PacketType::RpcCall;
};
inline Result<void> serialize(DataBuffer& buffer, const RpcCallPacket& p)
{
    buffer.write(p.id);

    uint32_t name_size = p.name.size();
    buffer.write(name_size);
    buffer.write_array(View(p.name.data(), p.name.size()));

    BufferWriter writer;
    for (const Variant& v : p.args) {
	TRY(writer.write_variant(v));
    }

    uint32_t size = writer.buffer().size();
    buffer.write(size);
    buffer.write_array(writer.buffer());

    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, RpcCallPacket& p)
{
    p.id = buffer.read<EntityId>();

    uint32_t name_size = buffer.read<uint32_t>();
    Vector<uint8_t> name_vec = buffer.read_array<uint8_t>(name_size);

    p.name.append((char *)name_vec.data(), name_vec.size());

    uint32_t size = buffer.read<uint32_t>();
    Vector<uint8_t> data = buffer.read_array<uint8_t>(size);

    BufferReader reader(data.data(), data.size());
    for (;;) {
	Option<Variant> v = TRY(reader.read_variant());
	if (v.has_value()) {
	    p.args.append(v.value());
	} else {
	    break;
	}
    }

    return Result<void>();
}

struct RequestChunkPacket
{
    int64_t x;
    int64_t z;

    static constexpr PacketType type = PacketType::RequestChunk;
};
inline Result<void> serialize(DataBuffer& buffer, const RequestChunkPacket& p)
{
    buffer.write(p.x);
    buffer.write(p.z);
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, RequestChunkPacket& p)
{
    p.x = buffer.read<int64_t>();
    p.z = buffer.read<int64_t>();
    return Result<void>();
}

struct ChunkDataPacket
{
    int64_t x;
    int64_t z;
    Vector<uint8_t> blocks;
    Vector<uint8_t> tags;

    static constexpr PacketType type = PacketType::ChunkData;
};
inline Result<void> serialize(DataBuffer& buffer, const ChunkDataPacket& p)
{
    buffer.write(p.x);
    buffer.write(p.z);

    uint32_t size = p.blocks.size();
    buffer.write(size);

    buffer.write_array(View(p.blocks));
    
    size = p.tags.size();
    buffer.write(size);

    buffer.write_array(View(p.tags));
    return Result<void>();
}
inline Result<void> deserialize(DataBuffer& buffer, ChunkDataPacket& p)
{
    p.x = buffer.read<int64_t>();
    p.z = buffer.read<int64_t>();

    uint32_t size = buffer.read<uint32_t>();
    p.blocks = buffer.read_array<uint8_t>(size);

    size = buffer.read<uint32_t>();
    p.tags = buffer.read_array<uint8_t>(size);    
    return Result<void>();
}

#include "World/Save.hpp"
#include "Core/Filesystem.hpp"
#include "World/Registry.hpp"

#include <filesystem>
#include <fstream>

Save::Save(std::string name)
    : m_name(name)
{
}

void Save::save_info(const Ref<World>& world)
{
    const std::string path = get_path();
    std::filesystem::create_directories(path);

    const std::string chunks_path = get_path() + "/chunks";
    std::filesystem::create_directories(chunks_path);

    std::ofstream os(path + "/world.dat", std::ios::binary);
    ERR_COND_VR(!os.is_open(), "Failed to open {}/world.dat", m_name);

    std::vector<uint32_t> encoded_string_table;
    uint32_t accumulated_size = 0;
    uint32_t block_id = 0;
    for (const auto& block : BlockRegistry::get_blocks())
    {
        encoded_string_table.push_back(accumulated_size);
        block_id += 1;
        accumulated_size += block->name().size() + 1;
    }

    // write the file header.
    SavedWorldHeader header{};
    header.magic = saved_world_magic;
    header.version = 1;
    header.block_table_offset = sizeof(SavedWorldHeader);
    header.block_table_count = BlockRegistry::get_block_count();
    header.string_table_offset = sizeof(SavedWorldHeader) + sizeof(uint32_t) * BlockRegistry::get_block_count();
    os.write((char *)&header, sizeof(SavedWorldHeader));

    // write the block table
    os.write((char *)encoded_string_table.data(), (ssize_t)(encoded_string_table.size() * sizeof(uint32_t)));

    // write the string table
    for (const auto& block : BlockRegistry::get_blocks())
    {
        os.write(block->name().c_str(), (ssize_t)block->name().size() + 1); // copy also the null byte
    }
}

struct __attribute__((packed)) PosStatePair
{
    uint16_t pos;
    uint32_t state;
};

void Save::save_chunk(const Ref<Chunk>& chunk)
{
    const std::string path = get_path() + "/chunks/" + std::to_string(chunk->x()) + "$" + std::to_string(chunk->z()) + ".chunkdat";

    std::ofstream os(path, std::ios::binary);
    ERR_COND_VR(!os.is_open(), "Failed to open {}/chunks/{}${}.chunkdat", m_name, chunk->x(), chunk->z());

    SavedChunkHeader header{};
    header.magic = saved_chunk_magic;
    header.version = 1;

    std::vector<PosStatePair> blocks;

    for (size_t x = 0; x < Chunk::width; x++)
    {
        for (size_t y = 0; y < Chunk::height; y++)
        {
            for (size_t z = 0; z < Chunk::width; z++)
            {
                uint16_t pos = CHUNK_POS(x, y, z);
                BlockState state = chunk->get_block(x, y, z);

                if (state.is_air())
                    continue;

                blocks.push_back(PosStatePair{.pos = pos, .state = *(uint32_t *)&state});
            }
        }
    }

    header.blocks_count = blocks.size();
    os.write((char *)&header, sizeof(SavedChunkHeader));
    os.write((char *)blocks.data(), (ssize_t)(sizeof(PosStatePair) * blocks.size()));
}

bool Save::chunk_exists(int64_t x, int64_t z)
{
    const std::string path = get_path() + "/chunks/" + std::to_string(x) + "$" + std::to_string(z) + ".chunkdat";
    return std::filesystem::exists(path);
}

Ref<Chunk> Save::load_chunk(int64_t x, int64_t z)
{
    const std::string path = get_path() + "/chunks/" + std::to_string(x) + "$" + std::to_string(z) + ".chunkdat";
    std::ifstream is(path);

    ERR_COND_V(!is.is_open(), "Failed to open {}/chunks/{}${}.chunkdat", m_name, x, z);

    Ref<Chunk> chunk = make_ref<Chunk>(x, z);

    if (!is.is_open())
    {
        return chunk;
    }

    SavedChunkHeader header;
    is.read((char *)&header, sizeof(SavedChunkHeader));

    std::vector<PosStatePair> blocks;
    blocks.resize(header.blocks_count);
    is.read((char *)blocks.data(), (ssize_t)(sizeof(PosStatePair) * header.blocks_count));

    for (const auto& pair : blocks)
    {
        uint32_t state = pair.state;
        chunk->set_block(CHUNK_LOCAL_X(pair.pos), CHUNK_LOCAL_Y(pair.pos), CHUNK_LOCAL_Z(pair.pos), *(BlockState *)&state);
    }

    return chunk;
}

std::string Save::get_path()
{
    return get_data_directory() + "/" + std::string(saves_path) + "/" + m_name;
}

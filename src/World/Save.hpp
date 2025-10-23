#pragma once

#include "World/Chunk.hpp"

constexpr uint32_t saved_world_magic = 0x57524C44;
constexpr uint32_t saved_chunk_magic = 0x4B4E4843;

constexpr uint32_t saved_current_version = 1;

struct SavedWorldHeader
{
    uint32_t magic = saved_world_magic;
    uint32_t version = saved_current_version;

    // The block_table is a table of uint32_t indexed by block ids. The `uint32_t` is an offset relative
    // to the string_table.
    uint32_t block_table_offset = 0;
    uint32_t block_table_count = 0;

    // Offset of the string table.
    uint32_t string_table_offset = 0;
};

struct SavedChunkHeader
{
    uint32_t magic = saved_chunk_magic;
    uint32_t version = saved_current_version;

    /**
     * The number of blocks saved.
     */
    uint16_t blocks_count = 0;
};

class Save
{
public:
    static constexpr const char *saves_path = "saves/";

    Save(std::string name);

    /**
     * @brief Save world info and block table to the disk.
     */
    void save_info(const Ref<World>& world);

    /**
     * @brief Save a chunk of world to the disk.
     * @param chunk The chunk to save.
     */
    void save_chunk(const Ref<Chunk>& chunk);

    /**
     * @brief Returns whether a chunk exists on disk or not.
     */
    bool chunk_exists(int64_t x, int64_t z);

    /**
     * @brief Load a chunk from disk.
     */
    Ref<Chunk> load_chunk(int64_t x, int64_t z);

private:
    std::string m_name;

    std::string get_path();
};

#include "Network/Packet.hpp"

#include "Item/ItemStack.hpp"

std::expected<void, Error> serialize(DataBuffer& buffer, const SyncInventory& p)
{
    BufferWriter writer;
    for (const ItemStack& v : p.items)
    {
        TRY(writer.write_variant(Variant(v)));
    }

    uint32_t size = writer.buffer().size();
    buffer.write(size);
    buffer.write_array(writer.buffer());

    return std::expected<void, Error>();
}
std::expected<void, Error> deserialize(DataBuffer& buffer, SyncInventory& p)
{
    uint32_t size = buffer.read<uint32_t>();
    std::vector<uint8_t> data = buffer.read_array<uint8_t>(size);

    BufferReader reader(data.data(), data.size());
    for (;;)
    {
        std::optional<Variant> v = TRY(reader.read_variant());
        if (v.has_value())
            p.items.push_back(v.value().get<ItemStack>());
        else
            break;
    }

    return std::expected<void, Error>();
}

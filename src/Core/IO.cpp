#include "Core/IO.hpp"

#include "Core/Error.hpp"
#include "Engine.hpp"
#include "Item/ItemStack.hpp"
#include "Variant.hpp"

#include <optional>

Result<std::optional<Variant>> Reader::read_variant()
{
    uint32_t type_raw;
    size_t n = TRY(read_raw(&type_raw, sizeof(uint32_t)));
    if (n == 0)
        return Result<std::optional<Variant>>(std::nullopt);

    VariantType type = VariantType((uint8_t)type_raw);
    if (type == VariantType::Null)
    {
        return std::make_optional(Variant(nullptr));
    }
    else if (type == VariantType::Bool)
    {
        uint32_t b;
        TRY(read_raw(&b, sizeof(uint32_t)));
        return std::make_optional(Variant(bool(b)));
    }
    else if (type == VariantType::Double)
    {
        double d;
        TRY(read_raw(&d, sizeof(double)));
        return std::make_optional(Variant(d));
    }
    else if (type == VariantType::Integer)
    {
        int64_t i;
        TRY(read_raw(&i, sizeof(int64_t)));
        return std::make_optional(Variant(i));
    }
    else if (type == VariantType::String)
    {
        uint32_t size;
        TRY(read_raw(&size, sizeof(uint32_t)));

        std::string s;
        s.resize(size);

        TRY(read_raw(s.data(), size));

        size_t aligned_size = ((size - 1) / 4 + 1) * 4;
        char buf[4]{0};

        if (size != aligned_size)
            TRY(read_raw(buf, aligned_size - size));

        return std::make_optional(Variant(s));
    }
    else if (type == VariantType::Vec2)
    {
        double f[2];
        TRY(read_raw(f, sizeof(f)));
        return std::make_optional(Variant(glm::dvec2(f[0], f[1])));
    }
    else if (type == VariantType::Vec3)
    {
        double f[3];
        TRY(read_raw(f, sizeof(f)));
        return std::make_optional(Variant(glm::dvec3(f[0], f[1], f[2])));
    }
    else if (type == VariantType::Quat)
    {
        double f[4];
        TRY(read_raw(f, sizeof(f)));
        return std::make_optional(Variant(glm::dquat(f[0], f[1], f[2], f[3])));
    }
    else if (type == VariantType::ItemStack)
    {
        uint32_t size;

        TRY(read_raw(&size, sizeof(uint32_t)));

        uint32_t name_length = 0;
        TRY(read_raw(&name_length, sizeof(uint32_t)));

        char *name = (char *)alloca(name_length + 1);
        name[name_length] = 0;
        TRY(read_raw(name, name_length));

        std::optional<Variant> variant = TRY(read_variant());
        Variant v = variant.value();

        std::map<std::string, Variant> map = v.to_map<std::string, Variant>();

        Id<Item> item = Engine::get().registry().item_from_name(name);

        return std::make_optional(Variant(ItemStack(item, size, map)));
    }
    else if (type == VariantType::Array)
    {
        std::vector<Variant> array;

        uint32_t size;
        TRY(read_raw(&size, sizeof(uint32_t)));

        array.reserve(size);

        for (size_t i = 0; i < size; i++)
        {
            Variant variant = TRY(read_variant()).value();
            array.push_back(variant);
        }

        return std::make_optional(Variant(array));
    }
    else if (type == VariantType::Map)
    {
        std::map<Variant, Variant> map;

        uint32_t size;
        TRY(read_raw(&size, sizeof(uint32_t)));

        for (size_t i = 0; i < size; i++)
        {
            Variant key = TRY(read_variant()).value();
            Variant value = TRY(read_variant()).value();

            map[key] = value;
        }

        return std::make_optional(Variant(map));
    }

    println("{}", type_raw);
    return Error(ErrorKind::ReadFailure);
}

Result<void> Reader::read_to_buffer(std::vector<char>& buffer)
{
    buffer.resize(size());
    TRY(read_raw(buffer.data(), buffer.size()));
    return Result<void>();
}

Result<std::string> Reader::read_to_string()
{
    std::string str;
    str.resize(size());
    TRY(read_raw(str.data(), str.size()));
    return str;
}

Result<void> Writer::write_variant(const Variant& variant)
{
    uint32_t type = uint32_t(variant.tag);
    TRY(write_raw(&type, sizeof(uint32_t)));

    if (variant.has(VariantType::Null))
    {
        return Result<void>();
    }

    if (variant.has(VariantType::Bool))
    {
        uint32_t b = variant.get_unchecked<bool>();
        TRY(write_raw(&b, sizeof(uint32_t)));
    }
    else if (variant.has(VariantType::Double))
    {
        double d = variant.get_unchecked<double>();
        TRY(write_raw(&d, sizeof(double)));
    }
    else if (variant.has(VariantType::Integer))
    {
        int64_t d = variant.get_unchecked<int64_t>();
        TRY(write_raw(&d, sizeof(int64_t)));
    }
    else if (variant.has(VariantType::String))
    {
        const std::string& s = variant.get_unchecked<std::string>();
        const uint32_t size = s.size();
        TRY(write_raw(&size, sizeof(uint32_t)));
        TRY(write_raw(s.data(), s.size()));

        char buf[4]{0};
        size_t aligned_size = ((size - 1) / 4 + 1) * 4;
        if (size != aligned_size)
            TRY(write_raw(buf, aligned_size - size));
    }
    else if (variant.has(VariantType::Vec2))
    {
        glm::dvec2 d = variant.get_unchecked<glm::dvec2>();
        TRY(write_raw(&d.x, sizeof(double)));
        TRY(write_raw(&d.y, sizeof(double)));
    }
    else if (variant.has(VariantType::Vec3))
    {
        glm::dvec3 d = variant.get_unchecked<glm::dvec3>();
        TRY(write_raw(&d.x, sizeof(double)));
        TRY(write_raw(&d.y, sizeof(double)));
        TRY(write_raw(&d.z, sizeof(double)));
    }
    else if (variant.has(VariantType::Quat))
    {
        glm::dquat d = variant.get_unchecked<glm::dquat>();
        TRY(write_raw(&d.w, sizeof(double)));
        TRY(write_raw(&d.x, sizeof(double)));
        TRY(write_raw(&d.y, sizeof(double)));
        TRY(write_raw(&d.z, sizeof(double)));
    }
    else if (variant.has(VariantType::ItemStack))
    {
        ItemStack stack = variant.get_unchecked<ItemStack>();
        uint32_t size = stack.count();
        const char *block_id = stack.item().str;

        TRY(write_raw(&size, sizeof(uint32_t)));

        uint32_t name_length = strlen(block_id);
        TRY(write_raw(&name_length, sizeof(uint32_t)));
        TRY(write_raw(block_id, name_length));

        Variant variant(stack.get_tags());
        TRY(write_variant(variant));
    }
    else if (variant.has(VariantType::Array))
    {
        const std::vector<Variant>& array = variant.get_unchecked<std::vector<Variant>>();
        uint32_t size = array.size();

        TRY(write_raw(&size, sizeof(uint32_t)));
        for (const auto& value : array)
            TRY(write_variant(value));
    }
    else if (variant.has(VariantType::Map))
    {
        const std::map<Variant, Variant>& map = variant.get_unchecked<std::map<Variant, Variant>>();
        uint32_t size = map.size();

        TRY(write_raw(&size, sizeof(uint32_t)));
        for (const auto& [key, value] : map)
        {
            TRY(write_variant(key));
            TRY(write_variant(value));
        }
    }

    return Result<void>();
}

Result<size_t> BufferReader::read_raw(void *buf, size_t size)
{
    size_t remaining_size = std::min(size, m_size - m_cursor);
    memcpy(buf, m_buffer + m_cursor, remaining_size);
    m_cursor += size;
    return remaining_size;
}

size_t BufferReader::size()
{
    return m_size;
}

bool BufferReader::eof()
{
    return m_cursor >= m_size;
}

Result<size_t> BufferWriter::write_raw(const void *buf, size_t size)
{
    size_t prev_size = m_buffer.size();
    m_buffer.resize(prev_size + size);
    memcpy(m_buffer.data() + prev_size, buf, size);
    return size;
}

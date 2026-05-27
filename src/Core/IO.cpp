#include "Core/IO.hpp"
#include "Core/Error.hpp"
#include "Variant.hpp"

Result<Option<Variant>> Reader::read_variant()
{
    uint32_t type_raw;
    size_t n = TRY(read_raw(&type_raw, sizeof(uint32_t)));
    if (n == 0)
        return Option<Variant>(None);

    VariantType type = VariantType((uint8_t)type_raw);
    if (type == VariantType::Null)
    {
        return Option(Variant(nullptr));
    }
    else if (type == VariantType::Bool)
    {
        uint32_t b;
        TRY(read_raw(&b, sizeof(uint32_t)));
        return Option(Variant(bool(b)));
    }
    else if (type == VariantType::Double)
    {
        double d;
        TRY(read_raw(&d, sizeof(double)));
        return Option(Variant(d));
    }
    else if (type == VariantType::Integer)
    {
        int64_t i;
        TRY(read_raw(&i, sizeof(int64_t)));
        return Option(Variant(i));
    }
    else if (type == VariantType::String)
    {
        uint32_t size;
        TRY(read_raw(&size, sizeof(uint32_t)));

        String s;
        s.resize(size);

        TRY(read_raw(s.data(), size));

        size_t aligned_size = ((size - 1) / 4 + 1) * 4;
        char buf[4]{0};

        if (size != aligned_size)
            TRY(read_raw(buf, aligned_size - size));

        return Option(Variant(s));
    }
    else if (type == VariantType::Vec2)
    {
        float f[2];
        TRY(read_raw(f, sizeof(f)));
        return Option(Variant(glm::vec2(f[0], f[1])));
    }
    else if (type == VariantType::Vec3)
    {
        float f[3];
        TRY(read_raw(f, sizeof(f)));
        return Option(Variant(glm::vec3(f[0], f[1], f[2])));
    }
    else if (type == VariantType::Quat)
    {
        float f[4];
        TRY(read_raw(f, sizeof(f)));
        return Option(Variant(glm::quat(f[0], f[1], f[2], f[3])));
    }
    else if (type == VariantType::ItemStack)
    {
        uint32_t size;
        uint32_t block_id;

        TRY(read_raw(&size, sizeof(uint32_t)));
        TRY(read_raw(&block_id, sizeof(uint32_t)));

        return Option(Variant(ItemStack(block_id, size)));
    }

    println("{}", type_raw);
    return Error(ErrorKind::ReadFailure);
}

Result<void> Reader::read_to_buffer(LocalVector<char>& buffer)
{
    TRY(buffer.resize(size()));
    TRY(read_raw(buffer.data(), buffer.size()));
    return Result<void>();
}

Result<String> Reader::read_to_string()
{
    String str;
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
        const String& s = variant.get_unchecked<String>();
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
        glm::vec2 d = variant.get_unchecked<glm::vec2>();
        TRY(write_raw(&d.x, sizeof(float)));
        TRY(write_raw(&d.y, sizeof(float)));
    }
    else if (variant.has(VariantType::Vec3))
    {
        glm::vec3 d = variant.get_unchecked<glm::vec3>();
        TRY(write_raw(&d.x, sizeof(float)));
        TRY(write_raw(&d.y, sizeof(float)));
        TRY(write_raw(&d.z, sizeof(float)));
    }
    else if (variant.has(VariantType::Quat))
    {
        glm::quat d = variant.get_unchecked<glm::quat>();
        TRY(write_raw(&d.w, sizeof(float)));
        TRY(write_raw(&d.x, sizeof(float)));
        TRY(write_raw(&d.y, sizeof(float)));
        TRY(write_raw(&d.z, sizeof(float)));
    }
    else if (variant.has(VariantType::ItemStack))
    {
        ItemStack stack = variant.get_unchecked<ItemStack>();
        uint32_t size = stack.count();
        uint32_t block_id = stack.get_block();

        TRY(write_raw(&size, sizeof(uint32_t)));
        TRY(write_raw(&block_id, sizeof(uint32_t)));
    }

    return Result<void>();
}

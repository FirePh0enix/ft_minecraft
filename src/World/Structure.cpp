#include "World/Structure.hpp"

#include "Core/Alloc.hpp"
#include "Engine.hpp"

std::shared_ptr<Structure> Structure::load(const StringView& path)
{
    // try
    // {
    std::shared_ptr<Structure> self = std::make_shared<Structure>();
    YAML::Node config = YAML::LoadFile(path.data());

    int64_t w = config["metadata"]["size"]["width"].as<int64_t>();
    int64_t h = config["metadata"]["size"]["height"].as<int64_t>();
    int64_t l = config["metadata"]["size"]["length"].as<int64_t>();
    const size_t count = w * h * l;

    debug("w = {}", w);
    debug("h = {}", h);
    debug("l = {}", l);

    self->m_width = w;
    self->m_height = h;
    self->m_length = l;

    std::map<std::string, std::string> map;

    const YAML::Node& block_node = config["blocks"];
    for (const auto& a : block_node)
    {
        std::string name = a.first.as<std::string>();
        std::string block_name = a.second.as<std::string>();
        map[name] = block_name;
    }

    self->m_blocks = alloc_array_uninitialized<BlockState>(count);

    const YAML::Node& data = config["data"];
    size_t y = 0;
    for (auto ynode : data)
    {
        ASSERT_V(y < h, "y is {} but height is {}", y, h);
        size_t x = 0;
        std::vector<std::string> lines = ynode.second.as<std::vector<std::string>>();
        for (auto line : lines)
        {
            ASSERT_V(x < w, "x is {} but width is {}", x, w);
            size_t z = 0;
            for (char c : line)
            {
                ASSERT_V(z < l, "z is {} but length is {}", z, l);
                std::string s;
                s.push_back(c);

                std::string name = map.at(s);
                String s2;
                s2.append(name.data(), name.size());
                if (name == "air")
                {
                    self->m_blocks[x + y * w + z * w * h] = BlockState();
                }
                else
                {
                    Id<Block> block = Engine::get().registry().item_from_name(s2).value();
                    self->m_blocks[x + y * w + z * w * h] = BlockState(block);
                }

                z++;
            }
            x++;
        }
        y++;
    }

    return self;
    // }
    // catch (std::exception& ex)
    // {
    //     debug("waaaaaaaa: {}", ex.what());
    //     return nullptr;
    // }
}

#include "World/Structure.hpp"

#include "Engine.hpp"

Structure::~Structure()
{
    delete[] m_blocks;
}

std::shared_ptr<Structure> Structure::load(std::string_view path)
{
    // try
    // {
    std::shared_ptr<Structure> self = std::make_shared<Structure>();
    YAML::Node config = YAML::LoadFile(path.data());

    int64_t w = config["metadata"]["size"]["width"].as<int64_t>();
    int64_t h = config["metadata"]["size"]["height"].as<int64_t>();
    int64_t l = config["metadata"]["size"]["length"].as<int64_t>();
    const size_t count = w * h * l;

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

    self->m_blocks = new BlockState[count]();

    const YAML::Node& data = config["data"];
    int64_t y = 0;
    for (auto ynode : data)
    {
        ASSERT_V(y < h, "y is {} but height is {}", y, h);
        int64_t x = 0;
        std::vector<std::string> lines = ynode.second.as<std::vector<std::string>>();
        for (auto line : lines)
        {
            ASSERT_V(x < w, "x is {} but width is {}", x, w);
            int64_t z = 0;
            for (char c : line)
            {
                ASSERT_V(z < l, "z is {} but length is {}", z, l);
                std::string s;
                s.push_back(c);

                std::string name = map.at(s);
                if (name == "air")
                {
                    self->m_blocks[x + y * w + z * w * h] = BlockState();
                }
                else
                {
                    self->m_blocks[x + y * w + z * w * h] = BlockState(Engine::get().registry().get_block_id(name).hash);
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

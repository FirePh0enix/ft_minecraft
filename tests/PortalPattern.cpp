// Standalone regression test: c++ -std=c++17 -Isrc tests/PortalPattern.cpp -o /tmp/portal-pattern-test
#include "Block/PortalPattern.hpp"
#include <cassert>
#include <map>
#include <tuple>

int main()
{
    using Pos = std::tuple<int64_t, int64_t, int64_t>;
    for (bool along_z : {false, true})
    {
        std::map<Pos, uint32_t> blocks;
        auto pos = [&](int x, int y) { return Pos{-17 + (along_z ? 0 : x), 40 + y, 15 + (along_z ? x : 0)}; };
        auto read = [&](int64_t x, int64_t y, int64_t z) {
            auto it = blocks.find({x, y, z});
            return it == blocks.end() ? 0u : it->second;
        };
        auto find = [&](int x, int y, bool active) {
            const auto [px, py, pz] = pos(x, y);
            return PortalPattern::find(px, py, pz, 1, 2, active, read);
        };
        blocks[pos(1, 1)] = 2;
        assert(!find(1, 1, true)); // A lone portal cell is never active.
        blocks.clear();
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 4; ++x)
                if ((x == 0 || x == 3) != (y == 0 || y == 4))
                    blocks[pos(x, y)] = 1;
        for (const auto& [p, block] : blocks)
        {
            const auto [x, y, z] = p;
            assert(PortalPattern::find(x, y, z, 1, 2, false, read));
        }
        assert(find(1, 1, false));
        assert(!find(1, 1, true)); // Empty opening is not yet active.
        blocks[pos(2, 2)] = 3;
        assert(!find(1, 1, false)); // Obstructed opening.
        blocks.erase(pos(2, 2));
        for (int y = 1; y < 4; ++y)
            for (int x = 1; x < 3; ++x)
                blocks[pos(x, y)] = 2;
        for (int y = 1; y < 4; ++y)
            for (int x = 1; x < 3; ++x)
                assert(find(x, y, true));
        blocks.erase(pos(2, 4));
        assert(!find(1, 1, false)); // Missing top frame block.
        assert(!find(1, 1, true));
        blocks[pos(2, 4)] = 1;
        blocks.erase(pos(2, 2));
        assert(!find(1, 1, true)); // Partial portal surface.
        blocks[pos(2, 2)] = 2;
        blocks[pos(0, 2)] = 3;
        assert(!find(1, 1, true)); // Wrong frame material.
    }
}

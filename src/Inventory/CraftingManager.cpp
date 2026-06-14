#include "CraftingManager.hpp"
#include "Id.hpp"
#include "World/Registry.hpp"

constexpr int two_d_to_1d(int x, int y, int w)
{
    return y * w + x;
}

Option<ItemStack> CraftingManager::match(const InplaceVector<Id<Item>, 9>& grid, int width, int height) 
{
    for (const auto& r : m_recipes)
    {
        // Skip recipes that don't fit in crafting grid. Ex: Inventory will have a limit of 2x2.
        if (r.width > width || r.height > height)
            continue;

        for (int off_y = 0; off_y <= height - r.height; off_y++)
        {
            for (int off_x = 0; off_x <= width - r.width; off_x++)
            {
                bool ok = true;
                for (int y = 0; y < r.height && ok; y++)
                {
                    // Try all possible positions where the recipe could fit.
                    for (int x = 0; x < r.width; x++)
                    {
                        Id<Item> a = grid[two_d_to_1d(off_x + x, off_y + y, 3)];
                        Id<Item> b = r.pattern[two_d_to_1d(x, y, 3)];

                        if (a != b)
                        {
                            ok = false;
                            break;
                        }
                    }
                }

                if (!ok)
                    continue;

                // Ensure there is no extra item.
                for (int y = 0; y < height && ok; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        bool inside = y >= off_y && y < off_y + r.height && x >= off_x && x < off_x + r.width;

                        if (!inside && grid[two_d_to_1d(x, y, width)] != Items::none)
                        {
                            ok = false;
                            break;
                        }
                    }
                }

                if (ok)
                    return r.result;
            }
        }
    }

    return None;
}
#pragma once

#include "Core/Containers/InplaceVector.hpp"
#include "Item/Item.hpp"
#include "Item/ItemStack.hpp"

struct Recipe
{
    uint8_t width = 3;
    uint8_t height = 3;

    InplaceVector<Id<Item>, 9> pattern;
    ItemStack result;
};

class CraftingManager
{
public:
    void add_recipe(const Recipe& recipe) { m_recipes.append(recipe); }

    Option<ItemStack> match(const InplaceVector<Id<Item>, 9>& grid, int width, int height);

private:
    Vector<Recipe> m_recipes;
};
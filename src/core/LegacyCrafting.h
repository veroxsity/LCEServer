#pragma once

#include "Types.h"

namespace LCEServer
{
    struct CraftIngredientSpec
    {
        int id = -1;
        int count = 0;
        int aux = 0;
        bool anyAux = false;
    };

    struct CraftRecipeSpec
    {
        int recipeIndex = -1;
        ItemInstanceData result;
        std::vector<CraftIngredientSpec> ingredients;
    };

    const CraftRecipeSpec* FindLegacyCraftingRecipe(int recipeIndex);
}

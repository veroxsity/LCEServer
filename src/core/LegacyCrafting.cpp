#include "LegacyCrafting.h"
#include "../access/JsonUtil.h"
#include "EmbeddedRecipes.h"
#include "Logger.h"
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace LCEServer
{
    namespace
    {
        constexpr int kAnyAuxValue = -1;

        std::once_flag g_recipeLoadOnce;
        std::unordered_map<int, CraftRecipeSpec> g_recipeMap;
        bool g_recipeLoadFailed = false;

        std::string TrimCopy(const std::string& value)
        {
            return JsonUtil::Trim(value);
        }

        bool ParseIntValue(const std::string& text, int& outValue)
        {
            const std::string trimmed = TrimCopy(text);
            if (trimmed.empty())
                return false;

            const char* begin = trimmed.c_str();
            char* end = nullptr;
            errno = 0;
            const long parsed = std::strtol(begin, &end, 10);
            if (begin == end || *end != '\0' || errno == ERANGE)
                return false;
            if (parsed < (std::numeric_limits<int>::min)() ||
                parsed > (std::numeric_limits<int>::max)())
            {
                return false;
            }

            outValue = static_cast<int>(parsed);
            return true;
        }

        bool ParseItemToken(const std::string& token, ItemInstanceData& outItem)
        {
            size_t first = token.find(':');
            size_t second = (first == std::string::npos) ? std::string::npos : token.find(':', first + 1);
            if (first == std::string::npos || second == std::string::npos)
                return false;

            int id = -1;
            int aux = 0;
            int count = 0;
            if (!ParseIntValue(TrimCopy(token.substr(0, first)), id) ||
                !ParseIntValue(TrimCopy(token.substr(first + 1, second - first - 1)), aux) ||
                !ParseIntValue(TrimCopy(token.substr(second + 1)), count))
            {
                return false;
            }

            if (id < 0 || count <= 0 || count > 255)
                return false;

            outItem.id = static_cast<int16_t>(id);
            outItem.aux = static_cast<int16_t>(aux);
            outItem.count = static_cast<uint8_t>(count);
            return true;
        }

        bool ParseIngredientToken(const std::string& token, CraftIngredientSpec& outIngredient)
        {
            size_t first = token.find(':');
            size_t second = (first == std::string::npos) ? std::string::npos : token.find(':', first + 1);
            if (first == std::string::npos || second == std::string::npos)
                return false;

            int id = -1;
            int count = 0;
            const std::string idPart = TrimCopy(token.substr(0, first));
            const std::string auxPart = TrimCopy(token.substr(first + 1, second - first - 1));
            const std::string countPart = TrimCopy(token.substr(second + 1));

            if (!ParseIntValue(idPart, id) || !ParseIntValue(countPart, count))
                return false;
            if (id < 0 || count <= 0)
                return false;

            outIngredient.id = id;
            outIngredient.count = count;
            if (auxPart == "*")
            {
                outIngredient.anyAux = true;
                outIngredient.aux = kAnyAuxValue;
            }
            else
            {
                int aux = 0;
                if (!ParseIntValue(auxPart, aux))
                    return false;
                outIngredient.anyAux = false;
                outIngredient.aux = aux;
            }
            return true;
        }

        bool ParseIngredientsField(
            const std::string& text,
            std::vector<CraftIngredientSpec>& outIngredients)
        {
            outIngredients.clear();
            if (TrimCopy(text).empty())
                return true;

            size_t start = 0;
            while (start <= text.size())
            {
                size_t end = text.find('|', start);
                std::string token = (end == std::string::npos)
                    ? text.substr(start)
                    : text.substr(start, end - start);
                token = TrimCopy(token);

                if (!token.empty())
                {
                    CraftIngredientSpec ingredient;
                    if (!ParseIngredientToken(token, ingredient))
                        return false;
                    outIngredients.push_back(ingredient);
                }

                if (end == std::string::npos)
                    break;
                start = end + 1;
            }

            return true;
        }

        bool LoadEmbeddedRecipes()
        {
            std::vector<JsonUtil::JsonObject> rawRecipes;
            const std::string embeddedJson = EmbeddedRecipes::BuildInventoryRecipesJson();
            if (!JsonUtil::LoadJsonArrayFromString(embeddedJson, rawRecipes))
            {
                Logger::Error("Crafting", "Failed to parse embedded recipe manifest");
                g_recipeLoadFailed = true;
                return false;
            }

            std::unordered_map<int, CraftRecipeSpec> loadedRecipes;
            for (const JsonUtil::JsonObject& raw : rawRecipes)
            {
                auto recipeIt = raw.find("recipe");
                auto resultIt = raw.find("result");
                auto ingredientsIt = raw.find("ingredients");
                if (recipeIt == raw.end() || resultIt == raw.end() || ingredientsIt == raw.end())
                {
                    Logger::Error("Crafting", "Embedded recipe manifest entry missing required field");
                    g_recipeLoadFailed = true;
                    return false;
                }

                int recipeIndex = -1;
                if (!ParseIntValue(TrimCopy(recipeIt->second), recipeIndex))
                {
                    Logger::Error("Crafting", "Invalid embedded recipe index '%s'", recipeIt->second.c_str());
                    g_recipeLoadFailed = true;
                    return false;
                }

                CraftRecipeSpec recipe;
                recipe.recipeIndex = recipeIndex;
                if (!ParseItemToken(resultIt->second, recipe.result) ||
                    !ParseIngredientsField(ingredientsIt->second, recipe.ingredients))
                {
                    Logger::Error("Crafting", "Invalid embedded recipe payload for recipe=%d", recipeIndex);
                    g_recipeLoadFailed = true;
                    return false;
                }

                if (loadedRecipes.find(recipeIndex) != loadedRecipes.end())
                {
                    Logger::Error("Crafting", "Duplicate embedded recipe index=%d", recipeIndex);
                    g_recipeLoadFailed = true;
                    return false;
                }

                loadedRecipes[recipeIndex] = std::move(recipe);
            }

            g_recipeMap = std::move(loadedRecipes);
            Logger::Info("Crafting", "Loaded %d embedded inventory crafting recipes",
                static_cast<int>(g_recipeMap.size()));
            return true;
        }

        void EnsureRecipesLoaded()
        {
            std::call_once(g_recipeLoadOnce, []()
            {
                LoadEmbeddedRecipes();
            });
        }
    }

    const CraftRecipeSpec* FindLegacyCraftingRecipe(int recipeIndex)
    {
        EnsureRecipesLoaded();
        if (g_recipeLoadFailed)
            return nullptr;

        auto it = g_recipeMap.find(recipeIndex);
        if (it == g_recipeMap.end())
            return nullptr;
        return &it->second;
    }
}

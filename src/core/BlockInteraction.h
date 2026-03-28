#pragma once

#include "../world/World.h"

namespace LCEServer::BlockInteraction
{
    struct InteractionResult
    {
        int blockId = 0;
        int blockData = 0;
    };

    int GetToggledRepeaterDelayData(int blockData);
    int GetToggledLeverData(int blockData);

    bool TryResolveComparatorToggle(
        World* world,
        int x,
        int y,
        int z,
        int blockData,
        InteractionResult& outResult);

    bool TryResolveButtonPress(
        int blockId,
        int blockData,
        InteractionResult& outResult,
        int& outReleaseTicks);
}

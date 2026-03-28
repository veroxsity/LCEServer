#pragma once

#include "../world/World.h"

namespace LCEServer::BlockInteraction
{
    struct InteractionResult
    {
        int blockId = 0;
        int blockData = 0;
    };

    enum class PressurePlateTickAction
    {
        NoChange,
        KeepPressed,
        Release
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

    bool TryResolvePressurePlateState(
        World* world,
        int x,
        int y,
        int z,
        bool occupied,
        InteractionResult& outResult);

    bool ShouldApplyDiodeTransition(
        World* world,
        int x,
        int y,
        int z,
        int expectedBlockId,
        int expectedBlockData);

    bool TryResolveButtonRelease(
        World* world,
        int x,
        int y,
        int z,
        int blockId,
        int baseData,
        InteractionResult& outResult);

    PressurePlateTickAction GetPressurePlateTickAction(
        World* world,
        int x,
        int y,
        int z,
        int expectedBlockId,
        bool occupied);
}

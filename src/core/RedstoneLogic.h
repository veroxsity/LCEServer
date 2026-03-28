#pragma once

#include "../world/World.h"
#include <functional>

namespace LCEServer::RedstoneLogic
{
    using BlockChangedCallback =
        std::function<void(int, int, int, int, int, int, int)>;
    using DustDataChangedCallback =
        std::function<void(int, int, int, int, int)>;
    using ScheduleDiodeChangeCallback =
        std::function<void(int, int, int, int, int, int, int)>;

    int GetMinimalDiodeDirection(int blockData);

    bool TryGetMinimalTorchSupportPosition(
        int blockData,
        int sourceX,
        int sourceY,
        int sourceZ,
        int& outX,
        int& outY,
        int& outZ);

    bool IsMinimalBlockPoweredAt(
        World* world,
        int x,
        int y,
        int z,
        int excludeX,
        int excludeY,
        int excludeZ);

    int GetMinimalComparatorInputSignal(
        World* world,
        int x,
        int y,
        int z,
        int dir);

    int GetMinimalComparatorSideSignal(
        World* world,
        int x,
        int y,
        int z,
        int dir);

    void RefreshMinimalRedstoneAround(
        World* world,
        int originX,
        int originY,
        int originZ,
        const BlockChangedCallback& onBlockChanged,
        const DustDataChangedCallback& onDustDataChanged,
        const ScheduleDiodeChangeCallback& onScheduleDiodeChange);

    void CleanupUnsupportedSupportSensitiveBlocksAround(
        World* world,
        int originX,
        int originY,
        int originZ,
        const BlockChangedCallback& onBlockChanged,
        const DustDataChangedCallback& onDustDataChanged,
        const ScheduleDiodeChangeCallback& onScheduleDiodeChange);
}

#pragma once

#include "../world/World.h"

namespace LCEServer::TileSupport
{
    bool IsTopSolidSupportBlock(int blockId);
    bool IsPressurePlateBlock(int blockId);
    bool IsSupportSensitiveBlock(int blockId);

    bool TryGetWorldBlock(World* world, int x, int y, int z, int& outBlockId, int& outBlockData);
    int GetWorldBlockId(World* world, int x, int y, int z);
    int GetWorldBlockData(World* world, int x, int y, int z);
    bool TryGetTorchSupportPosition(
        int blockData,
        int sourceX,
        int sourceY,
        int sourceZ,
        int& outX,
        int& outY,
        int& outZ);

    bool CanSupportSensitiveBlockStay(
        World* world,
        int x,
        int y,
        int z,
        int blockId,
        int blockData);
}

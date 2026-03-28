#pragma once

#include "../world/World.h"

namespace LCEServer::BlockPlacement
{
    bool ResolveTilePlanterPlacementTarget(
        int clickedBlockId,
        int clickedBlockData,
        int clickedX,
        int clickedY,
        int clickedZ,
        int face,
        int& outX,
        int& outY,
        int& outZ);

    bool ResolveRedstoneDustPlacementTarget(
        int clickedBlockId,
        int clickedX,
        int clickedY,
        int clickedZ,
        int face,
        int& outX,
        int& outY,
        int& outZ);

    int GetFacingFromPlayerYaw(float yRot);
    int GetHorizontalDirectionFromPlayerYaw(float yRot);
    int GetDoorDirectionFromPlayerYaw(float yRot);
    int GetRotatedPillarDataFromFace(int itemDamage, int face);
    int GetPistonFacingForPlacement(int x, int y, int z, double playerX, double playerY, double playerZ, float yRot);
    int GetDiodeDirectionFromPlayerYaw(float yRot);

    bool TryGetLeverPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        float yRot,
        int& outData);

    bool TryGetButtonPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int& outData);

    bool TryGetTorchPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int& outData);
}

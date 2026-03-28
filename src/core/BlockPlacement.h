#pragma once

#include "../world/World.h"

namespace LCEServer::BlockPlacement
{
    struct BedPlacementPlan
    {
        int footX = 0;
        int footY = 0;
        int footZ = 0;
        int headX = 0;
        int headY = 0;
        int headZ = 0;
        int footData = 0;
        int headData = 0;
    };

    struct DoorPlacementPlan
    {
        int lowerX = 0;
        int lowerY = 0;
        int lowerZ = 0;
        int upperX = 0;
        int upperY = 0;
        int upperZ = 0;
        int blockId = 0;
        int lowerData = 0;
        int upperData = 0;
    };

    bool IsReplaceableBlock(int blockId);
    bool CanReplaceAt(World* world, int x, int y, int z);

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

    bool TryResolvePlacementData(
        World* world,
        int blockId,
        int itemDamage,
        int x,
        int y,
        int z,
        int face,
        double playerX,
        double playerY,
        double playerZ,
        float yRot,
        int& outBlockData);

    bool TryBuildBedPlacementPlan(
        int x,
        int y,
        int z,
        int face,
        float yRot,
        BedPlacementPlan& outPlan);

    bool CanPlaceBed(World* world, const BedPlacementPlan& plan);

    bool TryPlanDoorPlacement(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int doorBlockId,
        float yRot,
        DoorPlacementPlan& outPlan);

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

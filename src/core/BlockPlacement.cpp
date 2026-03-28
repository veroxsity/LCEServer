#include "BlockPlacement.h"
#include "TileSupport.h"
#include <cmath>

namespace LCEServer::BlockPlacement
{
    namespace
    {
        constexpr double kPlayerEyeHeight = 1.62;

        void OffsetByFace(int face, int& x, int& y, int& z)
        {
            switch (face)
            {
            case 0: --y; break;
            case 1: ++y; break;
            case 2: --z; break;
            case 3: ++z; break;
            case 4: --x; break;
            case 5: ++x; break;
            default: break;
            }
        }

        int GetWorldBlockId(World* world, int x, int y, int z)
        {
            int blockId = 0;
            int blockData = 0;
            if (!TileSupport::TryGetWorldBlock(world, x, y, z, blockId, blockData))
                return 0;
            return blockId;
        }
    }

    bool IsReplaceableBlock(int blockId)
    {
        switch (blockId)
        {
        case 0:
        case 31:
        case 32:
        case 37:
        case 38:
        case 39:
        case 40:
        case 59:
        case 78:
        case 83:
        case 106:
        case 175:
            return true;
        default:
            return false;
        }
    }

    bool CanReplaceAt(World* world, int x, int y, int z)
    {
        return GetWorldBlockId(world, x, y, z) == 0 || IsReplaceableBlock(GetWorldBlockId(world, x, y, z));
    }

    bool TryResolvePlacedBlock(int itemId, int itemDamage, ResolvedBlockPlacement& outPlacement)
    {
        if (itemId >= 0 && itemId < 256)
        {
            outPlacement.blockId = itemId;
            outPlacement.blockData = itemDamage & 0xF;
            return true;
        }

        switch (itemId)
        {
        case 331:
            outPlacement.blockId = 55;
            outPlacement.blockData = 0;
            return true;
        case 354:
            outPlacement.blockId = 92;
            outPlacement.blockData = 0;
            return true;
        case 356:
            outPlacement.blockId = 93;
            outPlacement.blockData = 0;
            return true;
        case 379:
            outPlacement.blockId = 117;
            outPlacement.blockData = 0;
            return true;
        case 380:
            outPlacement.blockId = 118;
            outPlacement.blockData = 0;
            return true;
        case 390:
            outPlacement.blockId = 140;
            outPlacement.blockData = 0;
            return true;
        case 404:
            outPlacement.blockId = 149;
            outPlacement.blockData = 0;
            return true;
        default:
            return false;
        }
    }

    bool CanPlaceResolvedBlock(
        World* world,
        const ResolvedBlockPlacement& placement,
        int placeX,
        int placeY,
        int placeZ,
        int clickedX,
        int clickedY,
        int clickedZ,
        int clickedBlockId)
    {
        if (placeY < 0 || placeY >= LEGACY_WORLD_HEIGHT)
            return false;

        if ((placement.blockId == 55 || placement.blockId == 93 || placement.blockId == 149) &&
            !TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, placeX, placeY - 1, placeZ)))
        {
            return false;
        }

        if (placement.blockId == 55)
        {
            const int existing = GetWorldBlockId(world, placeX, placeY, placeZ);
            const bool replacingThinSnow =
                (placeX == clickedX && placeY == clickedY && placeZ == clickedZ && clickedBlockId == 78);
            if (existing != 0 && !replacingThinSnow)
                return false;
        }

        return true;
    }

    bool ResolveTilePlanterPlacementTarget(
        int clickedBlockId,
        int clickedBlockData,
        int clickedX,
        int clickedY,
        int clickedZ,
        int face,
        int& outX,
        int& outY,
        int& outZ)
    {
        if (face < 0 || face > 5)
            return false;

        outX = clickedX;
        outY = clickedY;
        outZ = clickedZ;

        if (clickedBlockId == 78 && (clickedBlockData & 0x7) < 1)
            return true;

        if (clickedBlockId == 106 || clickedBlockId == 31 || clickedBlockId == 32)
            return true;

        OffsetByFace(face, outX, outY, outZ);
        return true;
    }

    bool ResolveRedstoneDustPlacementTarget(
        int clickedBlockId,
        int clickedX,
        int clickedY,
        int clickedZ,
        int face,
        int& outX,
        int& outY,
        int& outZ)
    {
        if (face < 0 || face > 5)
            return false;

        outX = clickedX;
        outY = clickedY;
        outZ = clickedZ;

        if (clickedBlockId == 78)
            return true;

        OffsetByFace(face, outX, outY, outZ);
        return true;
    }

    bool TryResolvePredictedPlacementTarget(
        World* world,
        int clickedX,
        int clickedY,
        int clickedZ,
        int face,
        int& outX,
        int& outY,
        int& outZ)
    {
        outX = clickedX;
        outY = clickedY;
        outZ = clickedZ;

        if (face == -1)
            return true;

        const int normalizedFace = face & 0xFF;
        if (normalizedFace > 5)
            return false;

        const bool clickedReplaceable = IsReplaceableBlock(GetWorldBlockId(world, clickedX, clickedY, clickedZ));
        if (!clickedReplaceable)
            OffsetByFace(normalizedFace, outX, outY, outZ);

        return true;
    }

    bool TryResolvePlacementTarget(
        World* world,
        int itemId,
        int clickedBlockId,
        int clickedBlockData,
        int clickedX,
        int clickedY,
        int clickedZ,
        int face,
        int& outX,
        int& outY,
        int& outZ)
    {
        (void)world;

        outX = clickedX;
        outY = clickedY;
        outZ = clickedZ;

        if (face == -1)
            return true;

        const int normalizedFace = face & 0xFF;
        if (itemId == 331)
        {
            return ResolveRedstoneDustPlacementTarget(
                clickedBlockId,
                clickedX,
                clickedY,
                clickedZ,
                normalizedFace,
                outX,
                outY,
                outZ);
        }

        return ResolveTilePlanterPlacementTarget(
            clickedBlockId,
            clickedBlockData,
            clickedX,
            clickedY,
            clickedZ,
            normalizedFace,
            outX,
            outY,
            outZ);
    }

    int GetFacingFromPlayerYaw(float yRot)
    {
        int dir = (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 3;
        switch (dir)
        {
        case 0: return 2;
        case 1: return 5;
        case 2: return 3;
        default: return 4;
        }
    }

    int GetHorizontalDirectionFromPlayerYaw(float yRot)
    {
        return (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 3;
    }

    int GetDoorDirectionFromPlayerYaw(float yRot)
    {
        return (static_cast<int>(std::floor(((yRot + 180.0) * 4.0) / 360.0 - 0.5))) & 3;
    }

    int GetRotatedPillarDataFromFace(int itemDamage, int face)
    {
        const int type = itemDamage & 0x3;
        switch (face)
        {
        case 2:
        case 3:
            return type | (2 << 2);
        case 4:
        case 5:
            return type | (1 << 2);
        case 0:
        case 1:
        default:
            return type;
        }
    }

    int GetPistonFacingForPlacement(int x, int y, int z, double playerX, double playerY, double playerZ, float yRot)
    {
        if (std::abs(playerX - x) < 2.0 && std::abs(playerZ - z) < 2.0)
        {
            const double py = playerY + kPlayerEyeHeight;
            if (py - y > 2.0)
                return 1;
            if (y - py > 0.0)
                return 0;
        }

        const int dir = (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 0x3;
        switch (dir)
        {
        case 0: return 2;
        case 1: return 5;
        case 2: return 3;
        default: return 4;
        }
    }

    int GetDiodeDirectionFromPlayerYaw(float yRot)
    {
        return (((static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 3) + 2) % 4;
    }

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
        int& outBlockData)
    {
        outBlockData = itemDamage & 0xF;

        if (blockId == 17)
        {
            outBlockData = GetRotatedPillarDataFromFace(itemDamage, face);
            return true;
        }

        if (blockId == 69)
        {
            return TryGetLeverPlacementData(world, x, y, z, face, yRot, outBlockData);
        }

        if (blockId == 77 || blockId == 143)
        {
            return TryGetButtonPlacementData(world, x, y, z, face, outBlockData);
        }

        if (blockId == 29 || blockId == 33)
        {
            outBlockData = GetPistonFacingForPlacement(x, y, z, playerX, playerY, playerZ, yRot);
            return true;
        }

        if (blockId == 54 || blockId == 146 || blockId == 61 || blockId == 62)
        {
            outBlockData = GetFacingFromPlayerYaw(yRot);
            return true;
        }

        if (blockId == 93 || blockId == 149)
        {
            outBlockData = GetDiodeDirectionFromPlayerYaw(yRot);
            return true;
        }

        if (blockId == 50 || blockId == 75 || blockId == 76)
        {
            return TryGetTorchPlacementData(world, x, y, z, face, outBlockData);
        }

        return true;
    }

    bool TryBuildBedPlacementPlan(
        int x,
        int y,
        int z,
        int face,
        float yRot,
        BedPlacementPlan& outPlan)
    {
        if (face != 1 || y < 0 || y >= LEGACY_WORLD_HEIGHT)
            return false;

        outPlan.footX = x;
        outPlan.footY = y;
        outPlan.footZ = z;
        outPlan.footData = GetHorizontalDirectionFromPlayerYaw(yRot);

        int xOffset = 0;
        int zOffset = 0;
        if (outPlan.footData == 0) zOffset = 1;
        else if (outPlan.footData == 1) xOffset = -1;
        else if (outPlan.footData == 2) zOffset = -1;
        else xOffset = 1;

        outPlan.headX = x + xOffset;
        outPlan.headY = y;
        outPlan.headZ = z + zOffset;
        outPlan.headData = outPlan.footData | 0x8;
        return outPlan.headY >= 0 && outPlan.headY < LEGACY_WORLD_HEIGHT;
    }

    bool CanPlaceBed(World* world, const BedPlacementPlan& plan)
    {
        return CanReplaceAt(world, plan.footX, plan.footY, plan.footZ) &&
               CanReplaceAt(world, plan.headX, plan.headY, plan.headZ) &&
               TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, plan.footX, plan.footY - 1, plan.footZ)) &&
               TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, plan.headX, plan.headY - 1, plan.headZ));
    }

    bool TryPlanDoorPlacement(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int doorBlockId,
        float yRot,
        DoorPlacementPlan& outPlan)
    {
        if (face != 1 || y < 0 || y >= LEGACY_WORLD_HEIGHT - 1)
            return false;

        if (!TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y - 1, z)) ||
            !CanReplaceAt(world, x, y, z) ||
            !CanReplaceAt(world, x, y + 1, z))
        {
            return false;
        }

        outPlan.lowerX = x;
        outPlan.lowerY = y;
        outPlan.lowerZ = z;
        outPlan.upperX = x;
        outPlan.upperY = y + 1;
        outPlan.upperZ = z;
        outPlan.blockId = doorBlockId;
        outPlan.lowerData = GetDoorDirectionFromPlayerYaw(yRot);

        int xOffset = 0;
        int zOffset = 0;
        if (outPlan.lowerData == 0) zOffset = 1;
        else if (outPlan.lowerData == 1) xOffset = -1;
        else if (outPlan.lowerData == 2) zOffset = -1;
        else xOffset = 1;

        const int solidLeft =
            (TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x - xOffset, y, z - zOffset)) ? 1 : 0) +
            (TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x - xOffset, y + 1, z - zOffset)) ? 1 : 0);
        const int solidRight =
            (TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x + xOffset, y, z + zOffset)) ? 1 : 0) +
            (TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x + xOffset, y + 1, z + zOffset)) ? 1 : 0);

        const bool doorLeft =
            (GetWorldBlockId(world, x - xOffset, y, z - zOffset) == doorBlockId) ||
            (GetWorldBlockId(world, x - xOffset, y + 1, z - zOffset) == doorBlockId);
        const bool doorRight =
            (GetWorldBlockId(world, x + xOffset, y, z + zOffset) == doorBlockId) ||
            (GetWorldBlockId(world, x + xOffset, y + 1, z + zOffset) == doorBlockId);

        bool flip = false;
        if (doorLeft && !doorRight)
            flip = true;
        else if (solidRight > solidLeft)
            flip = true;

        outPlan.upperData = 0x8 | (flip ? 1 : 0);
        return true;
    }

    bool TryGetLeverPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        float yRot,
        int& outData)
    {
        const int yawDir = (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 1;
        int leverData = -1;
        if (face == 0 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y + 1, z)))
            leverData = yawDir == 0 ? 7 : 0;
        else if (face == 1 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y - 1, z)))
            leverData = yawDir == 0 ? 5 : 6;
        else if (face == 2 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y, z + 1)))
            leverData = 4;
        else if (face == 3 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y, z - 1)))
            leverData = 3;
        else if (face == 4 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x + 1, y, z)))
            leverData = 2;
        else if (face == 5 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x - 1, y, z)))
            leverData = 1;

        if (leverData < 0)
            return false;

        outData = leverData;
        return true;
    }

    bool TryGetButtonPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int& outData)
    {
        int buttonData = -1;
        if (face == 2 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y, z + 1)))
            buttonData = 4;
        else if (face == 3 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y, z - 1)))
            buttonData = 3;
        else if (face == 4 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x + 1, y, z)))
            buttonData = 2;
        else if (face == 5 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x - 1, y, z)))
            buttonData = 1;

        if (buttonData < 0)
            return false;

        outData = buttonData;
        return true;
    }

    bool TryGetTorchPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int& outData)
    {
        int torchData = -1;
        if (face == 1 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y - 1, z)))
            torchData = 5;
        else if (face == 2 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y, z + 1)))
            torchData = 4;
        else if (face == 3 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x, y, z - 1)))
            torchData = 3;
        else if (face == 4 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x + 1, y, z)))
            torchData = 2;
        else if (face == 5 && TileSupport::IsTopSolidSupportBlock(GetWorldBlockId(world, x - 1, y, z)))
            torchData = 1;

        if (torchData < 0)
            return false;

        outData = torchData;
        return true;
    }
}

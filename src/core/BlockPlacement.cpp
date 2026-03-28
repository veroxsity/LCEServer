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

    bool TryGetLeverPlacementData(
        World* world,
        int x,
        int y,
        int z,
        int face,
        float yRot,
        int& outData)
    {
        const auto getBlockId = [world](int bx, int by, int bz) -> int
        {
            int blockId = 0;
            int blockData = 0;
            if (!TileSupport::TryGetWorldBlock(world, bx, by, bz, blockId, blockData))
                return 0;
            return blockId;
        };

        const int yawDir = (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 1;
        int leverData = -1;
        if (face == 0 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y + 1, z)))
            leverData = yawDir == 0 ? 7 : 0;
        else if (face == 1 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y - 1, z)))
            leverData = yawDir == 0 ? 5 : 6;
        else if (face == 2 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y, z + 1)))
            leverData = 4;
        else if (face == 3 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y, z - 1)))
            leverData = 3;
        else if (face == 4 && TileSupport::IsTopSolidSupportBlock(getBlockId(x + 1, y, z)))
            leverData = 2;
        else if (face == 5 && TileSupport::IsTopSolidSupportBlock(getBlockId(x - 1, y, z)))
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
        const auto getBlockId = [world](int bx, int by, int bz) -> int
        {
            int blockId = 0;
            int blockData = 0;
            if (!TileSupport::TryGetWorldBlock(world, bx, by, bz, blockId, blockData))
                return 0;
            return blockId;
        };

        int buttonData = -1;
        if (face == 2 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y, z + 1)))
            buttonData = 4;
        else if (face == 3 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y, z - 1)))
            buttonData = 3;
        else if (face == 4 && TileSupport::IsTopSolidSupportBlock(getBlockId(x + 1, y, z)))
            buttonData = 2;
        else if (face == 5 && TileSupport::IsTopSolidSupportBlock(getBlockId(x - 1, y, z)))
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
        const auto getBlockId = [world](int bx, int by, int bz) -> int
        {
            int blockId = 0;
            int blockData = 0;
            if (!TileSupport::TryGetWorldBlock(world, bx, by, bz, blockId, blockData))
                return 0;
            return blockId;
        };

        int torchData = -1;
        if (face == 1 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y - 1, z)))
            torchData = 5;
        else if (face == 2 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y, z + 1)))
            torchData = 4;
        else if (face == 3 && TileSupport::IsTopSolidSupportBlock(getBlockId(x, y, z - 1)))
            torchData = 3;
        else if (face == 4 && TileSupport::IsTopSolidSupportBlock(getBlockId(x + 1, y, z)))
            torchData = 2;
        else if (face == 5 && TileSupport::IsTopSolidSupportBlock(getBlockId(x - 1, y, z)))
            torchData = 1;

        if (torchData < 0)
            return false;

        outData = torchData;
        return true;
    }
}

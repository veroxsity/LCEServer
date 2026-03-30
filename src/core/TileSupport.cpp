#include "TileSupport.h"

namespace LCEServer::TileSupport
{
    namespace
    {
        bool HasSolidSideSupport(World* world, int x, int y, int z)
        {
            int blockId = 0;
            int blockData = 0;
            return TryGetWorldBlock(world, x, y, z, blockId, blockData) &&
                   IsTopSolidSupportBlock(blockId);
        }
    }

    bool IsTopSolidSupportBlock(int blockId)
    {
        switch (blockId)
        {
        case 0:
        case 6:
        case 27:
        case 28:
        case 31:
        case 32:
        case 37:
        case 38:
        case 39:
        case 40:
        case 50:
        case 51:
        case 55:
        case 59:
        case 63:
        case 64:
        case 65:
        case 66:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
        case 75:
        case 76:
        case 77:
        case 78:
        case 83:
        case 92:
        case 93:
        case 94:
        case 96:
        case 104:
        case 105:
        case 106:
        case 111:
        case 115:
        case 117:
        case 118:
        case 127:
        case 131:
        case 132:
        case 140:
        case 143:
        case 149:
        case 150:
        case 157:
        case 171:
        case 175:
            return false;
        default:
            return true;
        }
    }

    bool IsPressurePlateBlock(int blockId)
    {
        return blockId == 70 || blockId == 72;
    }

    bool IsSupportSensitiveBlock(int blockId)
    {
        switch (blockId)
        {
        case 50:
        case 55:
        case 69:
        case 70:
        case 72:
        case 75:
        case 76:
        case 77:
        case 93:
        case 94:
        case 143:
        case 149:
        case 150:
            return true;
        default:
            return false;
        }
    }

    bool TryGetWorldBlock(World* world, int x, int y, int z, int& outBlockId, int& outBlockData)
    {
        outBlockId = 0;
        outBlockData = 0;
        if (!world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
            return false;

        ChunkData* chunk = world->GetChunk(x >> 4, z >> 4);
        if (!chunk || chunk->blocks.empty())
            return false;

        int lx = ((x % 16) + 16) % 16;
        int lz = ((z % 16) + 16) % 16;
        int idx = ChunkBlockIndex(lx, lz, y);
        if (idx < 0 || idx >= static_cast<int>(chunk->blocks.size()))
            return false;

        outBlockId = chunk->blocks[idx];
        if (!chunk->data.empty())
        {
            int nibbleIndex = idx >> 1;
            uint8_t packed = chunk->data[nibbleIndex];
            outBlockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
        }
        return true;
    }

    int GetWorldBlockId(World* world, int x, int y, int z)
    {
        int blockId = 0;
        int blockData = 0;
        return TryGetWorldBlock(world, x, y, z, blockId, blockData) ? blockId : 0;
    }

    int GetWorldBlockData(World* world, int x, int y, int z)
    {
        int blockId = 0;
        int blockData = 0;
        return TryGetWorldBlock(world, x, y, z, blockId, blockData) ? blockData : 0;
    }

    bool TryGetTorchSupportPosition(
        int blockData,
        int sourceX,
        int sourceY,
        int sourceZ,
        int& outX,
        int& outY,
        int& outZ)
    {
        switch (blockData & 0x7)
        {
        case 1:
            outX = sourceX - 1; outY = sourceY; outZ = sourceZ;
            return true;
        case 2:
            outX = sourceX + 1; outY = sourceY; outZ = sourceZ;
            return true;
        case 3:
            outX = sourceX; outY = sourceY; outZ = sourceZ - 1;
            return true;
        case 4:
            outX = sourceX; outY = sourceY; outZ = sourceZ + 1;
            return true;
        case 5:
            outX = sourceX; outY = sourceY - 1; outZ = sourceZ;
            return true;
        default:
            return false;
        }
    }

    bool CanSupportSensitiveBlockStay(
        World* world,
        int x,
        int y,
        int z,
        int blockId,
        int blockData)
    {
        switch (blockId)
        {
        case 55:
        case 70:
        case 72:
        case 93:
        case 94:
        case 149:
        case 150:
            return HasSolidSideSupport(world, x, y - 1, z);

        case 50:
        case 75:
        case 76:
        {
            int supportX = 0;
            int supportY = 0;
            int supportZ = 0;
            if (!TryGetTorchSupportPosition(blockData, x, y, z, supportX, supportY, supportZ))
                return false;
            return HasSolidSideSupport(world, supportX, supportY, supportZ);
        }

        case 77:
        case 143:
            switch (blockData & 0x7)
            {
            case 1: return HasSolidSideSupport(world, x - 1, y, z);
            case 2: return HasSolidSideSupport(world, x + 1, y, z);
            case 3: return HasSolidSideSupport(world, x, y, z - 1);
            case 4: return HasSolidSideSupport(world, x, y, z + 1);
            default: return false;
            }

        case 69:
            switch (blockData & 0x7)
            {
            case 0:
            case 7:
                return HasSolidSideSupport(world, x, y + 1, z);
            case 1:
                return HasSolidSideSupport(world, x - 1, y, z);
            case 2:
                return HasSolidSideSupport(world, x + 1, y, z);
            case 3:
                return HasSolidSideSupport(world, x, y, z - 1);
            case 4:
                return HasSolidSideSupport(world, x, y, z + 1);
            case 5:
            case 6:
                return HasSolidSideSupport(world, x, y - 1, z);
            default:
                return false;
            }

        default:
            return true;
        }
    }
}

// LCEServer — Connection implementation
// Handshake logic mirrors PendingConnection.cpp from the LCE source
#include "Connection.h"
#include "LegacyCrafting.h"
#include "TileSupport.h"
#include "TcpLayer.h"
#include "ServerConfig.h"
#include "../world/World.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace LCEServer
{
    namespace
    {
        constexpr double kSpawnFeetEpsilon = 0.1;
        constexpr int kAnyAuxValue = -1;

        bool ItemsMatch(const ItemInstanceData& a, const ItemInstanceData& b)
        {
            if (a.IsEmpty() && b.IsEmpty())
                return true;
            return a.id == b.id && a.count == b.count && a.aux == b.aux;
        }

        int GetLegacyMaxStackSize(int itemId)
        {
            switch (itemId)
            {
            case 259: // flint and steel
            case 342: // chest minecart
            case 343: // furnace minecart
            case 359: // shears
            case 398: // carrot on a stick
            case 407: // tnt minecart
            case 408: // hopper minecart
                return 1;
            default:
                return 64;
            }
        }

        ItemInstanceData GetCraftingRemainingItem(int itemId)
        {
            switch (itemId)
            {
            case 326: // water bucket
            case 327: // lava bucket
            case 335: // milk bucket
                return ItemInstanceData{325, 1, 0}; // empty bucket
            default:
                return ItemInstanceData();
            }
        }

        bool TryResolvePlacedBlock(int itemId, int itemDamage, int& outBlockId, int& outBlockData)
        {
            if (itemId >= 0 && itemId < 256)
            {
                outBlockId = itemId;
                outBlockData = itemDamage & 0xF;
                return true;
            }

            switch (itemId)
            {
            case 331: // redstone dust
                outBlockId = 55;
                outBlockData = 0;
                return true;
            case 354: // cake
                outBlockId = 92;
                outBlockData = 0;
                return true;
            case 356: // repeater
                outBlockId = 93;
                outBlockData = 0;
                return true;
            case 379: // brewing stand
                outBlockId = 117;
                outBlockData = 0;
                return true;
            case 380: // cauldron
                outBlockId = 118;
                outBlockData = 0;
                return true;
            case 390: // flower pot
                outBlockId = 140;
                outBlockData = 0;
                return true;
            case 404: // comparator
                outBlockId = 149;
                outBlockData = 0;
                return true;
            default:
                return false;
            }
        }

        int CountInventoryResource(
            const std::array<ItemInstanceData, 36>& inventory,
            int itemId,
            int aux,
            bool anyAux)
        {
            int total = 0;
            for (const ItemInstanceData& slot : inventory)
            {
                if (slot.IsEmpty() || slot.id != itemId)
                    continue;
                if (!anyAux && slot.aux != aux)
                    continue;
                total += slot.count;
            }
            return total;
        }

        bool RemoveInventoryResource(
            std::array<ItemInstanceData, 36>& inventory,
            int itemId,
            int aux,
            bool anyAux,
            int count)
        {
            int remaining = count;
            for (ItemInstanceData& slot : inventory)
            {
                if (remaining <= 0)
                    break;
                if (slot.IsEmpty() || slot.id != itemId)
                    continue;
                if (!anyAux && slot.aux != aux)
                    continue;

                int toRemove = (slot.count < remaining) ? slot.count : remaining;
                slot.count = static_cast<uint8_t>(slot.count - toRemove);
                remaining -= toRemove;
                if (slot.count == 0)
                    slot = ItemInstanceData();
            }

            return remaining == 0;
        }

        bool TryInsertInventoryItem(
            std::array<ItemInstanceData, 36>& inventory,
            const ItemInstanceData& item)
        {
            if (item.IsEmpty())
                return true;

            int remaining = item.count;
            const int maxStack = GetLegacyMaxStackSize(item.id);

            for (ItemInstanceData& slot : inventory)
            {
                if (remaining <= 0)
                    break;
                if (slot.IsEmpty() || slot.id != item.id || slot.aux != item.aux || slot.count >= maxStack)
                    continue;

                int space = maxStack - slot.count;
                int toAdd = (space < remaining) ? space : remaining;
                slot.count = static_cast<uint8_t>(slot.count + toAdd);
                remaining -= toAdd;
            }

            for (ItemInstanceData& slot : inventory)
            {
                if (remaining <= 0)
                    break;
                if (!slot.IsEmpty())
                    continue;

                int toAdd = (remaining < maxStack) ? remaining : maxStack;
                slot.id = item.id;
                slot.aux = item.aux;
                slot.count = static_cast<uint8_t>(toAdd);
                remaining -= toAdd;
            }

            return remaining == 0;
        }

        int GetArmorMenuSlotForItemId(int itemId)
        {
            switch (itemId)
            {
            case 298: case 302: case 306: case 310: case 314:
                return 5;
            case 299: case 303: case 307: case 311: case 315:
                return 6;
            case 300: case 304: case 308: case 312: case 316:
                return 7;
            case 301: case 305: case 309: case 313: case 317:
                return 8;
            default:
                return -1;
            }
        }

        int GetFacingFromPlayerYaw(float yRot)
        {
            int dir = (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 3;
            switch (dir)
            {
            case 0: return 2; // Facing::NORTH
            case 1: return 5; // Facing::EAST
            case 2: return 3; // Facing::SOUTH
            default: return 4; // Facing::WEST
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
                return type | (2 << 2); // FACING_Z
            case 4:
            case 5:
                return type | (1 << 2); // FACING_X
            case 0:
            case 1:
            default:
                return type; // FACING_Y
            }
        }

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
            {
                return true;
            }

            if (clickedBlockId == 106 || clickedBlockId == 31 || clickedBlockId == 32)
            {
                return true;
            }

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
            {
                return true;
            }

            OffsetByFace(face, outX, outY, outZ);
            return true;
        }

        using TileSupport::IsTopSolidSupportBlock;

        bool IsInstantBreakBlock(int blockId)
        {
            switch (blockId)
            {
            case 50:  // torch
            case 55:  // redstone dust
            case 75:  // redstone torch off
            case 76:  // redstone torch on
            case 93:  // repeater off
            case 94:  // repeater on
            case 131: // tripwire hook
            case 132: // tripwire
            case 149: // comparator off
            case 150: // comparator on
                return true;
            default:
                return false;
            }
        }

        bool IsRetargetableInstantBreakBlock(int blockId)
        {
            switch (blockId)
            {
            case 55:  // redstone dust
            case 93:  // repeater off
            case 94:  // repeater on
            case 131: // tripwire hook
            case 132: // tripwire
            case 149: // comparator off
            case 150: // comparator on
                return true;
            default:
                return false;
            }
        }

        bool IsRedstoneDebugBlock(int blockId)
        {
            switch (blockId)
            {
            case 55:  // redstone dust
            case 69:  // lever
            case 70:  // stone pressure plate
            case 72:  // wooden pressure plate
            case 75:  // redstone torch off
            case 76:  // redstone torch on
            case 77:  // stone button
            case 93:  // repeater off
            case 94:  // repeater on
            case 131: // tripwire hook
            case 132: // tripwire
            case 143: // wooden button
            case 149: // comparator off
            case 150: // comparator on
                return true;
            default:
                return false;
            }
        }

        using TileSupport::IsPressurePlateBlock;
        using TileSupport::TryGetWorldBlock;

        bool IsMinimalRedstoneSourceBlock(int blockId, int blockData)
        {
            switch (blockId)
            {
            case 69:
            case 77:
            case 143:
                return (blockData & 0x8) != 0;
            case 70:
            case 72:
                return (blockData & 0x1) != 0;
            case 152:
                return true;
            default:
                return false;
            }
        }

        bool TryGetMinimalTorchSupportPosition(
            int blockData,
            int sourceX,
            int sourceY,
            int sourceZ,
            int& outX,
            int& outY,
            int& outZ)
        {
            return TileSupport::TryGetTorchSupportPosition(
                blockData,
                sourceX,
                sourceY,
                sourceZ,
                outX,
                outY,
                outZ);
        }

        int GetMinimalSourcePowerToPosition(
            int blockId,
            int blockData,
            int sourceX,
            int sourceY,
            int sourceZ,
            int targetX,
            int targetY,
            int targetZ)
        {
            if (std::abs(sourceX - targetX) + std::abs(sourceY - targetY) + std::abs(sourceZ - targetZ) != 1)
                return 0;

            switch (blockId)
            {
            case 69:
            case 77:
            case 143:
                return (blockData & 0x8) != 0 ? 15 : 0;
            case 70:
            case 72:
                return (blockData & 0x1) != 0 ? 15 : 0;
            case 152:
                return 15;
            case 76:
            {
                int supportX = 0;
                int supportY = 0;
                int supportZ = 0;
                if (TryGetMinimalTorchSupportPosition(blockData, sourceX, sourceY, sourceZ, supportX, supportY, supportZ) &&
                    supportX == targetX && supportY == targetY && supportZ == targetZ)
                {
                    return 0;
                }
                return 15;
            }
            default:
                return 0;
            }
        }

        bool IsMinimalDiodeBlock(int blockId)
        {
            return blockId == 93 || blockId == 94 || blockId == 149 || blockId == 150;
        }

        bool IsMinimalRedstoneTorchBlock(int blockId)
        {
            return blockId == 75 || blockId == 76;
        }

        int GetMinimalDiodeDirection(int blockData)
        {
            return blockData & 0x3;
        }

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

        int GetMinimalComparatorOutputSignal(
            World* world,
            int x,
            int y,
            int z,
            int blockId,
            int blockData);

        int GetMinimalDiodeOutputToPosition(
            World* world,
            int blockId,
            int blockData,
            int sourceX,
            int sourceY,
            int sourceZ,
            int targetX,
            int targetY,
            int targetZ)
        {
            if (blockId != 94 && blockId != 150)
            {
                if (blockId != 149 && blockId != 150)
                    return 0;
            }
            if (sourceY != targetY)
                return 0;

            static const int stepX[] = { 0, -1, 0, 1 };
            static const int stepZ[] = { 1, 0, -1, 0 };
            const int dir = GetMinimalDiodeDirection(blockData);
            const int outputX = sourceX - stepX[dir];
            const int outputZ = sourceZ - stepZ[dir];
            if (outputX != targetX || outputZ != targetZ)
                return 0;

            if (blockId == 94)
                return 15;
            if (blockId == 150)
                return std::clamp(
                    GetMinimalComparatorOutputSignal(world, sourceX, sourceY, sourceZ, blockId, blockData),
                    0, 15);
            return 0;
        }

        int GetMinimalDirectDustPower(World* world, int x, int y, int z)
        {
            static const int dx[] = { -1, 1, 0, 0, 0, 0 };
            static const int dy[] = { 0, 0, 0, 0, -1, 1 };
            static const int dz[] = { 0, 0, -1, 1, 0, 0 };

            for (int i = 0; i < 6; ++i)
            {
                int neighborBlockId = 0;
                int neighborBlockData = 0;
                const int neighborX = x + dx[i];
                const int neighborY = y + dy[i];
                const int neighborZ = z + dz[i];
                if (!TryGetWorldBlock(world, neighborX, neighborY, neighborZ, neighborBlockId, neighborBlockData))
                    continue;

                int neighborPower = 0;
                neighborPower = GetMinimalSourcePowerToPosition(
                    neighborBlockId, neighborBlockData,
                    neighborX, neighborY, neighborZ,
                    x, y, z);
                if (neighborPower <= 0)
                    neighborPower = GetMinimalDiodeOutputToPosition(
                        world,
                        neighborBlockId, neighborBlockData,
                        neighborX, neighborY, neighborZ,
                        x, y, z);

                if (neighborPower > 0)
                    return neighborPower;
            }
            return 0;
        }

        int GetMinimalComparatorInputSignal(
            World* world,
            int x,
            int y,
            int z,
            int dir)
        {
            if (!world)
                return 0;

            static const int stepX[] = { 0, -1, 0, 1 };
            static const int stepZ[] = { 1, 0, -1, 0 };

            const int inputX = x + stepX[dir];
            const int inputZ = z + stepZ[dir];

            int blockId = 0;
            int blockData = 0;
            if (!TryGetWorldBlock(world, inputX, y, inputZ, blockId, blockData))
                return 0;

            if (blockId == 55)
                return std::clamp(blockData, 0, 15);

            int directSignal = 0;
            directSignal = GetMinimalSourcePowerToPosition(
                blockId, blockData,
                inputX, y, inputZ,
                x, y, z);
            if (directSignal <= 0)
                directSignal = GetMinimalDiodeOutputToPosition(
                    world,
                    blockId, blockData,
                    inputX, y, inputZ,
                    x, y, z);

            return std::clamp(directSignal, 0, 15);
        }

        int GetMinimalComparatorSideSignal(
            World* world,
            int x,
            int y,
            int z,
            int dir)
        {
            if (!world)
                return 0;

            static const int leftX[] = { -1, 0, 1, 0 };
            static const int leftZ[] = { 0, 1, 0, -1 };
            static const int rightX[] = { 1, 0, -1, 0 };
            static const int rightZ[] = { 0, -1, 0, 1 };

            auto getSideSignal = [&](int sx, int sy, int sz) -> int
            {
                int blockId = 0;
                int blockData = 0;
                if (!TryGetWorldBlock(world, sx, sy, sz, blockId, blockData))
                    return 0;

                if (blockId == 55)
                    return std::clamp(blockData, 0, 15);

                const int directSourceSignal = GetMinimalSourcePowerToPosition(
                    blockId, blockData,
                    sx, sy, sz,
                    x, y, z);
                if (directSourceSignal > 0)
                    return directSourceSignal;

                return std::clamp(
                    GetMinimalDiodeOutputToPosition(
                        world,
                        blockId, blockData,
                        sx, sy, sz,
                        x, y, z),
                    0, 15);
            };

            const int leftSignal = getSideSignal(x + leftX[dir], y, z + leftZ[dir]);
            const int rightSignal = getSideSignal(x + rightX[dir], y, z + rightZ[dir]);
            return (leftSignal > rightSignal) ? leftSignal : rightSignal;
        }

        int GetMinimalComparatorOutputSignal(
            World* world,
            int x,
            int y,
            int z,
            int blockId,
            int blockData)
        {
            if (!world || (blockId != 149 && blockId != 150))
                return 0;

            const int dir = GetMinimalDiodeDirection(blockData);
            const int inputSignal = GetMinimalComparatorInputSignal(world, x, y, z, dir);
            const int sideSignal = GetMinimalComparatorSideSignal(world, x, y, z, dir);
            const bool subtractMode = (blockData & 0x4) != 0;

            if (subtractMode)
            {
                const int outputSignal = inputSignal - sideSignal;
                return outputSignal > 0 ? outputSignal : 0;
            }

            if (inputSignal == 0)
                return 0;
            if (sideSignal == 0 || inputSignal >= sideSignal)
                return inputSignal;
            return 0;
        }

        bool IsMinimalBlockPoweredAt(
            World* world,
            int x,
            int y,
            int z,
            int excludeX,
            int excludeY,
            int excludeZ)
        {
            if (!world)
                return false;

            static const int dx[] = { -1, 1, 0, 0, 0, 0 };
            static const int dy[] = { 0, 0, 0, 0, -1, 1 };
            static const int dz[] = { 0, 0, -1, 1, 0, 0 };

            for (int i = 0; i < 6; ++i)
            {
                const int nx = x + dx[i];
                const int ny = y + dy[i];
                const int nz = z + dz[i];
                if (nx == excludeX && ny == excludeY && nz == excludeZ)
                    continue;

                int blockId = 0;
                int blockData = 0;
                if (!TryGetWorldBlock(world, nx, ny, nz, blockId, blockData))
                    continue;

                if (blockId == 55 && blockData > 0)
                    return true;

                if (GetMinimalSourcePowerToPosition(
                        blockId, blockData,
                        nx, ny, nz,
                        x, y, z) > 0)
                {
                    return true;
                }

                if (GetMinimalDiodeOutputToPosition(world, blockId, blockData, nx, ny, nz, x, y, z) > 0)
                    return true;
            }

            return false;
        }

        struct RedstonePos
        {
            int x = 0;
            int y = 0;
            int z = 0;
        };

        void RecomputeMinimalDustPowerAround(
            World* world,
            int originX,
            int originY,
            int originZ,
            const std::function<void(int, int, int, int, int)>& onDustDataChanged,
            std::vector<RedstonePos>* changedDustPositions = nullptr)
        {
            if (!world)
                return;

            auto posKey = [](int x, int y, int z) -> int64_t
            {
                return (static_cast<int64_t>(x) << 40) ^
                       (static_cast<int64_t>(y & 0xFF) << 32) ^
                       static_cast<uint32_t>(z);
            };

            std::queue<RedstonePos> discoverQueue;
            std::unordered_set<int64_t> dustSet;
            std::vector<RedstonePos> dustPositions;

            auto enqueueDustIfPresent = [&](int x, int y, int z)
            {
                int blockId = 0;
                int blockData = 0;
                if (!TryGetWorldBlock(world, x, y, z, blockId, blockData) || blockId != 55)
                    return;

                int64_t key = posKey(x, y, z);
                if (dustSet.insert(key).second)
                {
                    discoverQueue.push({x, y, z});
                    dustPositions.push_back({x, y, z});
                }
            };

            enqueueDustIfPresent(originX, originY, originZ);
            enqueueDustIfPresent(originX, originY + 1, originZ);
            enqueueDustIfPresent(originX, originY - 1, originZ);
            enqueueDustIfPresent(originX - 1, originY, originZ);
            enqueueDustIfPresent(originX + 1, originY, originZ);
            enqueueDustIfPresent(originX, originY, originZ - 1);
            enqueueDustIfPresent(originX, originY, originZ + 1);

            static const int neighborDx[] = { -1, 1, 0, 0 };
            static const int neighborDz[] = { 0, 0, -1, 1 };

            while (!discoverQueue.empty())
            {
                RedstonePos current = discoverQueue.front();
                discoverQueue.pop();

                for (int i = 0; i < 4; ++i)
                    enqueueDustIfPresent(current.x + neighborDx[i], current.y, current.z + neighborDz[i]);
            }

            if (dustPositions.empty())
                return;

            std::unordered_map<int64_t, int> signalByDust;
            std::queue<RedstonePos> powerQueue;

            for (const RedstonePos& dust : dustPositions)
            {
                int sourcePower = GetMinimalDirectDustPower(world, dust.x, dust.y, dust.z);
                int64_t key = posKey(dust.x, dust.y, dust.z);
                signalByDust[key] = sourcePower;
                if (sourcePower > 0)
                    powerQueue.push(dust);
            }

            while (!powerQueue.empty())
            {
                RedstonePos current = powerQueue.front();
                powerQueue.pop();

                int64_t currentKey = posKey(current.x, current.y, current.z);
                int currentSignal = signalByDust[currentKey];
                if (currentSignal <= 1)
                    continue;

                for (int i = 0; i < 4; ++i)
                {
                    RedstonePos neighbor{ current.x + neighborDx[i], current.y, current.z + neighborDz[i] };
                    int64_t neighborKey = posKey(neighbor.x, neighbor.y, neighbor.z);
                    if (dustSet.find(neighborKey) == dustSet.end())
                        continue;

                    int nextSignal = currentSignal - 1;
                    auto found = signalByDust.find(neighborKey);
                    if (found == signalByDust.end() || nextSignal > found->second)
                    {
                        signalByDust[neighborKey] = nextSignal;
                        powerQueue.push(neighbor);
                    }
                }
            }

            for (const RedstonePos& dust : dustPositions)
            {
                int blockId = 0;
                int oldData = 0;
                if (!TryGetWorldBlock(world, dust.x, dust.y, dust.z, blockId, oldData) || blockId != 55)
                    continue;

                int64_t key = posKey(dust.x, dust.y, dust.z);
                int newData = 0;
                auto found = signalByDust.find(key);
                if (found != signalByDust.end())
                    newData = std::clamp(found->second, 0, 15);

                if (oldData == newData)
                    continue;

                if (world->SetBlock(dust.x, dust.y, dust.z, 55, newData))
                {
                    onDustDataChanged(dust.x, dust.y, dust.z, oldData, newData);
                    if (changedDustPositions)
                        changedDustPositions->push_back({ dust.x, dust.y, dust.z });
                }
            }
        }

        struct MinimalDiodeChange
        {
            int x = 0;
            int y = 0;
            int z = 0;
            int oldBlockId = 0;
            int newBlockId = 0;
            int blockData = 0;
            int delayTicks = 0;
        };

        std::vector<MinimalDiodeChange> CollectMinimalDiodeChangesAround(
            World* world,
            int originX,
            int originY,
            int originZ)
        {
            std::vector<MinimalDiodeChange> changes;
            if (!world)
                return changes;

            static const int stepX[] = { 0, -1, 0, 1 };
            static const int stepZ[] = { 1, 0, -1, 0 };

            for (int y = originY - 1; y <= originY + 1; ++y)
            {
                for (int x = originX - 2; x <= originX + 2; ++x)
                {
                    for (int z = originZ - 2; z <= originZ + 2; ++z)
                    {
                        int blockId = 0;
                        int blockData = 0;
                        if (!TryGetWorldBlock(world, x, y, z, blockId, blockData) || !IsMinimalDiodeBlock(blockId))
                            continue;

                        const int dir = GetMinimalDiodeDirection(blockData);
                        const int inputX = x + stepX[dir];
                        const int inputZ = z + stepZ[dir];

                        int targetBlockId = blockId;
                        if (blockId == 93 || blockId == 94)
                        {
                            int inputBlockId = 0;
                            int inputBlockData = 0;
                            int inputSignal = 0;
                            if (TryGetWorldBlock(world, inputX, y, inputZ, inputBlockId, inputBlockData))
                            {
                                if (inputBlockId == 55)
                                    inputSignal = inputBlockData;
                                else if (IsMinimalRedstoneSourceBlock(inputBlockId, inputBlockData) ||
                                         GetMinimalDiodeOutputToPosition(world, inputBlockId, inputBlockData, inputX, y, inputZ, x, y, z) > 0)
                                    inputSignal = 15;
                            }

                            targetBlockId = (inputSignal > 0) ? 94 : 93;
                        }
                        else if (blockId == 149 || blockId == 150)
                        {
                            const int inputSignal = GetMinimalComparatorInputSignal(world, x, y, z, dir);
                            const int sideSignal = GetMinimalComparatorSideSignal(world, x, y, z, dir);
                            const bool shouldTurnOn =
                                inputSignal >= 15 || (inputSignal > 0 && (sideSignal == 0 || inputSignal >= sideSignal));
                            targetBlockId = shouldTurnOn ? 150 : 149;
                        }

                        if (targetBlockId == blockId)
                            continue;

                        int delayTicks = 0;
                        if (blockId == 93 || blockId == 94)
                            delayTicks = (((blockData & 0xC) >> 2) + 1) * 2;
                        else if (blockId == 149 || blockId == 150)
                            delayTicks = 2;

                        changes.push_back({x, y, z, blockId, targetBlockId, blockData, delayTicks});
                    }
                }
            }

            return changes;
        }

        void AppendUniqueMinimalDiodeChangesAround(
            World* world,
            int originX,
            int originY,
            int originZ,
            std::vector<MinimalDiodeChange>& outChanges,
            std::unordered_set<int64_t>& seenPositions)
        {
            auto posKey = [](int x, int y, int z) -> int64_t
            {
                return (static_cast<int64_t>(x) << 40) ^
                       (static_cast<int64_t>(y & 0xFF) << 32) ^
                       static_cast<uint32_t>(z);
            };

            std::vector<MinimalDiodeChange> localChanges =
                CollectMinimalDiodeChangesAround(world, originX, originY, originZ);
            for (const MinimalDiodeChange& change : localChanges)
            {
                const int64_t key = posKey(change.x, change.y, change.z);
                if (!seenPositions.insert(key).second)
                    continue;
                outChanges.push_back(change);
            }
        }

        void AppendUniqueMinimalTorchChangesAround(
            World* world,
            int originX,
            int originY,
            int originZ,
            std::vector<MinimalDiodeChange>& outChanges,
            std::unordered_set<int64_t>& seenPositions)
        {
            auto posKey = [](int x, int y, int z) -> int64_t
            {
                return (static_cast<int64_t>(x) << 40) ^
                       (static_cast<int64_t>(y & 0xFF) << 32) ^
                       static_cast<uint32_t>(z);
            };

            for (int y = originY - 2; y <= originY + 2; ++y)
            {
                for (int x = originX - 2; x <= originX + 2; ++x)
                {
                    for (int z = originZ - 2; z <= originZ + 2; ++z)
                    {
                        int blockId = 0;
                        int blockData = 0;
                        if (!TryGetWorldBlock(world, x, y, z, blockId, blockData) ||
                            !IsMinimalRedstoneTorchBlock(blockId))
                        {
                            continue;
                        }

                        int supportX = 0;
                        int supportY = 0;
                        int supportZ = 0;
                        if (!TryGetMinimalTorchSupportPosition(blockData, x, y, z, supportX, supportY, supportZ))
                            continue;

                        const bool supportPowered =
                            IsMinimalBlockPoweredAt(world, supportX, supportY, supportZ, x, y, z);
                        int targetBlockId = blockId;
                        if (blockId == 76 && supportPowered)
                            targetBlockId = 75;
                        else if (blockId == 75 && !supportPowered)
                            targetBlockId = 76;

                        if (targetBlockId == blockId)
                            continue;

                        const int64_t key = posKey(x, y, z);
                        if (!seenPositions.insert(key).second)
                            continue;

                        outChanges.push_back({x, y, z, blockId, targetBlockId, blockData, 2});
                    }
                }
            }
        }

        bool TryGetMinimalDiodeOutputPosition(
            int blockId,
            int blockData,
            int sourceX,
            int sourceY,
            int sourceZ,
            int& outX,
            int& outY,
            int& outZ)
        {
            if (!IsMinimalDiodeBlock(blockId))
                return false;

            static const int stepX[] = { 0, -1, 0, 1 };
            static const int stepZ[] = { 1, 0, -1, 0 };
            const int dir = GetMinimalDiodeDirection(blockData);
            outX = sourceX - stepX[dir];
            outY = sourceY;
            outZ = sourceZ - stepZ[dir];
            return true;
        }

        void RecomputeMinimalDustAtNearbyDiodeOutputs(
            World* world,
            int originX,
            int originY,
            int originZ,
            const std::function<void(int, int, int, int, int)>& onDustDataChanged)
        {
            if (!world)
                return;

            std::unordered_set<int64_t> refreshedOutputs;
            auto outputKey = [](int x, int y, int z) -> int64_t
            {
                return (static_cast<int64_t>(x) << 40) ^
                       (static_cast<int64_t>(y & 0xFF) << 32) ^
                       static_cast<uint32_t>(z);
            };

            for (int y = originY - 1; y <= originY + 1; ++y)
            {
                for (int x = originX - 2; x <= originX + 2; ++x)
                {
                    for (int z = originZ - 2; z <= originZ + 2; ++z)
                    {
                        int blockId = 0;
                        int blockData = 0;
                        if (!TryGetWorldBlock(world, x, y, z, blockId, blockData) || !IsMinimalDiodeBlock(blockId))
                            continue;

                        int outputX = 0;
                        int outputY = 0;
                        int outputZ = 0;
                        if (!TryGetMinimalDiodeOutputPosition(blockId, blockData, x, y, z, outputX, outputY, outputZ))
                            continue;

                        const int64_t key = outputKey(outputX, outputY, outputZ);
                        if (!refreshedOutputs.insert(key).second)
                            continue;

                        RecomputeMinimalDustPowerAround(world, outputX, outputY, outputZ, onDustDataChanged);
                    }
                }
            }
        }

        void RefreshMinimalRedstoneAround(
            World* world,
            int originX,
            int originY,
            int originZ,
            const std::function<void(int, int, int, int, int, int, int)>& onBlockChanged,
            const std::function<void(int, int, int, int, int)>& onDustDataChanged,
            const std::function<void(int, int, int, int, int, int, int)>& onScheduleDiodeChange)
        {
            if (!world)
                return;

            std::vector<RedstonePos> changedDustPositions;
            RecomputeMinimalDustPowerAround(
                world, originX, originY, originZ, onDustDataChanged, &changedDustPositions);

            bool anyImmediateDiodeChange = false;
            std::vector<MinimalDiodeChange> diodeChanges;
            std::unordered_set<int64_t> seenDiodePositions;
            AppendUniqueMinimalDiodeChangesAround(
                world, originX, originY, originZ, diodeChanges, seenDiodePositions);
            AppendUniqueMinimalTorchChangesAround(
                world, originX, originY, originZ, diodeChanges, seenDiodePositions);
            for (const RedstonePos& dust : changedDustPositions)
            {
                AppendUniqueMinimalDiodeChangesAround(
                    world, dust.x, dust.y, dust.z, diodeChanges, seenDiodePositions);
                AppendUniqueMinimalTorchChangesAround(
                    world, dust.x, dust.y, dust.z, diodeChanges, seenDiodePositions);
            }

            for (const MinimalDiodeChange& change : diodeChanges)
            {
                if (change.delayTicks > 0)
                {
                    onScheduleDiodeChange(
                        change.x, change.y, change.z,
                        change.oldBlockId, change.newBlockId, change.blockData, change.delayTicks);
                    continue;
                }

                if (world->SetBlock(change.x, change.y, change.z, change.newBlockId, change.blockData))
                {
                    anyImmediateDiodeChange = true;
                    onBlockChanged(change.x, change.y, change.z,
                        change.newBlockId, change.blockData,
                        change.oldBlockId, change.blockData);
                    RecomputeMinimalDustPowerAround(world, change.x, change.y, change.z, onDustDataChanged);
                }
            }

            RecomputeMinimalDustAtNearbyDiodeOutputs(world, originX, originY, originZ, onDustDataChanged);
            for (const RedstonePos& dust : changedDustPositions)
                RecomputeMinimalDustAtNearbyDiodeOutputs(world, dust.x, dust.y, dust.z, onDustDataChanged);

            if (anyImmediateDiodeChange)
                RecomputeMinimalDustPowerAround(world, originX, originY, originZ, onDustDataChanged);
        }

        using TileSupport::IsSupportSensitiveBlock;
        using TileSupport::CanSupportSensitiveBlockStay;

        void CleanupUnsupportedSupportSensitiveBlocksAround(
            World* world,
            int originX,
            int originY,
            int originZ,
            const std::function<void(int, int, int, int, int, int, int)>& onBlockChanged,
            const std::function<void(int, int, int, int, int)>& onDustDataChanged,
            const std::function<void(int, int, int, int, int, int, int)>& onScheduleDiodeChange)
        {
            if (!world)
                return;

            auto posKey = [](int x, int y, int z) -> int64_t
            {
                return (static_cast<int64_t>(x) << 40) ^
                       (static_cast<int64_t>(y & 0xFF) << 32) ^
                       static_cast<uint32_t>(z);
            };

            std::queue<RedstonePos> queue;
            std::unordered_set<int64_t> queued;

            auto enqueueNearby = [&](int x, int y, int z)
            {
                static const int dx[] = { 0, -1, 1, 0, 0, 0, 0 };
                static const int dy[] = { 0, 0, 0, -1, 1, 0, 0 };
                static const int dz[] = { 0, 0, 0, 0, 0, -1, 1 };
                for (int i = 0; i < 7; ++i)
                {
                    const int nx = x + dx[i];
                    const int ny = y + dy[i];
                    const int nz = z + dz[i];
                    if (ny < 0 || ny >= LEGACY_WORLD_HEIGHT)
                        continue;

                    const int64_t key = posKey(nx, ny, nz);
                    if (!queued.insert(key).second)
                        continue;
                    queue.push({ nx, ny, nz });
                }
            };

            enqueueNearby(originX, originY, originZ);

            while (!queue.empty())
            {
                const RedstonePos current = queue.front();
                queue.pop();

                int blockId = 0;
                int blockData = 0;
                if (!TryGetWorldBlock(world, current.x, current.y, current.z, blockId, blockData) ||
                    !IsSupportSensitiveBlock(blockId))
                {
                    continue;
                }

                if (CanSupportSensitiveBlockStay(world, current.x, current.y, current.z, blockId, blockData))
                    continue;

                if (!world->SetBlock(current.x, current.y, current.z, 0, 0))
                    continue;

                onBlockChanged(current.x, current.y, current.z, 0, 0, blockId, blockData);
                RefreshMinimalRedstoneAround(
                    world,
                    current.x, current.y, current.z,
                    onBlockChanged,
                    onDustDataChanged,
                    onScheduleDiodeChange);

                enqueueNearby(current.x, current.y, current.z);
            }
        }

        int GetPistonFacingForPlacement(int x, int y, int z, double playerX, double playerY, double playerZ, float yRot)
        {
            if (std::abs(playerX - x) < 2.0 && std::abs(playerZ - z) < 2.0)
            {
                const double py = playerY + 1.62;
                if (py - y > 2.0)
                    return 1; // Facing::UP
                if (y - py > 0.0)
                    return 0; // Facing::DOWN
            }

            const int dir = (static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 0x3;
            switch (dir)
            {
            case 0: return 2; // Facing::NORTH
            case 1: return 5; // Facing::EAST
            case 2: return 3; // Facing::SOUTH
            default: return 4; // Facing::WEST
            }
        }

        int GetDiodeDirectionFromPlayerYaw(float yRot)
        {
            return (((static_cast<int>(std::floor(yRot * 4.0 / 360.0 + 0.5))) & 3) + 2) % 4;
        }
    }

    int64_t Connection::MakeChunkKey(int cx, int cz)
    {
        return (static_cast<int64_t>(cx) << 32) |
            static_cast<uint32_t>(cz);
    }

    int Connection::InventoryIndexToMenuSlot(int inventoryIndex)
    {
        if (inventoryIndex < 0 || inventoryIndex >= 36)
            return -1;
        return inventoryIndex < 9 ? (36 + inventoryIndex) : inventoryIndex;
    }

    int Connection::ArmorIndexToMenuSlot(int armorIndex)
    {
        if (armorIndex < 0 || armorIndex >= 4)
            return -1;
        return 8 - armorIndex;
    }

    int Connection::MenuSlotToInventoryIndex(int menuSlot)
    {
        if (menuSlot >= 9 && menuSlot <= 35)
            return menuSlot;
        if (menuSlot >= 36 && menuSlot <= 44)
            return menuSlot - 36;
        return -1;
    }

    int Connection::MenuSlotToArmorIndex(int menuSlot)
    {
        if (menuSlot >= 5 && menuSlot <= 8)
            return 8 - menuSlot;
        return -1;
    }

    int Connection::InventoryIndexToWorkbenchMenuSlot(int inventoryIndex)
    {
        if (inventoryIndex < 0 || inventoryIndex >= 36)
            return -1;
        return inventoryIndex < 9 ? (37 + inventoryIndex) : (inventoryIndex + 1);
    }

    int Connection::WorkbenchMenuSlotToInventoryIndex(int menuSlot)
    {
        if (menuSlot >= 10 && menuSlot <= 36)
            return menuSlot - 1;
        if (menuSlot >= 37 && menuSlot <= 45)
            return menuSlot - 37;
        return -1;
    }

    ItemInstanceData Connection::GetMenuSlotItem(int menuSlot) const
    {
        int inventoryIndex = MenuSlotToInventoryIndex(menuSlot);
        if (inventoryIndex >= 0)
            return m_inventoryItems[inventoryIndex];

        int armorIndex = MenuSlotToArmorIndex(menuSlot);
        if (armorIndex >= 0)
            return m_armorItems[armorIndex];

        return ItemInstanceData();
    }

    void Connection::SetMenuSlotItem(int menuSlot, const ItemInstanceData& item)
    {
        int inventoryIndex = MenuSlotToInventoryIndex(menuSlot);
        if (inventoryIndex >= 0)
        {
            m_inventoryItems[inventoryIndex] = item;
            return;
        }

        int armorIndex = MenuSlotToArmorIndex(menuSlot);
        if (armorIndex >= 0)
        {
            m_armorItems[armorIndex] = item;
        }
    }

    Connection::Connection(uint8_t smallId, TcpLayer* tcp,
                           const ServerConfig* config, World* world)
        : m_smallId(smallId)
        , m_tcp(tcp)
        , m_config(config)
        , m_world(world)
        , m_state(ConnectionState::Handshake)
        , m_chunkRadius(config ? (config->viewDistance < 2 ? 2 : config->viewDistance > 16 ? 16 : config->viewDistance) : 4)
        , m_chunksPerTick(config ? (config->chunksPerTick < 1 ? 1 : config->chunksPerTick > 64 ? 64 : config->chunksPerTick) : 10)
    {
    }

    Connection::~Connection()
    {
        ReleaseVisibleChunks(false);
    }

    void Connection::OnDataReceived(const uint8_t* data, int size)
    {
        // Queue packet for processing on tick thread
        std::lock_guard<std::mutex> lock(m_packetMutex);
        m_packetQueue.emplace_back(data, data + size);
    }

    void Connection::Tick()
    {
        if (m_done) return;

        m_tickCount++;

        auto getWorldBlock = [this](int x, int y, int z, int& outBlockId, int& outBlockData)
        {
            return TryGetWorldBlock(m_world, x, y, z, outBlockId, outBlockData);
        };

        auto scheduleAwareRedstoneRefresh = [this](int x, int y, int z)
        {
            if (!m_world)
                return;

            RefreshMinimalRedstoneAround(
                m_world,
                x, y, z,
                [this](int bx, int by, int bz, int newBlockId, int newBlockData, int oldBlockId, int oldBlockData)
                {
                    if (onBlockUpdate)
                        onBlockUpdate(this, bx, by, bz, newBlockId, newBlockData, oldBlockId, oldBlockData);
                },
                [this](int dx, int dy, int dz, int oldData, int newData)
                {
                    if (onBlockUpdate)
                        onBlockUpdate(this, dx, dy, dz, 55, newData, 55, oldData);
                },
                [this](int dx, int dy, int dz, int expectedBlockId, int targetBlockId, int blockData, int delayTicks)
                {
                    m_pendingDiodeTransitions.erase(
                        std::remove_if(
                            m_pendingDiodeTransitions.begin(),
                            m_pendingDiodeTransitions.end(),
                            [&](const PendingDiodeTransition& pending)
                            {
                                return pending.x == dx && pending.y == dy && pending.z == dz;
                            }),
                        m_pendingDiodeTransitions.end());

                    PendingDiodeTransition pending;
                    pending.x = dx;
                    pending.y = dy;
                    pending.z = dz;
                    pending.expectedBlockId = expectedBlockId;
                    pending.targetBlockId = targetBlockId;
                    pending.blockData = blockData;
                    pending.ticksRemaining = delayTicks;
                    m_pendingDiodeTransitions.push_back(pending);
                });
        };

        auto ensurePressurePlateCheck = [this](int x, int y, int z, int blockId)
        {
            for (PendingPressurePlateCheck& pending : m_pendingPressurePlateChecks)
            {
                if (pending.x == x && pending.y == y && pending.z == z)
                {
                    pending.blockId = blockId;
                    pending.ticksRemaining = 20;
                    return;
                }
            }

            PendingPressurePlateCheck pending;
            pending.x = x;
            pending.y = y;
            pending.z = z;
            pending.blockId = blockId;
            pending.ticksRemaining = 20;
            m_pendingPressurePlateChecks.push_back(pending);
        };

        auto updatePressurePlateState = [this, &getWorldBlock, &scheduleAwareRedstoneRefresh, &ensurePressurePlateCheck](
            int x,
            int y,
            int z) -> bool
        {
            if (!m_world)
                return false;

            int blockId = 0;
            int blockData = 0;
            if (!getWorldBlock(x, y, z, blockId, blockData) || !IsPressurePlateBlock(blockId))
                return false;

            const bool occupied = isPressurePlateOccupied ? isPressurePlateOccupied(x, y, z, blockId == 72) : false;
            const int targetData = occupied ? 1 : 0;
            if (blockData == targetData)
            {
                if (occupied)
                    ensurePressurePlateCheck(x, y, z, blockId);
                return occupied;
            }

            if (!m_world->SetBlock(x, y, z, blockId, targetData))
                return occupied;

            if (onBlockUpdate)
                onBlockUpdate(this, x, y, z, blockId, targetData, blockId, blockData);

            scheduleAwareRedstoneRefresh(x, y, z);
            if (occupied)
                ensurePressurePlateCheck(x, y, z, blockId);
            return occupied;
        };

        if (m_world &&
            (!m_pendingButtonReleases.empty() ||
             !m_pendingDiodeTransitions.empty() ||
             !m_pendingPressurePlateChecks.empty()))
        {

            for (size_t i = 0; i < m_pendingDiodeTransitions.size();)
            {
                PendingDiodeTransition& pending = m_pendingDiodeTransitions[i];
                if (--pending.ticksRemaining > 0)
                {
                    ++i;
                    continue;
                }

                int currentBlockId = 0;
                int currentBlockData = 0;
                const bool sameExpectedBlock =
                    getWorldBlock(pending.x, pending.y, pending.z, currentBlockId, currentBlockData) &&
                    currentBlockId == pending.expectedBlockId &&
                    currentBlockData == pending.blockData;

                if (sameExpectedBlock &&
                    m_world->SetBlock(pending.x, pending.y, pending.z, pending.targetBlockId, pending.blockData))
                {
                    if (onBlockUpdate)
                    {
                        onBlockUpdate(this, pending.x, pending.y, pending.z,
                            pending.targetBlockId, pending.blockData,
                            pending.expectedBlockId, pending.blockData);
                    }
                    scheduleAwareRedstoneRefresh(pending.x, pending.y, pending.z);
                }

                m_pendingDiodeTransitions.erase(
                    m_pendingDiodeTransitions.begin() + static_cast<std::ptrdiff_t>(i));
            }

            for (size_t i = 0; i < m_pendingButtonReleases.size();)
            {
                PendingButtonRelease& pending = m_pendingButtonReleases[i];
                if (--pending.ticksRemaining > 0)
                {
                    ++i;
                    continue;
                }

                int currentBlockId = 0;
                int currentBlockData = 0;
                const bool samePressedButton =
                    getWorldBlock(pending.x, pending.y, pending.z, currentBlockId, currentBlockData) &&
                    currentBlockId == pending.blockId &&
                    (currentBlockData & 0x8) != 0;

                if (samePressedButton &&
                    m_world->SetBlock(pending.x, pending.y, pending.z, pending.blockId, pending.baseData) &&
                    onBlockUpdate)
                {
                    onBlockUpdate(this, pending.x, pending.y, pending.z,
                        pending.blockId, pending.baseData,
                        pending.blockId, currentBlockData);
                    scheduleAwareRedstoneRefresh(pending.x, pending.y, pending.z);
                }

                m_pendingButtonReleases.erase(m_pendingButtonReleases.begin() + static_cast<std::ptrdiff_t>(i));
            }

            for (size_t i = 0; i < m_pendingPressurePlateChecks.size();)
            {
                PendingPressurePlateCheck& pending = m_pendingPressurePlateChecks[i];
                if (--pending.ticksRemaining > 0)
                {
                    ++i;
                    continue;
                }

                int currentBlockId = 0;
                int currentBlockData = 0;
                const bool samePressedPlate =
                    getWorldBlock(pending.x, pending.y, pending.z, currentBlockId, currentBlockData) &&
                    currentBlockId == pending.blockId &&
                    IsPressurePlateBlock(currentBlockId) &&
                    (currentBlockData & 0x1) != 0;

                if (samePressedPlate)
                {
                    const bool occupied =
                        isPressurePlateOccupied ? isPressurePlateOccupied(
                            pending.x, pending.y, pending.z, currentBlockId == 72) : false;
                    if (occupied)
                    {
                        pending.ticksRemaining = 20;
                        ++i;
                        continue;
                    }

                    if (m_world->SetBlock(pending.x, pending.y, pending.z, currentBlockId, 0))
                    {
                        if (onBlockUpdate)
                        {
                            onBlockUpdate(this, pending.x, pending.y, pending.z,
                                currentBlockId, 0,
                                currentBlockId, currentBlockData);
                        }
                        scheduleAwareRedstoneRefresh(pending.x, pending.y, pending.z);
                    }
                }

                m_pendingPressurePlateChecks.erase(
                    m_pendingPressurePlateChecks.begin() + static_cast<std::ptrdiff_t>(i));
            }
        }

        // Drain packet queue
        std::vector<std::vector<uint8_t>> packets;
        {
            std::lock_guard<std::mutex> lock(m_packetMutex);
            packets.swap(m_packetQueue);
        }

        if (packets.empty())
        {
            m_ticksSinceLastPacket++;

            // Timeout checks matching the source
            if (m_state == ConnectionState::Handshake ||
                m_state == ConnectionState::LoggingIn)
            {
                if (m_ticksSinceLastPacket >= MAX_TICKS_BEFORE_LOGIN)
                {
                    Disconnect(DisconnectReason::LoginTooLong);
                    return;
                }
            }
            else if (m_state == ConnectionState::Playing)
            {
                if (m_ticksSinceLastPacket >= MAX_TICKS_WITHOUT_INPUT)
                {
                    Disconnect(DisconnectReason::TimeOut);
                    return;
                }
            }
        }
        else
        {
            m_ticksSinceLastPacket = 0;
        }

        // Process packets
        for (auto& pkt : packets)
        {
            if (m_done) break;
            int id = PacketHandler::GetPacketId(pkt.data(),
                                                 (int)pkt.size());
            switch (id)
            {
            case PacketId::PreLogin:
                HandlePreLogin(pkt.data(), (int)pkt.size());
                break;
            case PacketId::Login:
                HandleLogin(pkt.data(), (int)pkt.size());
                break;
            case PacketId::KeepAlive:
                HandleKeepAlive(pkt.data(), (int)pkt.size());
                break;
            case PacketId::Chat:
                if (m_state == ConnectionState::Playing)
                    HandleChat(pkt.data(), (int)pkt.size());
                break;
            case PacketId::PlayerAction:
                if (m_state == ConnectionState::Playing)
                    HandlePlayerAction(pkt.data(), (int)pkt.size());
                break;
            case PacketId::UseItem:
                if (m_state == ConnectionState::Playing)
                    HandleUseItem(pkt.data(), (int)pkt.size());
                break;
            case PacketId::SetCarriedItem:
                if (m_state == ConnectionState::Playing)
                    HandleSetCarriedItem(pkt.data(), (int)pkt.size());
                break;
            case PacketId::ContainerClick:
                if (m_state == ConnectionState::Playing)
                    HandleContainerClick(pkt.data(), (int)pkt.size());
                break;
            case PacketId::ContainerAck:
                if (m_state == ConnectionState::Playing)
                    HandleContainerAck(pkt.data(), (int)pkt.size());
                break;
            case PacketId::CraftItem:
                if (m_state == ConnectionState::Playing)
                    HandleCraftItem(pkt.data(), (int)pkt.size());
                break;
            case PacketId::Animate:
                if (m_state == ConnectionState::Playing)
                    HandleAnimate(pkt.data(), (int)pkt.size());
                break;
            case PacketId::PlayerCommand:
                if (m_state == ConnectionState::Playing)
                    HandlePlayerCommand(pkt.data(), (int)pkt.size());
                break;
            case PacketId::Respawn:
                if (m_state == ConnectionState::Playing)
                    HandleRespawn(pkt.data(), (int)pkt.size());
                break;
            case PacketId::ClientCommand:
                if (m_state == ConnectionState::Playing)
                    HandleClientCommand(pkt.data(), (int)pkt.size());
                break;
            case PacketId::MovePlayer:
            case PacketId::MovePlayerPos:
            case PacketId::MovePlayerRot:
            case PacketId::MovePlayerPosRot:
                if (m_state == ConnectionState::Playing)
                    HandleMovePlayer(id, pkt.data(), (int)pkt.size());
                break;
            case PacketId::DebugOptions:
                // Optional client packet used by debug/crafting flows in LCE.
                // Dedicated server currently has no gameplay effect for it.
                break;
            case PacketId::ContainerClose:
                if (m_state == ConnectionState::Playing)
                    HandleContainerClose(pkt.data(), (int)pkt.size());
                break;
            case PacketId::SetCreativeModeSlot:
                // Creative inventory update packet; ignored until inventory sync is implemented.
                break;
            case PacketId::Disconnect:
                Logger::Info("Server", "Client %d sent disconnect",
                             m_smallId);
                m_done = true;
                m_state = ConnectionState::Closing;
                break;
            default:
                if (m_state == ConnectionState::Playing)
                {
                    Logger::Info("Server",
                        "Unhandled packet id=%d size=%d from '%ls'",
                        id, (int)pkt.size(), m_playerName.c_str());
                }
                else
                {
                    // Unexpected packet during handshake
                    Disconnect(DisconnectReason::UnexpectedPacket);
                }
                break;
            }
        }

        if (m_world && m_state == ConnectionState::Playing && isPressurePlateOccupied)
        {
            const int plateX = static_cast<int>(std::floor(m_x));
            const int plateY = static_cast<int>(std::floor(m_y));
            const int plateZ = static_cast<int>(std::floor(m_z));
            updatePressurePlateState(plateX, plateY, plateZ);
        }

        // Send keepalive every 20 ticks when playing
        // (matches Connection::tick from source)
        if (m_state == ConnectionState::Playing &&
            m_tickCount % 20 == 0)
        {
            SendPacket(PacketHandler::WriteKeepAlive());
        }

        // Drain chunk send queue — CHUNK_SEND_PER_TICK chunks per tick
        // so the spiral fills in smoothly rather than all at once.
        if (m_state == ConnectionState::Playing)
            DrainChunkQueue();
    }

    void Connection::SendPacket(const std::vector<uint8_t>& packetData)
    {
        if (m_done) return;
        m_tcp->SendToSmallId(m_smallId, packetData.data(),
                             (int)packetData.size());
    }

    void Connection::Disconnect(DisconnectReason reason)
    {
        if (m_done) return;
        Logger::Info("Server", "Disconnecting smallId=%d reason=%d",
                     m_smallId, (int)reason);
        ReleaseVisibleChunks(false);
        SendPacket(PacketHandler::WriteDisconnect(reason));
        m_done = true;
        m_state = ConnectionState::Closing;
    }

    // ---------------------------------------------------------------
    // HandlePreLogin — mirrors PendingConnection::handlePreLogin
    // ---------------------------------------------------------------
    void Connection::HandlePreLogin(const uint8_t* data, int size)
    {
        if (m_state != ConnectionState::Handshake)
        {
            Disconnect(DisconnectReason::UnexpectedPacket);
            return;
        }

        PreLoginData pkt;
        if (!PacketHandler::ReadPreLogin(data, size, pkt))
        {
            Disconnect(DisconnectReason::UnexpectedPacket);
            return;
        }

        // Version check — matches PendingConnection::handlePreLogin
        if (pkt.netcodeVersion != MINECRAFT_NET_VERSION)
        {
            Logger::Info("Server",
                "Netcode version mismatch: got %d, expected %d",
                pkt.netcodeVersion, MINECRAFT_NET_VERSION);
            if (pkt.netcodeVersion > MINECRAFT_NET_VERSION)
                Disconnect(DisconnectReason::OutdatedServer);
            else
                Disconnect(DisconnectReason::OutdatedClient);
            return;
        }

        m_playerName = pkt.loginKey;
        Logger::Info("Server", "PreLogin from '%ls' (smallId=%d)",
                     m_playerName.c_str(), m_smallId);

        // Send PreLogin response
        SendPreLoginResponse();
        m_state = ConnectionState::LoggingIn;
    }

    void Connection::SendPreLoginResponse()
    {
        // Build response matching PendingConnection::sendPreLoginResponse
        std::vector<PlayerUID> ugcXuids; // No players yet for a fresh server
        uint8_t saveName[14] = {};
        // TODO M2: fill from actual world save name

        auto pkt = PacketHandler::WritePreLoginResponse(
            ugcXuids,
            0,                              // friendsOnlyBits
            m_ugcPlayersVersion,            // ugcPlayersVersion
            saveName,
            m_config->BuildServerSettings(),
            0,                              // hostIndex
            0                               // texturePackId
        );
        SendPacket(pkt);
    }

    // ---------------------------------------------------------------
    // HandleLogin — mirrors PendingConnection::handleLogin
    // ---------------------------------------------------------------
    void Connection::HandleLogin(const uint8_t* data, int size)
    {
        if (m_state != ConnectionState::LoggingIn)
        {
            Disconnect(DisconnectReason::UnexpectedPacket);
            return;
        }

        LoginData pkt;
        if (!PacketHandler::ReadLogin(data, size, pkt))
        {
            Disconnect(DisconnectReason::UnexpectedPacket);
            return;
        }

        // Version check — matches PendingConnection::handleLogin
        if (pkt.clientVersion != NETWORK_PROTOCOL_VERSION)
        {
            Logger::Info("Server",
                "Protocol version mismatch: got %d, expected %d",
                pkt.clientVersion, NETWORK_PROTOCOL_VERSION);
            if (pkt.clientVersion > NETWORK_PROTOCOL_VERSION)
                Disconnect(DisconnectReason::OutdatedServer);
            else
                Disconnect(DisconnectReason::OutdatedClient);
            return;
        }

        // Resolve primary XUID — same logic as PendingConnection
        // "Guests use the online xuid, everyone else uses the offline one"
        // Update player name from the Login packet's userName field.
        // PreLogin only gives us the loginKey (gamertag); the canonical
        // in-game name comes from LoginPacket::userName.
        if (!pkt.userName.empty())
            m_playerName = pkt.userName;

        m_primaryXuid = pkt.offlineXuid;
        if (m_primaryXuid == INVALID_XUID)
            m_primaryXuid = pkt.onlineXuid;
        m_onlineXuid = pkt.onlineXuid;

        // Ban check (XUID)
        if (xuidBanCheck)
        {
            bool banned = false;
            if (m_primaryXuid != INVALID_XUID)
                banned = xuidBanCheck(m_primaryXuid);
            if (!banned && m_onlineXuid != INVALID_XUID &&
                m_onlineXuid != m_primaryXuid)
                banned = xuidBanCheck(m_onlineXuid);
            if (banned)
            {
                Logger::Info("Server",
                    "Rejected '%ls': banned XUID", m_playerName.c_str());
                Disconnect(DisconnectReason::Banned);
                return;
            }
        }

        // Duplicate XUID check
        if (duplicateXuidCheck)
        {
            bool dup = false;
            if (m_primaryXuid != INVALID_XUID)
                dup = duplicateXuidCheck(m_primaryXuid);
            if (!dup && m_onlineXuid != INVALID_XUID &&
                m_onlineXuid != m_primaryXuid)
                dup = duplicateXuidCheck(m_onlineXuid);

            if (dup)
            {
                Logger::Info("Server",
                    "Rejected '%ls': duplicate XUID",
                    m_playerName.c_str());
                Disconnect(DisconnectReason::Banned);
                return;
            }
        }

        // Duplicate name check
        if (duplicateNameCheck && duplicateNameCheck(m_playerName))
        {
            Logger::Info("Server",
                "Rejected '%ls': duplicate name", m_playerName.c_str());
            Disconnect(DisconnectReason::Banned);
            return;
        }

        // Whitelist check (M4)
        if (whitelistCheck && whitelistCheck(m_primaryXuid, m_playerName))
        {
            Logger::Info("Server",
                "Rejected '%ls': not on whitelist",
                m_playerName.c_str());
            Disconnect(DisconnectReason::Banned);
            return;
        }

        // Login accepted — send login response, enter PLAYING
        Logger::Info("Server", "Player '%ls' logged in (smallId=%d, gameMode=%d)",
                     m_playerName.c_str(), m_smallId, m_gameMode);
        m_entityId  = (int)m_smallId * 100 + 1;
        // Use the server-authoritative gameMode (what we send in the login response)
        // rather than pkt.gameType (what the client reported — it may be stale from
        // a previous session). This keeps m_gameMode in sync with what the client
        // will actually be in after receiving our LoginResponse.
        m_gameMode  = m_world ? m_world->GetGameMode() : m_config->gamemode;
        SendLoginResponse();
        m_state = ConnectionState::Playing;
        m_wasPlaying = true;

        // Set initial position from spawn
        if (m_world)
        {
            m_x = (double)m_world->GetSpawnX() + 0.5;
            m_y = (double)m_world->ResolveSpawnFeetY() + kSpawnFeetEpsilon;
            m_z = (double)m_world->GetSpawnZ() + 0.5;
        }

        // Notify ConnectionManager for join broadcast
        if (onPlayerJoined)
            onPlayerJoined(this);
    }

    void Connection::SendLoginResponse()
    {
        // Build server->client LoginPacket
        std::wstring typeName = L"flat";
        if (m_config->levelType == "default") typeName = L"default";

        int entityId = (int)m_smallId * 100 + 1;
        int64_t seed = m_world ? m_world->GetSeed() : 0;
        int gameMode = m_world ? m_world->GetGameMode() : m_config->gamemode;
        int difficulty = m_world ? m_world->GetDifficulty() : m_config->difficulty;

        auto loginPkt = PacketHandler::WriteLoginResponse(
            L"",
            entityId,
            typeName,
            seed,
            gameMode,
            0,                      // dimension (overworld)
            (uint8_t)LEGACY_WORLD_HEIGHT,
            (uint8_t)m_config->maxPlayers,
            (int8_t)difficulty,
            0,                      // multiplayerInstanceId
            m_smallId,              // playerIndex
            true,                   // newSeaLevel
            0,                      // gamePrivileges
            864,                    // xzSize
            3                       // hellScale
        );
        SendPacket(loginPkt);

        // --- Full spawn sequence from PlayerList::placeNewPlayer ---
        SendSpawnSequence();
    }

    void Connection::HandleKeepAlive(const uint8_t* data, int size)
    {
        // Just acknowledge — reset timeout counter
    }

    // ---------------------------------------------------------------
    // HandleMovePlayer — track player position + fall damage
    // ---------------------------------------------------------------
    void Connection::HandleMovePlayer(int packetId,
        const uint8_t* data, int size)
    {
        PacketHandler::MovePlayerData move;
        if (!PacketHandler::ReadMovePlayer(packetId, data, size, move))
            return;

        if (move.hasPos)
        {
            // Fall damage tracking (matches LivingEntity::travel logic)
            // Only accumulate when survival, not flying, not dead, not in grace period
            if (!m_isDead && m_gameMode != 1 /*creative*/)
            {
                if (m_respawnGraceTicks > 0)
                {
                    m_respawnGraceTicks--;
                    m_fallDistance = 0.0f;
                    m_wasOnGround  = move.onGround;
                    m_prevY        = move.y;
                }
                else
                {
                    double dy = move.y - m_prevY;
                    bool onGround = move.onGround;

                    if (!move.isFlying)
                    {
                        if (dy < 0.0 && !m_wasOnGround)
                            m_fallDistance += (float)(-dy);

                        if (onGround && !m_wasOnGround)
                        {
                            if (m_fallDistance > 3.0f)
                                ApplyDamage(m_fallDistance - 3.0f);
                            m_fallDistance = 0.0f;
                        }

                        if (onGround)
                            m_fallDistance = 0.0f;
                    }
                    else
                    {
                        m_fallDistance = 0.0f;
                    }

                    m_wasOnGround = onGround;
                    m_prevY = move.y;
                }
            }

            m_x = move.x;
            m_y = move.y;
            m_z = move.z;
        }
        if (move.hasRot)
        {
            m_yRot = move.yRot;
            m_xRot = move.xRot;
        }

        if (move.hasPos)
        {
            int cx = static_cast<int>(std::floor(m_x / 16.0));
            int cz = static_cast<int>(std::floor(m_z / 16.0));
            if (cx != m_lastChunkX || cz != m_lastChunkZ)
            {
                m_lastChunkX = cx;
                m_lastChunkZ = cz;
                StreamChunksAround(cx, cz, false);
            }
        }

        // P2: broadcast updated position/rotation to all other players
        if (move.hasPos || move.hasRot)
        {
            if (onPlayerMoved)
                onPlayerMoved(this);
        }
    }

    // ---------------------------------------------------------------
    // HandleChat — process incoming chat from client
    // ---------------------------------------------------------------
    void Connection::HandleChat(const uint8_t* data, int size)
    {
        PacketHandler::ChatData chat;
        if (!PacketHandler::ReadChat(data, size, chat))
            return;

        if (chat.stringArgs.empty()) return;

        // Custom chat messages (type 0) from the client contain
        // the raw message text. Format it and relay.
        if (chat.messageType == ChatMessageType::Custom)
        {
            std::wstring msg = chat.stringArgs[0];
            if (msg.empty()) return;

            // Build "<PlayerName> message" format
            std::wstring formatted = L"<" + m_playerName + L"> " + msg;

            Logger::Info("Chat", "<%ls> %ls",
                m_playerName.c_str(), msg.c_str());

            // Relay to ConnectionManager for broadcast
            if (onPlayerChat)
                onPlayerChat(this, formatted);
        }
    }

    // =============================================================
    // SendSpawnSequence — post-login packets matching
    // PlayerList::placeNewPlayer from the source
    // =============================================================
    void Connection::SendSpawnSequence()
    {
        // Get spawn from world or use defaults
        int spawnX = m_world ? m_world->GetSpawnX() : 0;
        int spawnY = m_world ? m_world->ResolveSpawnFeetY() : 5;
        int spawnZ = m_world ? m_world->GetSpawnZ() : 0;
        int64_t gameTime = m_world ? m_world->GetGameTime() : 0;
        int64_t dayTime  = m_world ? m_world->GetDayTime() : 0;
        int gameMode = m_world ? m_world->GetGameMode() : m_config->gamemode;

        // 1. SetSpawnPosition (id=6)
        SendPacket(PacketHandler::WriteSetSpawnPosition(
            spawnX, spawnY, spawnZ));

        // 2. PlayerAbilities (id=202)
        bool creative = (gameMode == 1);
        SendPacket(PacketHandler::WritePlayerAbilities(
            creative, false, creative, creative,
            0.05f, 0.1f));

        // 3. SetCarriedItem (id=16) — slot 0
        SendPacket(PacketHandler::WriteSetCarriedItem(0));

        // 4. Player inventory sync (id=104 + carried slot via id=103)
        SendInventorySnapshot();

        // 5. SetTime (id=4)
        SendPacket(PacketHandler::WriteSetTime(gameTime, dayTime));

        // 6. GameEvent STOP_RAINING (id=70)
        SendPacket(PacketHandler::WriteGameEvent(2, 0));

        // 7. Chunks
        SendSpawnChunks();

        // 8. Teleport to spawn
        double sy = (double)spawnY + kSpawnFeetEpsilon;
        SendPacket(PacketHandler::WriteMovePlayerPosRot(
            (double)spawnX + 0.5, sy + 1.62, sy,
            (double)spawnZ + 0.5,
            0.0f, 0.0f, true, false));

        // 9. SetHealth — send initial health so the HUD shows correctly
        SendPacket(PacketHandler::WriteSetHealth(
            m_health, m_food, m_saturation));

        Logger::Info("Server",
            "Sent spawn sequence to '%ls' at spawnY=%d playerY=%.2f pos=(%d, %d, %d)",
            m_playerName.c_str(), spawnY, sy + 1.62, spawnX, spawnY, spawnZ);
    }

    void Connection::SendInventorySnapshot()
    {
        std::vector<ItemInstanceData> menuItems(45);

        // 0=result, 1-4=2x2 crafting input stay empty until native container logic exists.
        for (int armorIndex = 0; armorIndex < 4; ++armorIndex)
        {
            int menuSlot = ArmorIndexToMenuSlot(armorIndex);
            if (menuSlot >= 0)
                menuItems[menuSlot] = m_armorItems[armorIndex];
        }

        for (int inventoryIndex = 0; inventoryIndex < 36; ++inventoryIndex)
        {
            int menuSlot = InventoryIndexToMenuSlot(inventoryIndex);
            if (menuSlot >= 0)
                menuItems[menuSlot] = m_inventoryItems[inventoryIndex];
        }

        SendPacket(PacketHandler::WriteContainerSetContent(0, menuItems));
        SendPacket(PacketHandler::WriteContainerSetSlot(-1, -1, m_carriedItem));

        if (m_openContainerType == ContainerType::Workbench)
            SendWorkbenchSnapshot();
    }

    void Connection::SendWorkbenchSnapshot()
    {
        if (m_openContainerType != ContainerType::Workbench || m_openContainerId <= 0)
            return;

        std::vector<ItemInstanceData> menuItems(46);
        for (int inventoryIndex = 0; inventoryIndex < 36; ++inventoryIndex)
        {
            int menuSlot = InventoryIndexToWorkbenchMenuSlot(inventoryIndex);
            if (menuSlot >= 0)
                menuItems[menuSlot] = m_inventoryItems[inventoryIndex];
        }

        SendPacket(PacketHandler::WriteContainerSetContent(
            static_cast<int8_t>(m_openContainerId), menuItems));
        SendPacket(PacketHandler::WriteContainerSetSlot(-1, -1, m_carriedItem));
    }

    void Connection::SendInventorySlotUpdate(int inventoryIndex)
    {
        int menuSlot = InventoryIndexToMenuSlot(inventoryIndex);
        if (menuSlot < 0) return;
        SendPacket(PacketHandler::WriteContainerSetSlot(
            0, static_cast<int16_t>(menuSlot), m_inventoryItems[inventoryIndex]));

        if (m_openContainerType == ContainerType::Workbench)
        {
            int workbenchSlot = InventoryIndexToWorkbenchMenuSlot(inventoryIndex);
            if (workbenchSlot >= 0)
            {
                SendPacket(PacketHandler::WriteContainerSetSlot(
                    static_cast<int8_t>(m_openContainerId),
                    static_cast<int16_t>(workbenchSlot),
                    m_inventoryItems[inventoryIndex]));
            }
        }
    }

    void Connection::SendArmorSlotUpdate(int armorIndex)
    {
        int menuSlot = ArmorIndexToMenuSlot(armorIndex);
        if (menuSlot < 0) return;
        SendPacket(PacketHandler::WriteContainerSetSlot(
            0, static_cast<int16_t>(menuSlot), m_armorItems[armorIndex]));
    }

    void Connection::OpenWorkbenchContainer(int x, int y, int z)
    {
        if (m_openContainerType == ContainerType::Workbench)
            return;

        CloseContainerIfOpen();

        m_openContainerId = m_nextContainerId;
        m_nextContainerId = (m_nextContainerId % 100) + 1;
        m_openContainerType = ContainerType::Workbench;
        m_openWorkbenchX = x;
        m_openWorkbenchY = y;
        m_openWorkbenchZ = z;

        SendPacket(PacketHandler::WriteContainerOpen(
            static_cast<int8_t>(m_openContainerId),
            static_cast<int8_t>(ContainerType::Workbench),
            9,
            false));
        SendWorkbenchSnapshot();

        Logger::Info("Server",
            "'%ls' opened workbench at (%d,%d,%d) containerId=%d",
            m_playerName.c_str(), x, y, z, m_openContainerId);
    }

    void Connection::CloseContainerIfOpen()
    {
        if (m_openContainerId <= 0)
            return;

        Logger::Info("Server",
            "'%ls' closed containerId=%d type=%d",
            m_playerName.c_str(), m_openContainerId, m_openContainerType);

        m_openContainerId = 0;
        m_openContainerType = -1;
    }

    // =============================================================
    // SendSpawnChunks — send flat chunks around spawn
    // Uses ChunkVisibility + BlockRegionUpdate for each chunk
    // =============================================================
    void Connection::SendSpawnChunks()
    {
        int spawnCX = m_world ? (m_world->GetSpawnX() >> 4) : 0;
        int spawnCZ = m_world ? (m_world->GetSpawnZ() >> 4) : 0;
        m_lastChunkX = spawnCX;
        m_lastChunkZ = spawnCZ;
        StreamChunksAround(spawnCX, spawnCZ, true);
    }

    void Connection::ReleaseVisibleChunks(bool sendHidePackets)
    {
        if (m_visibleChunks.empty())
            return;

        for (int64_t key : m_visibleChunks)
        {
            int cx = static_cast<int>(key >> 32);
            int cz = static_cast<int>(static_cast<uint32_t>(key));
            if (sendHidePackets)
                SendPacket(PacketHandler::WriteChunkVisibility(cx, cz, false));
            if (onChunkVisibility)
                onChunkVisibility(cx, cz, -1);
        }

        m_visibleChunks.clear();
    }

    void Connection::StreamChunksAround(
        int centerCX, int centerCZ, bool fullResync)
    {
        if (fullResync)
        {
            ReleaseVisibleChunks(true);
        }

        std::unordered_set<int64_t> wanted;
        wanted.reserve((m_chunkRadius * 2 + 1) * (m_chunkRadius * 2 + 1));

        std::vector<std::pair<int, int>> toSend;
        toSend.reserve((m_chunkRadius * 2 + 1) * (m_chunkRadius * 2 + 1));

        std::unordered_set<int64_t> toSendKeys;
        toSendKeys.reserve((m_chunkRadius * 2 + 1) * (m_chunkRadius * 2 + 1));

        // Build the visible window — collect all chunks in the square window.
        for (int cx = centerCX - m_chunkRadius;
            cx <= centerCX + m_chunkRadius; cx++)
        {
            for (int cz = centerCZ - m_chunkRadius;
                cz <= centerCZ + m_chunkRadius; cz++)
            {
                int64_t key = MakeChunkKey(cx, cz);
                wanted.insert(key);
                if (m_visibleChunks.find(key) != m_visibleChunks.end())
                    continue;

                toSend.push_back({ cx, cz });
                toSendKeys.insert(key);
            }
        }

        // Sort new chunks nearest-first (spiral outward from player).
        // This matches Java Edition's chunk loading order and makes the
        // world fill in around the player rather than in raster lines.
        std::sort(toSend.begin(), toSend.end(),
            [&](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                int adx = a.first - centerCX, adz = a.second - centerCZ;
                int bdx = b.first - centerCX, bdz = b.second - centerCZ;
                return (adx*adx + adz*adz) < (bdx*bdx + bdz*bdz);
            });

        int chunksResent = 0; // reserved for seam-refresh resends (future use)

        // NOTE: We no longer preload+refresh chunks here synchronously.
        // That was the source of the 8-second login stall — loading all 289
        // chunks before even sending the spawn sequence. Loading and lighting
        // now happens lazily in DrainChunkQueue, one chunk per drain cycle.

        // On login (fullResync), send one ChunkVisibilityArea packet covering
        // the full window instead of 81 individual ChunkVisibility(true) packets.
        if (fullResync && !toSend.empty())
        {
            SendPacket(PacketHandler::WriteChunkVisibilityArea(
                centerCX - m_chunkRadius, centerCX + m_chunkRadius,
                centerCZ - m_chunkRadius, centerCZ + m_chunkRadius));
        }

        // Enqueue new chunks (already sorted nearest-first).
        // DrainChunkQueue() sends CHUNK_SEND_PER_TICK of them each tick.
        // On fullResync (login/respawn) flush the old queue first so stale
        // entries from a previous position don't clog the new window.
        if (fullResync) m_chunkSendQueue.clear();
        for (const auto& pos : toSend)
            m_chunkSendQueue.push_back(pos);

        int chunksSent = (int)toSend.size(); // queued, not yet sent — for log

        int chunksHidden = 0;
        std::vector<int64_t> toHide;
        toHide.reserve(m_visibleChunks.size());
        for (int64_t key : m_visibleChunks)
        {
            if (wanted.find(key) == wanted.end())
                toHide.push_back(key);
        }

        // Also cancel any queued-but-not-yet-sent chunks that are no longer wanted.
        // Without this, DrainChunkQueue would try to send chunks that have already
        // been evicted from the world cache by UnloadChunk below.
        m_chunkSendQueue.erase(
            std::remove_if(m_chunkSendQueue.begin(), m_chunkSendQueue.end(),
                [&](const std::pair<int,int>& p) {
                    return wanted.find(MakeChunkKey(p.first, p.second)) == wanted.end();
                }),
            m_chunkSendQueue.end());

        for (int64_t key : toHide)
        {
            int cx = static_cast<int>(key >> 32);
            int cz = static_cast<int>(static_cast<uint32_t>(key));
            SendPacket(PacketHandler::WriteChunkVisibility(cx, cz, false));
            m_visibleChunks.erase(key);
            // Notify ConnectionManager so it can decrement the ref-count
            // and call UnloadChunk only when no other player still sees it.
            if (onChunkVisibility)
                onChunkVisibility(cx, cz, -1);
            chunksHidden++;
        }

        if (chunksSent > 0 || chunksHidden > 0)
        {
            Logger::Debug("Server",
                "Chunk stream: '%ls' center=(%d,%d) queued+%d -%d hidden",
                m_playerName.c_str(), centerCX, centerCZ,
                chunksSent, chunksHidden);
        }
    }

    // ---------------------------------------------------------------
    // DrainChunkQueue — send up to m_chunksPerTick chunks per tick.
    // Chunks are pre-sorted nearest-first so the world fills in as a
    // growing circle. Load + lighting refresh happens here lazily so
    // login is instant and the server never blocks on bulk preloads.
    // ---------------------------------------------------------------
    void Connection::DrainChunkQueue()
    {
        if (m_chunkSendQueue.empty()) return;

        int sent = 0;
        while (!m_chunkSendQueue.empty() && sent < m_chunksPerTick)
        {
            auto [cx, cz] = m_chunkSendQueue.front();
            m_chunkSendQueue.erase(m_chunkSendQueue.begin());

            // Skip if already visible (e.g. enqueued twice due to movement)
            int64_t key = MakeChunkKey(cx, cz);
            if (m_visibleChunks.find(key) != m_visibleChunks.end())
                continue;

            // Lazily load + refresh this chunk.
            // Preload the 8 immediate neighbours first so the lighting BFS
            // has cross-chunk context — avoids dark seam edges.
            if (m_world)
            {
                for (int nx = cx - 1; nx <= cx + 1; nx++)
                    for (int nz = cz - 1; nz <= cz + 1; nz++)
                        m_world->GetChunk(nx, nz); // no-op if cached
                m_world->RefreshChunkForSend(cx, cz);
            }

            int blockX = cx * 16;
            int blockZ = cz * 16;
            ChunkData* chunk = m_world ? m_world->GetChunk(cx, cz) : nullptr;

            SendPacket(PacketHandler::WriteChunkVisibility(cx, cz, true));

            if (!chunk || chunk->rawData.empty())
            {
                SendPacket(PacketHandler::WriteEmptyBlockRegionUpdate(
                    blockX, 0, blockZ, 16, 1, 16, 0, true));
            }
            else
            {
                SendPacket(PacketHandler::WriteBlockRegionUpdate(
                    blockX, 0, blockZ,
                    16, chunk->ys, 16,
                    0, true,
                    chunk->rawData.data(),
                    (int)chunk->rawData.size()));
            }

            m_visibleChunks.insert(key);
            // Notify ConnectionManager of the new visible chunk (+1 ref-count)
            if (onChunkVisibility)
                onChunkVisibility(cx, cz, +1);
            sent++;
        }
    }

    // ---------------------------------------------------------------
    // HandlePlayerAction — client breaking a block
    // Matches ServerPlayerGameMode::startDestroyBlock /
    // stopDestroyBlock from the reference source:
    //   Creative (m_gameMode==1): break immediately on START (action=0)
    //   Survival:                 break only on STOP (action=2)
    //   Abort (action=1):         no world mutation in either mode
    // ---------------------------------------------------------------
    void Connection::HandlePlayerAction(const uint8_t* data, int size)
    {
        PacketHandler::PlayerActionData act;
        if (!PacketHandler::ReadPlayerAction(data, size, act))
            return;

        Logger::Debug("Server", "PlayerAction '%ls' action=%d pos=(%d,%d,%d) face=%d",
            m_playerName.c_str(), act.action, act.x, act.y, act.z, act.face);
        Logger::Info("ACTION-DBG", "'%ls' action=%d pos=(%d,%d,%d) face=%d",
            m_playerName.c_str(), act.action, act.x, act.y, act.z, act.face);

        bool isCreative = (m_gameMode == 1);

        // Creative: break on START_DESTROY (action=0) — instant break
        // Survival: break on STOP_DESTROY  (action=2) — animation completed
        // LCE clients sometimes only surface ABORT for zero-destroy-time tiles
        // during rapid targeting changes, so treat ABORT as a valid break for
        // instant-break blocks to keep the authoritative world in sync.
        // All other actions (drop=3...) don't mutate the world.
        if (!m_world) return;
        if (act.y < 0 || act.y >= LEGACY_WORLD_HEIGHT) return;

        auto getWorldBlock = [&](int x, int y, int z, int& outBlockId, int& outBlockData)
        {
            outBlockId = 0;
            outBlockData = 0;
            if (!m_world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
                return false;

            ChunkData* existingChunk = m_world->GetChunk(x >> 4, z >> 4);
            if (!existingChunk || existingChunk->blocks.empty())
                return false;

            int lx = ((x % 16) + 16) % 16;
            int lz = ((z % 16) + 16) % 16;
            int idx = ChunkBlockIndex(lx, lz, y);
            if (idx < 0 || idx >= static_cast<int>(existingChunk->blocks.size()))
                return false;

            outBlockId = existingChunk->blocks[idx];
            if (!existingChunk->data.empty())
            {
                int nibbleIndex = idx >> 1;
                uint8_t packed = existingChunk->data[nibbleIndex];
                outBlockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
            }
            return true;
        };

        auto breakBlockAt = [&](int x, int y, int z)
        {
            int oldBlockId = 0;
            int oldBlockData = 0;
            if (!getWorldBlock(x, y, z, oldBlockId, oldBlockData) || oldBlockId == 0)
                return false;

            ChunkData* chunk = m_world->SetBlock(x, y, z, 0, 0);
            if (!chunk)
                return false;

            if (onBlockUpdate)
                onBlockUpdate(this, x, y, z, 0, 0, oldBlockId, oldBlockData);

            CleanupUnsupportedSupportSensitiveBlocksAround(
                m_world,
                x, y, z,
                [this](int blockX, int blockY, int blockZ, int newBlockId, int newBlockData, int oldBlockId, int oldBlockData)
                {
                    if (onBlockUpdate)
                        onBlockUpdate(this, blockX, blockY, blockZ, newBlockId, newBlockData, oldBlockId, oldBlockData);
                },
                [this](int dustX, int dustY, int dustZ, int previousData, int newData)
                {
                    if (onBlockUpdate)
                        onBlockUpdate(this, dustX, dustY, dustZ, 55, newData, 55, previousData);
                },
                [this](int dx, int dy, int dz, int expectedBlockId, int targetBlockId, int blockData, int delayTicks)
                {
                    m_pendingDiodeTransitions.erase(
                        std::remove_if(
                            m_pendingDiodeTransitions.begin(),
                            m_pendingDiodeTransitions.end(),
                            [&](const PendingDiodeTransition& pending)
                            {
                                return pending.x == dx && pending.y == dy && pending.z == dz;
                            }),
                        m_pendingDiodeTransitions.end());

                    PendingDiodeTransition pending;
                    pending.x = dx;
                    pending.y = dy;
                    pending.z = dz;
                    pending.expectedBlockId = expectedBlockId;
                    pending.targetBlockId = targetBlockId;
                    pending.blockData = blockData;
                    pending.ticksRemaining = delayTicks;
                    m_pendingDiodeTransitions.push_back(pending);
                });

            RefreshMinimalRedstoneAround(
                m_world,
                x, y, z,
                [this](int blockX, int blockY, int blockZ, int newBlockId, int newBlockData, int oldBlockId, int oldBlockData)
                {
                    if (onBlockUpdate)
                        onBlockUpdate(this, blockX, blockY, blockZ, newBlockId, newBlockData, oldBlockId, oldBlockData);
                },
                [this](int dustX, int dustY, int dustZ, int previousData, int newData)
                {
                    if (onBlockUpdate)
                        onBlockUpdate(this, dustX, dustY, dustZ, 55, newData, 55, previousData);
                },
                [this](int dx, int dy, int dz, int expectedBlockId, int targetBlockId, int blockData, int delayTicks)
                {
                    m_pendingDiodeTransitions.erase(
                        std::remove_if(
                            m_pendingDiodeTransitions.begin(),
                            m_pendingDiodeTransitions.end(),
                            [&](const PendingDiodeTransition& pending)
                            {
                                return pending.x == dx && pending.y == dy && pending.z == dz;
                            }),
                        m_pendingDiodeTransitions.end());

                    PendingDiodeTransition pending;
                    pending.x = dx;
                    pending.y = dy;
                    pending.z = dz;
                    pending.expectedBlockId = expectedBlockId;
                    pending.targetBlockId = targetBlockId;
                    pending.blockData = blockData;
                    pending.ticksRemaining = delayTicks;
                    m_pendingDiodeTransitions.push_back(pending);
                });
            return true;
        };

        int targetX = act.x;
        int targetY = act.y;
        int targetZ = act.z;

        int oldBlockId = 0;
        int oldBlockData = 0;
        bool hasTargetBlock = getWorldBlock(targetX, targetY, targetZ, oldBlockId, oldBlockData) && oldBlockId != 0;

        auto tryRetargetCandidate = [&](int candidateX, int candidateY, int candidateZ)
        {
            if (candidateY < 0 || candidateY >= LEGACY_WORLD_HEIGHT)
                return false;

            int candidateBlockId = 0;
            int candidateBlockData = 0;
            if (!getWorldBlock(candidateX, candidateY, candidateZ, candidateBlockId, candidateBlockData) ||
                candidateBlockId == 0 ||
                !IsRetargetableInstantBreakBlock(candidateBlockId))
            {
                return false;
            }

            if (hasTargetBlock && IsInstantBreakBlock(oldBlockId))
                return false;

            targetX = candidateX;
            targetY = candidateY;
            targetZ = candidateZ;
            oldBlockId = candidateBlockId;
            oldBlockData = candidateBlockData;
            hasTargetBlock = true;
            Logger::Info("ACTION-DBG",
                "'%ls' retarget break from (%d,%d,%d) to (%d,%d,%d) block=%d data=%d",
                m_playerName.c_str(),
                act.x, act.y, act.z,
                targetX, targetY, targetZ,
                oldBlockId, oldBlockData);
            return true;
        };

        const bool allowDirectAboveRetarget = (act.face == 1 || act.face == 255);
        if (allowDirectAboveRetarget &&
            act.y + 1 >= 0 && act.y + 1 < LEGACY_WORLD_HEIGHT)
        {
            if (!tryRetargetCandidate(act.x, act.y + 1, act.z) &&
                act.face >= 0 && act.face <= 5)
            {
                int adjacentX = act.x;
                int adjacentY = act.y;
                int adjacentZ = act.z;
                OffsetByFace(act.face, adjacentX, adjacentY, adjacentZ);

                if (!tryRetargetCandidate(adjacentX, adjacentY, adjacentZ))
                    tryRetargetCandidate(adjacentX, adjacentY + 1, adjacentZ);
            }
        }

        if (!hasTargetBlock)
        {
            Logger::Info("ACTION-DBG", "'%ls' action=%d targetState=air pos=(%d,%d,%d)",
                m_playerName.c_str(), act.action, targetX, targetY, targetZ);
            return;
        }

        const bool instantBreak = IsInstantBreakBlock(oldBlockId);
        const bool shouldBreak = (isCreative && act.action == 0) ||
                                 (!isCreative && act.action == 2) ||
                                 (!isCreative && instantBreak && (act.action == 0 || act.action == 1));
        const bool redstoneDebug = IsRedstoneDebugBlock(oldBlockId);

        if (redstoneDebug)
        {
            Logger::Info("REDSTONE-DBG",
                "'%ls' break action=%d block=%d data=%d instant=%d shouldBreak=%d pos=(%d,%d,%d)",
                m_playerName.c_str(),
                act.action,
                oldBlockId,
                oldBlockData,
                instantBreak ? 1 : 0,
                shouldBreak ? 1 : 0,
                targetX, targetY, targetZ);
        }

        if (!shouldBreak)
        {
            if (act.action == 1 || act.action == 2)
            {
                int wireBlockId = (oldBlockId == 0) ? 255 : oldBlockId;
                SendPacket(PacketHandler::WriteTileUpdate(
                    targetX, targetY, targetZ, wireBlockId, oldBlockData, 0));

                if (redstoneDebug)
                {
                    Logger::Info("REDSTONE-DBG",
                        "'%ls' resync action=%d block=%d data=%d pos=(%d,%d,%d)",
                        m_playerName.c_str(),
                        act.action,
                        oldBlockId,
                        oldBlockData,
                        targetX, targetY, targetZ);
                }
            }
            return;
        }

        const bool brokePrimary = breakBlockAt(targetX, targetY, targetZ);
        if (redstoneDebug)
        {
            int newBlockId = -1;
            int newBlockData = -1;
            bool hasBlockAfter = getWorldBlock(targetX, targetY, targetZ, newBlockId, newBlockData);
            Logger::Info("REDSTONE-DBG",
                "'%ls' break result broke=%d afterBlock=%d afterData=%d hasBlock=%d pos=(%d,%d,%d)",
                m_playerName.c_str(),
                brokePrimary ? 1 : 0,
                hasBlockAfter ? newBlockId : -1,
                hasBlockAfter ? newBlockData : -1,
                hasBlockAfter ? 1 : 0,
                targetX, targetY, targetZ);
        }

        if (oldBlockId == 64 || oldBlockId == 71) // doors
        {
            const bool isUpper = (oldBlockData & 0x8) != 0;
            const int otherY = isUpper ? (targetY - 1) : (targetY + 1);
            if (otherY >= 0 && otherY < LEGACY_WORLD_HEIGHT)
                breakBlockAt(targetX, otherY, targetZ);
        }
        else if (oldBlockId == 26) // bed
        {
            const bool isHead = (oldBlockData & 0x8) != 0;
            const int dir = oldBlockData & 0x3;
            int dx = 0;
            int dz = 0;
            if (dir == 0) dz = 1;
            else if (dir == 1) dx = -1;
            else if (dir == 2) dz = -1;
            else dx = 1;

            const int otherX = isHead ? (targetX - dx) : (targetX + dx);
            const int otherZ = isHead ? (targetZ - dz) : (targetZ + dz);
            breakBlockAt(otherX, targetY, otherZ);
        }

        Logger::Info("Server", "'%ls' broke block at (%d,%d,%d)",
            m_playerName.c_str(), targetX, targetY, targetZ);
    }

    // ---------------------------------------------------------------
    // HandleUseItem — client placing a block
    // face offsets: 0=-Y, 1=+Y, 2=-Z, 3=+Z, 4=-X, 5=+X
    // The target block coords (x,y,z) are the block being clicked ON.
    // We offset by face to get the block being PLACED.
    // ---------------------------------------------------------------
    // Returns true for blocks the client treats as replaceable
    // (clicking them places INTO the same position, not adjacent).
    // face == -1 (0xFF) is the wire signal for this.
    static bool IsReplaceableBlock(int blockId)
    {
        switch (blockId)
        {
        case 0:   // air
        case 31:  // tall grass
        case 32:  // dead shrub
        case 37:  // dandelion
        case 38:  // rose/poppy
        case 39:  // brown mushroom
        case 40:  // red mushroom
        case 59:  // wheat crops
        case 78:  // snow layer
        case 83:  // sugar cane
        case 106: // vine
        case 175: // double plant (sunflower etc)
            return true;
        default:
            return false;
        }
    }

    void Connection::HandleUseItem(const uint8_t* data, int size)
    {
        PacketHandler::UseItemData use;
        if (!PacketHandler::ReadUseItem(data, size, use))
            return;

        if (m_openContainerId > 0)
            CloseContainerIfOpen();

        if (m_world)
        {
            auto sendActualTileState = [this](int x, int y, int z)
            {
                if (!m_world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
                    return;

                int blockId = 0;
                int blockData = 0;
                ChunkData* chunk = m_world->GetChunk(x >> 4, z >> 4);
                if (chunk && !chunk->blocks.empty())
                {
                    int lx = ((x % 16) + 16) % 16;
                    int lz = ((z % 16) + 16) % 16;
                    int idx = ChunkBlockIndex(lx, lz, y);
                    if (idx >= 0 && idx < static_cast<int>(chunk->blocks.size()))
                    {
                        blockId = chunk->blocks[idx];
                        if (!chunk->data.empty())
                        {
                            int nibbleIndex = idx >> 1;
                            uint8_t packed = chunk->data[nibbleIndex];
                            blockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
                        }
                    }
                }

                int wireBlockId = (blockId == 0) ? 255 : blockId;
                SendPacket(PacketHandler::WriteTileUpdate(x, y, z, wireBlockId, blockData, 0));
            };

            auto resyncPlacementPrediction = [&]()
            {
                sendActualTileState(use.x, use.y, use.z);

                int px = use.x;
                int py = use.y;
                int pz = use.z;
                bool hasPredictedTarget = false;

                if (use.face == -1)
                {
                    hasPredictedTarget = true;
                }
                else
                {
                    static const int dx[] = { 0,  0,  0,  0, -1, 1 };
                    static const int dy[] = {-1,  1,  0,  0,  0, 0 };
                    static const int dz[] = { 0,  0, -1,  1,  0, 0 };

                    const int face = use.face & 0xFF;
                    if (face <= 5)
                    {
                        ChunkData* clickedChunk = m_world->GetChunk(use.x >> 4, use.z >> 4);
                        bool clickedReplaceable = false;
                        if (clickedChunk && !clickedChunk->blocks.empty() &&
                            use.y >= 0 && use.y < LEGACY_WORLD_HEIGHT)
                        {
                            int lx = ((use.x % 16) + 16) % 16;
                            int lz = ((use.z % 16) + 16) % 16;
                            int idx = ChunkBlockIndex(lx, lz, use.y);
                            if (idx >= 0 && idx < static_cast<int>(clickedChunk->blocks.size()))
                                clickedReplaceable = IsReplaceableBlock(clickedChunk->blocks[idx]);
                        }

                        if (!clickedReplaceable)
                        {
                            px = use.x + dx[face];
                            py = use.y + dy[face];
                            pz = use.z + dz[face];
                        }
                        hasPredictedTarget = true;
                    }
                }

                if (hasPredictedTarget &&
                    (px != use.x || py != use.y || pz != use.z))
                {
                    sendActualTileState(px, py, pz);
                }
            };

            auto recomputeMinimalRedstoneAround = [&](int x, int y, int z)
            {
                RefreshMinimalRedstoneAround(
                    m_world,
                    x, y, z,
                    [this](int blockX, int blockY, int blockZ, int newBlockId, int newBlockData, int oldBlockId, int oldBlockData)
                    {
                        if (onBlockUpdate)
                            onBlockUpdate(this, blockX, blockY, blockZ, newBlockId, newBlockData, oldBlockId, oldBlockData);
                    },
                    [this](int dustX, int dustY, int dustZ, int oldData, int newData)
                    {
                        if (onBlockUpdate)
                            onBlockUpdate(this, dustX, dustY, dustZ, 55, newData, 55, oldData);
                    },
                    [this](int dx, int dy, int dz, int expectedBlockId, int targetBlockId, int blockData, int delayTicks)
                    {
                        m_pendingDiodeTransitions.erase(
                            std::remove_if(
                                m_pendingDiodeTransitions.begin(),
                                m_pendingDiodeTransitions.end(),
                                [&](const PendingDiodeTransition& pending)
                                {
                                    return pending.x == dx && pending.y == dy && pending.z == dz;
                                }),
                            m_pendingDiodeTransitions.end());

                        PendingDiodeTransition pending;
                        pending.x = dx;
                        pending.y = dy;
                        pending.z = dz;
                        pending.expectedBlockId = expectedBlockId;
                        pending.targetBlockId = targetBlockId;
                        pending.blockData = blockData;
                        pending.ticksRemaining = delayTicks;
                        m_pendingDiodeTransitions.push_back(pending);
                    });
            };

            auto clearPendingDiodeTransition = [&](int x, int y, int z)
            {
                m_pendingDiodeTransitions.erase(
                    std::remove_if(
                        m_pendingDiodeTransitions.begin(),
                        m_pendingDiodeTransitions.end(),
                        [&](const PendingDiodeTransition& pending)
                        {
                            return pending.x == x && pending.y == y && pending.z == z;
                        }),
                    m_pendingDiodeTransitions.end());
            };

            ChunkData* clickedChunk = m_world->GetChunk(use.x >> 4, use.z >> 4);
            if (clickedChunk && !clickedChunk->blocks.empty() &&
                use.y >= 0 && use.y < LEGACY_WORLD_HEIGHT)
            {
                int lx = ((use.x % 16) + 16) % 16;
                int lz = ((use.z % 16) + 16) % 16;
                int idx = ChunkBlockIndex(lx, lz, use.y);
                if (idx >= 0 && idx < static_cast<int>(clickedChunk->blocks.size()))
                {
                    if (clickedChunk->blocks[idx] == 58)
                    {
                        resyncPlacementPrediction();
                        OpenWorkbenchContainer(use.x, use.y, use.z);
                        return;
                    }

                    int clickedBlockId = clickedChunk->blocks[idx];
                    int clickedBlockData = 0;
                    if (!clickedChunk->data.empty())
                    {
                        int nibbleIndex = idx >> 1;
                        uint8_t packed = clickedChunk->data[nibbleIndex];
                        clickedBlockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
                    }

                    if (clickedBlockId == 93 || clickedBlockId == 94)
                    {
                        const int newBlockData = (((clickedBlockData & 0xC) + 0x4) & 0xC) | (clickedBlockData & 0x3);
                        if (m_world->SetBlock(use.x, use.y, use.z, clickedBlockId, newBlockData) && onBlockUpdate)
                            onBlockUpdate(this, use.x, use.y, use.z, clickedBlockId, newBlockData, clickedBlockId, clickedBlockData);
                        resyncPlacementPrediction();
                        return;
                    }

                    if (clickedBlockId == 149 || clickedBlockId == 150)
                    {
                        const int newBlockData = clickedBlockData ^ 0x4;
                        clearPendingDiodeTransition(use.x, use.y, use.z);

                        const int dir = GetMinimalDiodeDirection(newBlockData);
                        const int inputSignal = GetMinimalComparatorInputSignal(m_world, use.x, use.y, use.z, dir);
                        const int sideSignal = GetMinimalComparatorSideSignal(m_world, use.x, use.y, use.z, dir);
                        const bool shouldTurnOn =
                            inputSignal >= 15 || (inputSignal > 0 && (sideSignal == 0 || inputSignal >= sideSignal));
                        const int targetBlockId = shouldTurnOn ? 150 : 149;

                        if (m_world->SetBlock(use.x, use.y, use.z, targetBlockId, newBlockData))
                        {
                            if (onBlockUpdate)
                                onBlockUpdate(this, use.x, use.y, use.z, targetBlockId, newBlockData, clickedBlockId, clickedBlockData);
                            recomputeMinimalRedstoneAround(use.x, use.y, use.z);
                        }
                        resyncPlacementPrediction();
                        return;
                    }

                    if (clickedBlockId == 69)
                    {
                        const int newBlockData = clickedBlockData ^ 0x8;
                        if (m_world->SetBlock(use.x, use.y, use.z, clickedBlockId, newBlockData) && onBlockUpdate)
                        {
                            onBlockUpdate(this, use.x, use.y, use.z, clickedBlockId, newBlockData, clickedBlockId, clickedBlockData);
                            recomputeMinimalRedstoneAround(use.x, use.y, use.z);
                        }
                        resyncPlacementPrediction();
                        return;
                    }

                    if (clickedBlockId == 77 || clickedBlockId == 143)
                    {
                        if ((clickedBlockData & 0x8) == 0)
                        {
                            const int newBlockData = clickedBlockData | 0x8;
                            if (m_world->SetBlock(use.x, use.y, use.z, clickedBlockId, newBlockData) && onBlockUpdate)
                            {
                                onBlockUpdate(this, use.x, use.y, use.z, clickedBlockId, newBlockData, clickedBlockId, clickedBlockData);
                                recomputeMinimalRedstoneAround(use.x, use.y, use.z);
                            }

                            m_pendingButtonReleases.erase(
                                std::remove_if(
                                    m_pendingButtonReleases.begin(),
                                    m_pendingButtonReleases.end(),
                                    [&](const PendingButtonRelease& pending)
                                    {
                                        return pending.x == use.x &&
                                               pending.y == use.y &&
                                               pending.z == use.z;
                                    }),
                                m_pendingButtonReleases.end());

                            PendingButtonRelease pending;
                            pending.x = use.x;
                            pending.y = use.y;
                            pending.z = use.z;
                            pending.blockId = clickedBlockId;
                            pending.baseData = clickedBlockData & 0x7;
                            pending.ticksRemaining = (clickedBlockId == 143) ? 30 : 20;
                            m_pendingButtonReleases.push_back(pending);
                        }

                        resyncPlacementPrediction();
                        return;
                    }
                }
            }
        }

        const bool isCreative = (m_gameMode == 1);
        ItemInstanceData* selected = nullptr;
        if (m_hotbarSlot >= 0 &&
            m_hotbarSlot < static_cast<int>(m_inventoryItems.size()))
        {
            selected = &m_inventoryItems[m_hotbarSlot];
        }

        if (!isCreative)
        {
            if (!selected || selected->IsEmpty())
            {
                if (selected)
                    SendInventorySlotUpdate(m_hotbarSlot);
                if (m_world)
                    SendPacket(PacketHandler::WriteContainerSetSlot(-1, -1, m_carriedItem));
                return;
            }

            if (use.itemId < 0 ||
                selected->id != use.itemId ||
                selected->aux != use.itemDamage)
            {
                SendInventorySlotUpdate(m_hotbarSlot);
                if (m_world)
                {
                    auto sendActualTileState = [this](int x, int y, int z)
                    {
                        if (!m_world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
                            return;

                        int blockId = 0;
                        int blockData = 0;
                        ChunkData* chunk = m_world->GetChunk(x >> 4, z >> 4);
                        if (chunk && !chunk->blocks.empty())
                        {
                            int lx = ((x % 16) + 16) % 16;
                            int lz = ((z % 16) + 16) % 16;
                            int idx = ChunkBlockIndex(lx, lz, y);
                            if (idx >= 0 && idx < static_cast<int>(chunk->blocks.size()))
                            {
                                blockId = chunk->blocks[idx];
                                if (!chunk->data.empty())
                                {
                                    int nibbleIndex = idx >> 1;
                                    uint8_t packed = chunk->data[nibbleIndex];
                                    blockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
                                }
                            }
                        }

                        int wireBlockId = (blockId == 0) ? 255 : blockId;
                        SendPacket(PacketHandler::WriteTileUpdate(x, y, z, wireBlockId, blockData, 0));
                    };
                    sendActualTileState(use.x, use.y, use.z);
                }
                return;
            }
        }

        // itemId < 0 means no item / air - nothing to place
        if (use.itemId < 0) return;

        if (!m_world) return;

        auto sendActualTileState = [this](int x, int y, int z)
        {
            if (!m_world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
                return;

            int blockId = 0;
            int blockData = 0;
            ChunkData* chunk = m_world->GetChunk(x >> 4, z >> 4);
            if (chunk && !chunk->blocks.empty())
            {
                int lx = ((x % 16) + 16) % 16;
                int lz = ((z % 16) + 16) % 16;
                int idx = ChunkBlockIndex(lx, lz, y);
                if (idx >= 0 && idx < static_cast<int>(chunk->blocks.size()))
                {
                    blockId = chunk->blocks[idx];
                    if (!chunk->data.empty())
                    {
                        int nibbleIndex = idx >> 1;
                        uint8_t packed = chunk->data[nibbleIndex];
                        blockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
                    }
                }
            }

            int wireBlockId = (blockId == 0) ? 255 : blockId;
            SendPacket(PacketHandler::WriteTileUpdate(x, y, z, wireBlockId, blockData, 0));
        };

        auto resyncPlacementPrediction = [&]()
        {
            sendActualTileState(use.x, use.y, use.z);

            int px = use.x;
            int py = use.y;
            int pz = use.z;
            bool hasPredictedTarget = false;

            if (use.face == -1)
            {
                hasPredictedTarget = true;
            }
            else
            {
                static const int dx[] = { 0,  0,  0,  0, -1, 1 };
                static const int dy[] = {-1,  1,  0,  0,  0, 0 };
                static const int dz[] = { 0,  0, -1,  1,  0, 0 };

                int face = use.face & 0xFF;
                if (face <= 5)
                {
                    ChunkData* clickedChunk = m_world->GetChunk(use.x >> 4, use.z >> 4);
                    bool clickedReplaceable = false;
                    if (clickedChunk && !clickedChunk->blocks.empty() &&
                        use.y >= 0 && use.y < LEGACY_WORLD_HEIGHT)
                    {
                        int lx = ((use.x % 16) + 16) % 16;
                        int lz = ((use.z % 16) + 16) % 16;
                        int idx = ChunkBlockIndex(lx, lz, use.y);
                        if (idx >= 0 && idx < static_cast<int>(clickedChunk->blocks.size()))
                            clickedReplaceable = IsReplaceableBlock(clickedChunk->blocks[idx]);
                    }

                    if (!clickedReplaceable)
                    {
                        px = use.x + dx[face];
                        py = use.y + dy[face];
                        pz = use.z + dz[face];
                    }
                    hasPredictedTarget = true;
                }
            }

            if (hasPredictedTarget &&
                (px != use.x || py != use.y || pz != use.z))
            {
                sendActualTileState(px, py, pz);
            }
        };

        int px, py, pz;

        if (use.face == -1)
        {
            // face = -1: explicit replace-in-place signal
            px = use.x; py = use.y; pz = use.z;
        }
        else
        {
            static const int dx[] = { 0,  0,  0,  0, -1, 1 };
            static const int dy[] = {-1,  1,  0,  0,  0, 0 };
            static const int dz[] = { 0,  0, -1,  1,  0, 0 };

            int face = use.face & 0xFF;
            if (face > 5)
            {
                resyncPlacementPrediction();
                return;
            }

            // Check if the clicked block itself is replaceable.
            // If so, place INTO it rather than adjacent to it —
            // the client already did this visually (snow→cobblestone).
            ChunkData* clickedChunk = m_world->GetChunk(use.x >> 4, use.z >> 4);
            bool clickedReplaceable = false;
            if (clickedChunk && !clickedChunk->blocks.empty())
            {
                int lx = ((use.x % 16) + 16) % 16;
                int lz = ((use.z % 16) + 16) % 16;
                int idx = ChunkBlockIndex(lx, lz, use.y);
                if (idx >= 0 && idx < (int)clickedChunk->blocks.size())
                    clickedReplaceable = IsReplaceableBlock(clickedChunk->blocks[idx]);
            }

            if (clickedReplaceable)
            {
                px = use.x; py = use.y; pz = use.z;
            }
            else
            {
                px = use.x + dx[face];
                py = use.y + dy[face];
                pz = use.z + dz[face];
            }
        }

        auto getWorldBlockId = [&](int x, int y, int z) -> int
        {
            if (!m_world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
                return 0;

            ChunkData* chunk = m_world->GetChunk(x >> 4, z >> 4);
            if (!chunk || chunk->blocks.empty())
                return 0;

            int lx = ((x % 16) + 16) % 16;
            int lz = ((z % 16) + 16) % 16;
            int idx = ChunkBlockIndex(lx, lz, y);
            if (idx < 0 || idx >= static_cast<int>(chunk->blocks.size()))
                return 0;
            return chunk->blocks[idx];
        };

        auto getWorldBlockData = [&](int x, int y, int z) -> int
        {
            if (!m_world || y < 0 || y >= LEGACY_WORLD_HEIGHT)
                return 0;

            ChunkData* chunk = m_world->GetChunk(x >> 4, z >> 4);
            if (!chunk || chunk->data.empty())
                return 0;

            int lx = ((x % 16) + 16) % 16;
            int lz = ((z % 16) + 16) % 16;
            int idx = ChunkBlockIndex(lx, lz, y);
            if (idx < 0 || idx >= kChunkTotalBlocks)
                return 0;
            int nibbleIndex = idx >> 1;
            uint8_t packed = chunk->data[nibbleIndex];
            return (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
        };

        auto canReplaceAt = [&](int x, int y, int z) -> bool
        {
            int blockId = getWorldBlockId(x, y, z);
            return blockId == 0 || IsReplaceableBlock(blockId);
        };

        auto isTopSolidBlocking = [&](int x, int y, int z) -> bool
        {
            int blockId = getWorldBlockId(x, y, z);
            return IsTopSolidSupportBlock(blockId);
        };

        auto consumeSelectedPlacementItem = [&]()
        {
            if (isCreative || !selected)
                return;

            if (selected->count > 1)
                selected->count -= 1;
            else
                *selected = ItemInstanceData();

            SendInventorySlotUpdate(m_hotbarSlot);
        };

        auto notifyBlockPlaced = [&](int x, int y, int z, int newBlockId, int newBlockData, int oldBlockId, int oldBlockData)
        {
            if (onBlockUpdate)
                onBlockUpdate(this, x, y, z, newBlockId, newBlockData, oldBlockId, oldBlockData);
        };

        const int clickedBlockIdForPlacement = getWorldBlockId(use.x, use.y, use.z);
        const int clickedBlockDataForPlacement = getWorldBlockData(use.x, use.y, use.z);
        px = use.x;
        py = use.y;
        pz = use.z;

        if (use.face != -1)
        {
            const int face = use.face & 0xFF;
            bool resolvedTarget = false;

            if (use.itemId == 331)
            {
                resolvedTarget = ResolveRedstoneDustPlacementTarget(
                    clickedBlockIdForPlacement,
                    use.x,
                    use.y,
                    use.z,
                    face,
                    px,
                    py,
                    pz);
            }
            else
            {
                resolvedTarget = ResolveTilePlanterPlacementTarget(
                    clickedBlockIdForPlacement,
                    clickedBlockDataForPlacement,
                    use.x,
                    use.y,
                    use.z,
                    face,
                    px,
                    py,
                    pz);
            }

            if (!resolvedTarget)
            {
                resyncPlacementPrediction();
                return;
            }
        }

        if (use.itemId == 355) // bed
        {
            if (use.face != 1 || py < 0 || py >= LEGACY_WORLD_HEIGHT)
            {
                resyncPlacementPrediction();
                return;
            }

            const int dir = GetHorizontalDirectionFromPlayerYaw(m_yRot);
            int xra = 0;
            int zra = 0;
            if (dir == 0) zra = 1;
            else if (dir == 1) xra = -1;
            else if (dir == 2) zra = -1;
            else xra = 1;

            const int hx = px + xra;
            const int hy = py;
            const int hz = pz + zra;
            if (hy < 0 || hy >= LEGACY_WORLD_HEIGHT ||
                !canReplaceAt(px, py, pz) ||
                !canReplaceAt(hx, hy, hz) ||
                !isTopSolidBlocking(px, py - 1, pz) ||
                !isTopSolidBlocking(hx, hy - 1, hz))
            {
                resyncPlacementPrediction();
                sendActualTileState(hx, hy, hz);
                return;
            }

            const int oldFootId = getWorldBlockId(px, py, pz);
            const int oldFootData = getWorldBlockData(px, py, pz);
            const int oldHeadId = getWorldBlockId(hx, hy, hz);
            const int oldHeadData = getWorldBlockData(hx, hy, hz);

            if (!m_world->SetBlock(px, py, pz, 26, dir) ||
                !m_world->SetBlock(hx, hy, hz, 26, dir | 0x8))
            {
                resyncPlacementPrediction();
                sendActualTileState(hx, hy, hz);
                return;
            }

            Logger::Info("Server", "'%ls' placed bed at (%d,%d,%d) facing=%d",
                m_playerName.c_str(), px, py, pz, dir);

            consumeSelectedPlacementItem();
            notifyBlockPlaced(px, py, pz, 26, dir, oldFootId, oldFootData);
            notifyBlockPlaced(hx, hy, hz, 26, dir | 0x8, oldHeadId, oldHeadData);
            return;
        }

        if (use.itemId == 324 || use.itemId == 330) // wooden/iron door
        {
            if (use.face != 1 || py < 0 || py >= LEGACY_WORLD_HEIGHT - 1)
            {
                resyncPlacementPrediction();
                return;
            }

            const int doorBlockId = (use.itemId == 324) ? 64 : 71;
            if (!isTopSolidBlocking(px, py - 1, pz) ||
                !canReplaceAt(px, py, pz) ||
                !canReplaceAt(px, py + 1, pz))
            {
                resyncPlacementPrediction();
                sendActualTileState(px, py + 1, pz);
                return;
            }

            const int dir = GetDoorDirectionFromPlayerYaw(m_yRot);
            int xra = 0;
            int zra = 0;
            if (dir == 0) zra = 1;
            else if (dir == 1) xra = -1;
            else if (dir == 2) zra = -1;
            else xra = 1;

            const int solidLeft =
                (isTopSolidBlocking(px - xra, py, pz - zra) ? 1 : 0) +
                (isTopSolidBlocking(px - xra, py + 1, pz - zra) ? 1 : 0);
            const int solidRight =
                (isTopSolidBlocking(px + xra, py, pz + zra) ? 1 : 0) +
                (isTopSolidBlocking(px + xra, py + 1, pz + zra) ? 1 : 0);

            const bool doorLeft =
                (getWorldBlockId(px - xra, py, pz - zra) == doorBlockId) ||
                (getWorldBlockId(px - xra, py + 1, pz - zra) == doorBlockId);
            const bool doorRight =
                (getWorldBlockId(px + xra, py, pz + zra) == doorBlockId) ||
                (getWorldBlockId(px + xra, py + 1, pz + zra) == doorBlockId);

            bool flip = false;
            if (doorLeft && !doorRight)
                flip = true;
            else if (solidRight > solidLeft)
                flip = true;

            const int oldLowerId = getWorldBlockId(px, py, pz);
            const int oldLowerData = getWorldBlockData(px, py, pz);
            const int oldUpperId = getWorldBlockId(px, py + 1, pz);
            const int oldUpperData = getWorldBlockData(px, py + 1, pz);

            if (!m_world->SetBlock(px, py, pz, doorBlockId, dir) ||
                !m_world->SetBlock(px, py + 1, pz, doorBlockId, 0x8 | (flip ? 1 : 0)))
            {
                resyncPlacementPrediction();
                sendActualTileState(px, py + 1, pz);
                return;
            }

            Logger::Info("Server", "'%ls' placed door %d at (%d,%d,%d) dir=%d flip=%d",
                m_playerName.c_str(), doorBlockId, px, py, pz, dir, flip ? 1 : 0);

            consumeSelectedPlacementItem();
            notifyBlockPlaced(px, py, pz, doorBlockId, dir, oldLowerId, oldLowerData);
            notifyBlockPlaced(px, py + 1, pz, doorBlockId, 0x8 | (flip ? 1 : 0), oldUpperId, oldUpperData);
            return;
        }

        int placedBlockId = 0;
        int placedBlockData = 0;
        if (!TryResolvePlacedBlock(use.itemId, use.itemDamage, placedBlockId, placedBlockData))
        {
            resyncPlacementPrediction();
            return;
        }

        if (py < 0 || py >= LEGACY_WORLD_HEIGHT)
        {
            resyncPlacementPrediction();
            return;
        }

        if (placedBlockId == 17) // logs
        {
            placedBlockData = GetRotatedPillarDataFromFace(use.itemDamage, use.face);
        }
        else if (placedBlockId == 69) // lever
        {
            const int yawDir = (static_cast<int>(std::floor(m_yRot * 4.0 / 360.0 + 0.5))) & 1;
            int leverData = -1;
            if (use.face == 0 && IsTopSolidSupportBlock(getWorldBlockId(px, py + 1, pz)))
                leverData = yawDir == 0 ? 7 : 0;
            else if (use.face == 1 && isTopSolidBlocking(px, py - 1, pz))
                leverData = yawDir == 0 ? 5 : 6;
            else if (use.face == 2 && IsTopSolidSupportBlock(getWorldBlockId(px, py, pz + 1)))
                leverData = 4;
            else if (use.face == 3 && IsTopSolidSupportBlock(getWorldBlockId(px, py, pz - 1)))
                leverData = 3;
            else if (use.face == 4 && IsTopSolidSupportBlock(getWorldBlockId(px + 1, py, pz)))
                leverData = 2;
            else if (use.face == 5 && IsTopSolidSupportBlock(getWorldBlockId(px - 1, py, pz)))
                leverData = 1;

            if (leverData < 0)
            {
                resyncPlacementPrediction();
                return;
            }
            placedBlockData = leverData;
        }
        else if (placedBlockId == 77 || placedBlockId == 143) // buttons
        {
            int buttonData = -1;
            if (use.face == 2 && IsTopSolidSupportBlock(getWorldBlockId(px, py, pz + 1)))
                buttonData = 4;
            else if (use.face == 3 && IsTopSolidSupportBlock(getWorldBlockId(px, py, pz - 1)))
                buttonData = 3;
            else if (use.face == 4 && IsTopSolidSupportBlock(getWorldBlockId(px + 1, py, pz)))
                buttonData = 2;
            else if (use.face == 5 && IsTopSolidSupportBlock(getWorldBlockId(px - 1, py, pz)))
                buttonData = 1;

            if (buttonData < 0)
            {
                resyncPlacementPrediction();
                return;
            }
            placedBlockData = buttonData;
        }
        else if (placedBlockId == 29 || placedBlockId == 33) // sticky/normal piston
        {
            placedBlockData = GetPistonFacingForPlacement(px, py, pz, m_x, m_y, m_z, m_yRot);
        }
        else if (placedBlockId == 54 || placedBlockId == 146 || placedBlockId == 61 || placedBlockId == 62)
        {
            placedBlockData = GetFacingFromPlayerYaw(m_yRot);
        }
        else if (placedBlockId == 93 || placedBlockId == 149) // repeater/comparator
        {
            placedBlockData = GetDiodeDirectionFromPlayerYaw(m_yRot);
        }
        else if (placedBlockId == 50 || placedBlockId == 75 || placedBlockId == 76) // torches
        {
            int torchData = -1;
            if (use.face == 1 && isTopSolidBlocking(px, py - 1, pz))
                torchData = 5;
            else if (use.face == 2 && IsTopSolidSupportBlock(getWorldBlockId(px, py, pz + 1)))
                torchData = 4;
            else if (use.face == 3 && IsTopSolidSupportBlock(getWorldBlockId(px, py, pz - 1)))
                torchData = 3;
            else if (use.face == 4 && IsTopSolidSupportBlock(getWorldBlockId(px + 1, py, pz)))
                torchData = 2;
            else if (use.face == 5 && IsTopSolidSupportBlock(getWorldBlockId(px - 1, py, pz)))
                torchData = 1;

            if (torchData < 0)
            {
                resyncPlacementPrediction();
                return;
            }

            placedBlockData = torchData;
            if (placedBlockId == 76)
            {
                int supportX = 0;
                int supportY = 0;
                int supportZ = 0;
                if (TryGetMinimalTorchSupportPosition(torchData, px, py, pz, supportX, supportY, supportZ) &&
                    IsMinimalBlockPoweredAt(m_world, supportX, supportY, supportZ, px, py, pz))
                {
                    placedBlockId = 75;
                }
            }
        }

        if ((placedBlockId == 55 || placedBlockId == 93 || placedBlockId == 149) &&
            !isTopSolidBlocking(px, py - 1, pz))
        {
            resyncPlacementPrediction();
            return;
        }

        if (placedBlockId == 55)
        {
            const int existing = getWorldBlockId(px, py, pz);
            const bool replacingThinSnow =
                (px == use.x && py == use.y && pz == use.z && clickedBlockIdForPlacement == 78);
            if (existing != 0 && !replacingThinSnow)
            {
                resyncPlacementPrediction();
                return;
            }
        }

        // Don't overwrite a non-replaceable solid block
        ChunkData* destChunk = m_world->GetChunk(px >> 4, pz >> 4);
        int oldBlockId = 0;
        int oldBlockData = 0;
        if (destChunk && !destChunk->blocks.empty())
        {
            int lx = ((px % 16) + 16) % 16;
            int lz = ((pz % 16) + 16) % 16;
            int idx = ChunkBlockIndex(lx, lz, py);
            if (idx >= 0 && idx < (int)destChunk->blocks.size())
            {
                int existing = destChunk->blocks[idx];
                oldBlockId = existing;
                if (!destChunk->data.empty())
                {
                    int nibbleIndex = idx >> 1;
                    uint8_t packed = destChunk->data[nibbleIndex];
                    oldBlockData = (idx & 1) ? ((packed >> 4) & 0xF) : (packed & 0xF);
                }
                if (existing != 0 && !IsReplaceableBlock(existing))
                {
                    resyncPlacementPrediction();
                    return;
                }
            }
        }

        ChunkData* chunk = m_world->SetBlock(px, py, pz,
            placedBlockId, placedBlockData);
        if (!chunk)
        {
            resyncPlacementPrediction();
            return;
        }

        Logger::Info("Server", "'%ls' placed block %d at (%d,%d,%d)",
            m_playerName.c_str(), placedBlockId, px, py, pz);

        if (!isCreative && selected)
        {
            if (selected->count > 1)
            {
                selected->count -= 1;
            }
            else
            {
                *selected = ItemInstanceData();
            }
            SendInventorySlotUpdate(m_hotbarSlot);
        }

        if (onBlockUpdate)
            onBlockUpdate(this, px, py, pz, placedBlockId, placedBlockData, oldBlockId, oldBlockData);

        RefreshMinimalRedstoneAround(
            m_world,
            px, py, pz,
            [this](int blockX, int blockY, int blockZ, int newBlockId, int newBlockData, int oldBlockId, int oldBlockData)
            {
                if (onBlockUpdate)
                    onBlockUpdate(this, blockX, blockY, blockZ, newBlockId, newBlockData, oldBlockId, oldBlockData);
            },
            [this](int dustX, int dustY, int dustZ, int previousData, int newData)
            {
                if (onBlockUpdate)
                    onBlockUpdate(this, dustX, dustY, dustZ, 55, newData, 55, previousData);
            },
            [this](int dx, int dy, int dz, int expectedBlockId, int targetBlockId, int blockData, int delayTicks)
            {
                m_pendingDiodeTransitions.erase(
                    std::remove_if(
                        m_pendingDiodeTransitions.begin(),
                        m_pendingDiodeTransitions.end(),
                        [&](const PendingDiodeTransition& pending)
                        {
                            return pending.x == dx && pending.y == dy && pending.z == dz;
                        }),
                    m_pendingDiodeTransitions.end());

                PendingDiodeTransition pending;
                pending.x = dx;
                pending.y = dy;
                pending.z = dz;
                pending.expectedBlockId = expectedBlockId;
                pending.targetBlockId = targetBlockId;
                pending.blockData = blockData;
                pending.ticksRemaining = delayTicks;
                m_pendingDiodeTransitions.push_back(pending);
            });
    }

    // ---------------------------------------------------------------
    // HandleSetCarriedItem (id=16) client->server
    // Wire: [byte 16][short slot]
    // Mirrors PlayerConnection::handleSetCarriedItem in reference source:
    //   validates slot range, stores in player->inventory->selected.
    // We track m_hotbarSlot so UseItem and future EntityEquipment (P2)
    // can reference the player's currently selected hotbar slot.
    // ---------------------------------------------------------------
    void Connection::HandleSetCarriedItem(const uint8_t* data, int size)
    {
        if (size < 3) return; // [byte id][short slot]
        int16_t slot = (int16_t)((data[1] << 8) | data[2]);

        // Reference source: slot < 0 || slot >= Inventory::getSelectionSize() (9) → ignore
        if (slot < 0 || slot > 8) return;

        m_hotbarSlot = static_cast<int>(slot);
        Logger::Info("Server", "'%ls' hotbar slot -> %d",
            m_playerName.c_str(), m_hotbarSlot);
    }

    void Connection::HandleContainerClick(const uint8_t* data, int size)
    {
        PacketHandler::ContainerClickData click;
        if (!PacketHandler::ReadContainerClick(data, size, click))
            return;

        const bool isPlayerInventory = (click.containerId == 0);
        const bool isWorkbench = (m_openContainerType == ContainerType::Workbench &&
            click.containerId == m_openContainerId);

        if (!isPlayerInventory && !isWorkbench)
        {
            SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, false));
            SendInventorySnapshot();
            return;
        }

        if ((click.clickType != 0 && click.clickType != 1) ||
            (click.buttonNum != 0 && click.buttonNum != 1))
        {
            SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, false));
            SendInventorySnapshot();
            return;
        }

        if (click.slotNum == -999)
        {
            ItemInstanceData expected;
            if (!m_carriedItem.IsEmpty())
                expected = m_carriedItem;

            if (!ItemsMatch(expected, click.item))
            {
                SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, false));
                SendInventorySnapshot();
                return;
            }

            if (!m_carriedItem.IsEmpty())
            {
                if (click.buttonNum == 0)
                {
                    m_carriedItem = ItemInstanceData();
                }
                else
                {
                    if (m_carriedItem.count > 1)
                    {
                        m_carriedItem.count -= 1;
                    }
                    else
                    {
                        m_carriedItem = ItemInstanceData();
                    }
                }
            }

            SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, true));
            SendPacket(PacketHandler::WriteContainerSetSlot(-1, -1, m_carriedItem));
            return;
        }

        auto getSlotItem = [&](int slotNum) -> ItemInstanceData
        {
            if (isWorkbench)
            {
                int inventoryIndex = WorkbenchMenuSlotToInventoryIndex(slotNum);
                if (inventoryIndex >= 0)
                    return m_inventoryItems[inventoryIndex];
                return ItemInstanceData();
            }
            return GetMenuSlotItem(slotNum);
        };

        auto setSlotItem = [&](int slotNum, const ItemInstanceData& item)
        {
            if (isWorkbench)
            {
                int inventoryIndex = WorkbenchMenuSlotToInventoryIndex(slotNum);
                if (inventoryIndex >= 0)
                    m_inventoryItems[inventoryIndex] = item;
                return;
            }
            SetMenuSlotItem(slotNum, item);
        };

        auto slotToInventoryIndex = [&](int slotNum) -> int
        {
            return isWorkbench ? WorkbenchMenuSlotToInventoryIndex(slotNum) : MenuSlotToInventoryIndex(slotNum);
        };

        auto slotToArmorIndex = [&](int slotNum) -> int
        {
            return isWorkbench ? -1 : MenuSlotToArmorIndex(slotNum);
        };

        if (isWorkbench && click.slotNum >= 0 && slotToInventoryIndex(click.slotNum) < 0)
        {
            SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, false));
            SendWorkbenchSnapshot();
            return;
        }

        ItemInstanceData slotItem = getSlotItem(click.slotNum);
        ItemInstanceData clickedResult = slotItem;
        if (!ItemsMatch(clickedResult, click.item))
        {
            SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, false));
            SendInventorySnapshot();
            return;
        }

        if (click.clickType == 1)
        {
            bool changed = false;
            ItemInstanceData sourceItem = slotItem;

            auto moveIntoRange = [&](int startSlot, int endSlotExclusive) -> bool
            {
                bool moved = false;
                ItemInstanceData moving = getSlotItem(click.slotNum);
                if (moving.IsEmpty())
                    return false;

                for (int slot = startSlot; slot < endSlotExclusive && moving.count > 0; ++slot)
                {
                    if (slot == click.slotNum)
                        continue;

                    ItemInstanceData target = getSlotItem(slot);
                    if (target.IsEmpty() || target.id != moving.id || target.aux != moving.aux || target.count >= 64)
                        continue;

                    int space = 64 - target.count;
                    int transfer = moving.count < space ? moving.count : space;
                    target.count = static_cast<uint8_t>(target.count + transfer);
                    moving.count = static_cast<uint8_t>(moving.count - transfer);
                    setSlotItem(slot, target);
                    moved = true;
                }

                for (int slot = startSlot; slot < endSlotExclusive && moving.count > 0; ++slot)
                {
                    if (slot == click.slotNum)
                        continue;

                    ItemInstanceData target = getSlotItem(slot);
                    if (!target.IsEmpty())
                        continue;

                    setSlotItem(slot, moving);
                    moving = ItemInstanceData();
                    moved = true;
                }

                if (moved)
                    setSlotItem(click.slotNum, moving);

                return moved;
            };

            int inventoryIndex = slotToInventoryIndex(click.slotNum);
            int armorIndex = slotToArmorIndex(click.slotNum);

            if (inventoryIndex >= 0)
            {
                int armorMenuSlot = isWorkbench ? -1 : GetArmorMenuSlotForItemId(sourceItem.id);
                if (armorMenuSlot >= 0 && getSlotItem(armorMenuSlot).IsEmpty())
                {
                    setSlotItem(armorMenuSlot, sourceItem);
                    setSlotItem(click.slotNum, ItemInstanceData());
                    changed = true;
                }
                else if ((isWorkbench && click.slotNum >= 10 && click.slotNum < 37) ||
                         (!isWorkbench && click.slotNum >= 9 && click.slotNum < 36))
                {
                    changed = moveIntoRange(isWorkbench ? 37 : 36, isWorkbench ? 46 : 45);
                }
                else if ((isWorkbench && click.slotNum >= 37 && click.slotNum < 46) ||
                         (!isWorkbench && click.slotNum >= 36 && click.slotNum < 45))
                {
                    changed = moveIntoRange(isWorkbench ? 10 : 9, isWorkbench ? 37 : 36);
                }
            }
            else if (armorIndex >= 0)
            {
                changed = moveIntoRange(9, 45);
            }

            SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, true));
            if (changed)
                SendInventorySnapshot();
            return;
        }

        bool changedSlot = false;
        bool changedCarried = false;

        if (slotItem.IsEmpty())
        {
            if (!m_carriedItem.IsEmpty())
            {
                int moveCount = (click.buttonNum == 0) ? m_carriedItem.count : 1;
                ItemInstanceData placed = m_carriedItem;
                placed.count = static_cast<uint8_t>(moveCount);
                setSlotItem(click.slotNum, placed);
                changedSlot = true;

                if (m_carriedItem.count > moveCount)
                {
                    m_carriedItem.count = static_cast<uint8_t>(m_carriedItem.count - moveCount);
                }
                else
                {
                    m_carriedItem = ItemInstanceData();
                }
                changedCarried = true;
            }
        }
        else if (m_carriedItem.IsEmpty())
        {
            if (click.buttonNum == 0)
            {
                m_carriedItem = slotItem;
                setSlotItem(click.slotNum, ItemInstanceData());
            }
            else
            {
                int takeCount = (slotItem.count + 1) / 2;
                m_carriedItem = slotItem;
                m_carriedItem.count = static_cast<uint8_t>(takeCount);
                if (slotItem.count > takeCount)
                {
                    slotItem.count = static_cast<uint8_t>(slotItem.count - takeCount);
                    setSlotItem(click.slotNum, slotItem);
                }
                else
                {
                    setSlotItem(click.slotNum, ItemInstanceData());
                }
            }
            changedSlot = true;
            changedCarried = true;
        }
        else if (slotItem.id == m_carriedItem.id &&
                 slotItem.aux == m_carriedItem.aux &&
                 slotItem.count < 64)
        {
            int moveCount = (click.buttonNum == 0) ? m_carriedItem.count : 1;
            int space = 64 - slotItem.count;
            if (moveCount > space)
                moveCount = space;

            if (moveCount > 0)
            {
                slotItem.count = static_cast<uint8_t>(slotItem.count + moveCount);
                setSlotItem(click.slotNum, slotItem);
                changedSlot = true;

                if (m_carriedItem.count > moveCount)
                {
                    m_carriedItem.count = static_cast<uint8_t>(m_carriedItem.count - moveCount);
                }
                else
                {
                    m_carriedItem = ItemInstanceData();
                }
                changedCarried = true;
            }
        }
        else if (click.buttonNum == 0)
        {
            ItemInstanceData temp = slotItem;
            setSlotItem(click.slotNum, m_carriedItem);
            m_carriedItem = temp;
            changedSlot = true;
            changedCarried = true;
        }

        SendPacket(PacketHandler::WriteContainerAck(click.containerId, click.uid, true));

        if (changedSlot)
        {
            int inventoryIndex = slotToInventoryIndex(click.slotNum);
            if (inventoryIndex >= 0)
                SendInventorySlotUpdate(inventoryIndex);
            else
            {
                int armorIndex = slotToArmorIndex(click.slotNum);
                if (armorIndex >= 0)
                    SendArmorSlotUpdate(armorIndex);
            }
        }
        if (changedCarried)
            SendPacket(PacketHandler::WriteContainerSetSlot(-1, -1, m_carriedItem));
    }

    void Connection::HandleContainerAck(const uint8_t* data, int size)
    {
        (void)data;
        (void)size;
    }

    void Connection::HandleContainerClose(const uint8_t* data, int size)
    {
        if (size < 2)
            return;

        int containerId = static_cast<int>(data[1]);
        if (containerId == m_openContainerId)
            CloseContainerIfOpen();
    }

    void Connection::HandleCraftItem(const uint8_t* data, int size)
    {
        PacketHandler::CraftItemData craft;
        if (!PacketHandler::ReadCraftItem(data, size, craft))
            return;

        const CraftRecipeSpec* recipe = FindLegacyCraftingRecipe(craft.recipe);
        if (!recipe)
        {
            Logger::Info("Server",
                "'%ls' sent unsupported CraftItem uid=%d recipe=%d; resyncing inventory",
                m_playerName.c_str(), craft.uid, craft.recipe);
            SendInventorySnapshot();
            return;
        }

        for (const CraftIngredientSpec& ingredient : recipe->ingredients)
        {
            int have = CountInventoryResource(
                m_inventoryItems,
                ingredient.id,
                ingredient.aux,
                ingredient.anyAux);
            if (have < ingredient.count)
            {
                Logger::Info("Server",
                    "'%ls' lacked ingredients for CraftItem recipe=%d result=%d:%d x%d; resyncing inventory",
                    m_playerName.c_str(),
                    craft.recipe,
                    recipe->result.id,
                    recipe->result.aux,
                    recipe->result.count);
                SendInventorySnapshot();
                return;
            }
        }

        std::array<ItemInstanceData, 36> craftedInventory = m_inventoryItems;
        for (const CraftIngredientSpec& ingredient : recipe->ingredients)
        {
            if (!RemoveInventoryResource(
                    craftedInventory,
                    ingredient.id,
                    ingredient.aux,
                    ingredient.anyAux,
                    ingredient.count))
            {
                SendInventorySnapshot();
                return;
            }
        }

        std::vector<ItemInstanceData> remainders;
        for (const CraftIngredientSpec& ingredient : recipe->ingredients)
        {
            ItemInstanceData remainder = GetCraftingRemainingItem(ingredient.id);
            if (remainder.IsEmpty())
                continue;

            remainder.count = static_cast<uint8_t>(ingredient.count);
            remainders.push_back(remainder);
        }

        if (!TryInsertInventoryItem(craftedInventory, recipe->result))
        {
            Logger::Info("Server",
                "'%ls' had no room for CraftItem recipe=%d result=%d:%d x%d; resyncing inventory",
                m_playerName.c_str(),
                craft.recipe,
                recipe->result.id,
                recipe->result.aux,
                recipe->result.count);
            SendInventorySnapshot();
            return;
        }

        for (const ItemInstanceData& remainder : remainders)
        {
            if (!TryInsertInventoryItem(craftedInventory, remainder))
            {
                Logger::Info("Server",
                    "'%ls' had no room for CraftItem recipe=%d remainder=%d:%d x%d; resyncing inventory",
                    m_playerName.c_str(),
                    craft.recipe,
                    remainder.id,
                    remainder.aux,
                    remainder.count);
                SendInventorySnapshot();
                return;
            }
        }

        m_inventoryItems = craftedInventory;
        Logger::Info("Server",
            "'%ls' crafted recipe=%d -> %d:%d x%d",
            m_playerName.c_str(),
            craft.recipe,
            recipe->result.id,
            recipe->result.aux,
            recipe->result.count);
        SendInventorySnapshot();
    }

    // ---------------------------------------------------------------
    // HandleAnimate (id=18) client->server
    // Wire: [byte 18][int entityId][byte action]
    // action: 1=SWING, 2=HURT, 3=WAKE_UP, 4=RESPAWN, 5=EAT, etc.
    // Server: broadcast arm swing (action=1) to all other clients.
    // ---------------------------------------------------------------
    void Connection::HandleAnimate(const uint8_t* data, int size)
    {
        // Minimum: [byte id][int entityId][byte action] = 6 bytes
        if (size < 6) return;
        // data[1..4] = entityId (client sends its own entity id — ignore)
        uint8_t action = data[5];

        // Only broadcast arm swing; other animate actions (hurt/death)
        // are generated server-side and sent explicitly.
        if (action == 1 /*SWING*/ && onAnimateBroadcast)
            onAnimateBroadcast(this, action);
    }

    // ---------------------------------------------------------------
    // HandlePlayerCommand (id=19) client->server
    // Wire: [byte 19][int entityId][byte action][int data]
    // action: 1=START_SNEAKING, 2=STOP_SNEAKING, 3=STOP_SLEEPING,
    //         4=START_SPRINTING, 5=STOP_SPRINTING, 6=START_IDLEANIM,
    //         7=STOP_IDLEANIM, 8=RIDING_JUMP, 9=OPEN_INVENTORY
    // Server-side in reference: updates player sneak/sprint flags.
    // We'll track sneak/sprint state properly in P2; for now consume
    // the packet to silence "Unhandled" log spam.
    // ---------------------------------------------------------------
    void Connection::HandlePlayerCommand(const uint8_t* data, int size)
    {
        // Silently accepted — sneak/sprint state tracking added in P2
        (void)data; (void)size;
    }

    // ---------------------------------------------------------------
    // SendSetHealth — send SetHealth packet to this client
    // ---------------------------------------------------------------
    void Connection::SendSetHealth(float health, int16_t food, float sat)
    {
        SendPacket(PacketHandler::WriteSetHealth(health, food, sat));
    }

    // ---------------------------------------------------------------
    // ApplyDamage — reduce health, send SetHealth, handle death
    // Called by fall damage and future damage sources.
    // ---------------------------------------------------------------
    void Connection::ApplyDamage(float amount)
    {
        if (m_isDead || amount <= 0.0f) return;

        m_health -= amount;
        if (m_health < 0.0f) m_health = 0.0f;

        Logger::Info("Server",
            "'%ls' took %.1f damage, health now %.1f",
            m_playerName.c_str(), amount, m_health);

        if (m_health <= 0.0f)
        {
            m_health = 0.0f;
            m_isDead = true;
            Logger::Info("Server", "'%ls' died — m_isDead=true, sending SetHealth(0)+Death",
                m_playerName.c_str());
            SendPacket(PacketHandler::WriteSetHealth(0.0f, m_food, m_saturation));
            // EntityEvent id=3: death animation shown to *this* client (self)
            SendPacket(PacketHandler::WriteEntityEvent(
                m_entityId, EntityEventId::Death));
            Logger::Info("Server", "'%ls' died", m_playerName.c_str());
        }
        else
        {
            SendPacket(PacketHandler::WriteSetHealth(m_health, m_food, m_saturation));
            // EntityEvent id=2: hurt flash shown to *this* client (self)
            SendPacket(PacketHandler::WriteEntityEvent(
                m_entityId, EntityEventId::Hurt));
        }
    }

    bool Connection::GiveItem(int itemId, int amount, int aux)
    {
        if (itemId < 0 || amount <= 0)
            return false;

        std::vector<int> touchedSlots;
        int remaining = amount;

        for (int pass = 0; pass < 2 && remaining > 0; ++pass)
        {
            for (int i = 0; i < static_cast<int>(m_inventoryItems.size()) && remaining > 0; ++i)
            {
                ItemInstanceData& slot = m_inventoryItems[i];

                if (pass == 0)
                {
                    if (slot.IsEmpty() || slot.id != itemId || slot.aux != aux || slot.count >= 64)
                        continue;
                }
                else
                {
                    if (!slot.IsEmpty())
                        continue;
                    slot.id = static_cast<int16_t>(itemId);
                    slot.aux = static_cast<int16_t>(aux);
                    slot.count = 0;
                }

                int space = 64 - slot.count;
                if (space <= 0)
                    continue;

                int toAdd = (space < remaining) ? space : remaining;
                slot.count = static_cast<uint8_t>(slot.count + toAdd);
                remaining -= toAdd;
                touchedSlots.push_back(i);
            }
        }

        for (int slotIndex : touchedSlots)
            SendInventorySlotUpdate(slotIndex);

        return remaining == 0;
    }

    // ---------------------------------------------------------------
    // HandleRespawn (id=9) client->server
    // In this version of LCE, the client sends ClientCommandPacket
    // (id=205, action=1) for respawn — not RespawnPacket (id=9).
    // This handler is kept as a stub for dimension-change respawn paths.
    // ---------------------------------------------------------------
    void Connection::HandleRespawn(const uint8_t* data, int size)
    {
        // No-op for now — respawn is handled via HandleClientCommand.
    }

    // ---------------------------------------------------------------
    // HandleClientCommand (id=205) client->server
    // Wire: [byte 205][byte action]
    // action 0 = LOGIN_COMPLETE (ignored)
    // action 1 = PERFORM_RESPAWN
    //
    // Confirmed from MultiplayerLocalPlayer::respawn() in source:
    //   connection->send(ClientCommandPacket(PERFORM_RESPAWN))
    // And PlayerConnection::handleClientCommand — server calls
    //   server->getPlayers()->respawn(player, 0, false) on action=1.
    // ---------------------------------------------------------------
    void Connection::HandleClientCommand(const uint8_t* data, int size)
    {
        if (size < 2) return;
        uint8_t action = data[1];

        Logger::Info("Server",
            "HandleClientCommand: action=%d isDead=%s from '%ls'",
            (int)action, m_isDead ? "true" : "false",
            m_playerName.c_str());

        constexpr uint8_t PERFORM_RESPAWN = 1;
        if (action != PERFORM_RESPAWN) return;

        // Guard: ignore if player is still alive (health > 0).
        // Matches PlayerConnection::handleClientCommand source:
        //   if (player->getHealth() > 0) return;
        // Using health > 0 rather than !m_isDead so the kill console command
        // (which calls ApplyDamage) and any future damage sources all work.
        if (m_health > 0.0f) return;

        Logger::Info("Server", "'%ls' respawning (ClientCommand)",
            m_playerName.c_str());

        // Reset vitals
        m_health      = 20.0f;
        m_food        = 20;
        m_saturation  = 5.0f;
        m_fallDistance = 0.0f;
        m_isDead      = false;
        m_wasOnGround = true;
        // Grace period: ignore fall damage for 40 ticks (2 seconds) after
        // respawn so stale movement packets from the death position can't
        // immediately kill the player again.
        m_respawnGraceTicks = 40;

        // Send RespawnPacket (id=9) FIRST — matches PlayerList::respawn() source order.
        // This triggers client handleRespawn → Minecraft::respawnPlayer()
        // → SetPlayerRespawned(true) → app loop exits eAppAction_WaitForRespawnComplete
        // and calls CloseUIScenes, dismissing the "Respawning" overlay.
        // Same dimension (0) + same seed → same-dim path, NOT the loading-screen path.
        {
            int gameMode   = m_world ? m_world->GetGameMode()   : m_config->gamemode;
            int64_t seed   = m_world ? m_world->GetSeed()       : 0;
            int difficulty = m_world ? m_world->GetDifficulty() : m_config->difficulty;
            Logger::Info("Server",
                "'%ls' respawn: sending WriteRespawn dim=0 seed=%lld gm=%d diff=%d eid=%d",
                m_playerName.c_str(), seed, gameMode, difficulty, m_entityId);
            SendPacket(PacketHandler::WriteRespawn(
                0,                      // dimension: overworld
                (int8_t)gameMode,       // gameType
                seed,                   // mapSeed — must exactly match Login seed
                (int8_t)difficulty,     // difficulty
                true,                   // newSeaLevel
                (int16_t)m_entityId,    // newEntityId
                (int16_t)864,           // xzSize
                (uint8_t)3              // hellScale
            ));
        }

        // SetHealth after RespawnPacket — client entity is now rebuilt
        SendPacket(PacketHandler::WriteSetHealth(
            m_health, m_food, m_saturation));

        // After WriteRespawn, the client calls respawnPlayer() internally which
        // rebuilds the player entity. All we need to do is teleport them to spawn.
        // Do NOT call SendSpawnSequence() here — resending abilities/chunks/login
        // packets confuses the client state machine and causes a disconnect.
        // Matches PlayerList::respawn source: sends RespawnPacket then teleport only.
        int spawnX = m_world ? m_world->GetSpawnX() : 0;
        int spawnFeetY = m_world ? m_world->ResolveSpawnFeetY() : 5;
        int spawnZ = m_world ? m_world->GetSpawnZ() : 0;
        m_x = (double)spawnX + 0.5;
        m_y = (double)spawnFeetY + kSpawnFeetEpsilon;
        m_z = (double)spawnZ + 0.5;
        m_prevY = m_y;
        m_lastChunkX = INT32_MIN;
        m_lastChunkZ = INT32_MIN;
        m_visibleChunks.clear();

        // Teleport to spawn — matches player->connection->teleport() in source
        double sy = (double)spawnFeetY + kSpawnFeetEpsilon;
        SendPacket(PacketHandler::WriteMovePlayerPosRot(
            m_x, sy + 1.62, sy, m_z,
            0.0f, 0.0f, true, false));

        // Resend chunks so the world is visible after respawn
        SendSpawnChunks();

        Logger::Info("Server", "'%ls' respawn complete at spawnY=%d playerY=%.2f pos=(%d,%d,%d)",
            m_playerName.c_str(), spawnFeetY, sy + 1.62, spawnX, spawnFeetY, spawnZ);
    }

} // namespace LCEServer

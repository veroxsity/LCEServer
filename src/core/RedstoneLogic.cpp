#include "RedstoneLogic.h"
#include "TileSupport.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace LCEServer::RedstoneLogic
{
    int GetMinimalComparatorOutputSignal(
        World* world,
        int x,
        int y,
        int z,
        int blockId,
        int blockData);

    namespace
    {
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
                    RedstoneLogic::GetMinimalComparatorOutputSignal(world, sourceX, sourceY, sourceZ, blockId, blockData),
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
                if (!TileSupport::TryGetWorldBlock(world, neighborX, neighborY, neighborZ, neighborBlockId, neighborBlockData))
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
            const DustDataChangedCallback& onDustDataChanged,
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
                if (!TileSupport::TryGetWorldBlock(world, x, y, z, blockId, blockData) || blockId != 55)
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
                if (!TileSupport::TryGetWorldBlock(world, dust.x, dust.y, dust.z, blockId, oldData) || blockId != 55)
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
                        if (!TileSupport::TryGetWorldBlock(world, x, y, z, blockId, blockData) || !IsMinimalDiodeBlock(blockId))
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
                            if (TileSupport::TryGetWorldBlock(world, inputX, y, inputZ, inputBlockId, inputBlockData))
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
                        if (!TileSupport::TryGetWorldBlock(world, x, y, z, blockId, blockData) ||
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
            const DustDataChangedCallback& onDustDataChanged)
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
                        if (!TileSupport::TryGetWorldBlock(world, x, y, z, blockId, blockData) || !IsMinimalDiodeBlock(blockId))
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
    }

    int GetMinimalDiodeDirection(int blockData)
    {
        return blockData & 0x3;
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
        if (!TileSupport::TryGetWorldBlock(world, inputX, y, inputZ, blockId, blockData))
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
            if (!TileSupport::TryGetWorldBlock(world, sx, sy, sz, blockId, blockData))
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
            if (!TileSupport::TryGetWorldBlock(world, nx, ny, nz, blockId, blockData))
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

    void RefreshMinimalRedstoneAround(
        World* world,
        int originX,
        int originY,
        int originZ,
        const BlockChangedCallback& onBlockChanged,
        const DustDataChangedCallback& onDustDataChanged,
        const ScheduleDiodeChangeCallback& onScheduleDiodeChange)
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

    void CleanupUnsupportedSupportSensitiveBlocksAround(
        World* world,
        int originX,
        int originY,
        int originZ,
        const BlockChangedCallback& onBlockChanged,
        const DustDataChangedCallback& onDustDataChanged,
        const ScheduleDiodeChangeCallback& onScheduleDiodeChange)
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
            if (!TileSupport::TryGetWorldBlock(world, current.x, current.y, current.z, blockId, blockData) ||
                !TileSupport::IsSupportSensitiveBlock(blockId))
            {
                continue;
            }

            if (TileSupport::CanSupportSensitiveBlockStay(world, current.x, current.y, current.z, blockId, blockData))
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
}

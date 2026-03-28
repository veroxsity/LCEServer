#include "BlockInteraction.h"
#include "RedstoneLogic.h"

namespace LCEServer::BlockInteraction
{
    int GetToggledRepeaterDelayData(int blockData)
    {
        return (((blockData & 0xC) + 0x4) & 0xC) | (blockData & 0x3);
    }

    int GetToggledLeverData(int blockData)
    {
        return blockData ^ 0x8;
    }

    bool TryResolveComparatorToggle(
        World* world,
        int x,
        int y,
        int z,
        int blockData,
        InteractionResult& outResult)
    {
        const int newBlockData = blockData ^ 0x4;
        const int dir = RedstoneLogic::GetMinimalDiodeDirection(newBlockData);
        const int inputSignal = RedstoneLogic::GetMinimalComparatorInputSignal(world, x, y, z, dir);
        const int sideSignal = RedstoneLogic::GetMinimalComparatorSideSignal(world, x, y, z, dir);
        const bool shouldTurnOn =
            inputSignal >= 15 || (inputSignal > 0 && (sideSignal == 0 || inputSignal >= sideSignal));

        outResult.blockId = shouldTurnOn ? 150 : 149;
        outResult.blockData = newBlockData;
        return true;
    }

    bool TryResolveButtonPress(
        int blockId,
        int blockData,
        InteractionResult& outResult,
        int& outReleaseTicks)
    {
        if ((blockData & 0x8) != 0)
            return false;

        outResult.blockId = blockId;
        outResult.blockData = blockData | 0x8;
        outReleaseTicks = (blockId == 143) ? 30 : 20;
        return true;
    }
}

#include "BlockInteraction.h"
#include "RedstoneLogic.h"
#include "TileSupport.h"

namespace LCEServer::BlockInteraction
{
    namespace
    {
        bool TryGetWorldBlock(World* world, int x, int y, int z, int& outBlockId, int& outBlockData)
        {
            return TileSupport::TryGetWorldBlock(world, x, y, z, outBlockId, outBlockData);
        }
    }

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

    bool TryResolvePressurePlateState(
        World* world,
        int x,
        int y,
        int z,
        bool occupied,
        InteractionResult& outResult)
    {
        int blockId = 0;
        int blockData = 0;
        if (!TryGetWorldBlock(world, x, y, z, blockId, blockData) || !TileSupport::IsPressurePlateBlock(blockId))
            return false;

        outResult.blockId = blockId;
        outResult.blockData = occupied ? 1 : 0;
        return true;
    }

    bool TryResolveWorkbenchInteractionTarget(
        World* world,
        int x,
        int y,
        int z,
        int face,
        int& outX,
        int& outY,
        int& outZ)
    {
        outX = x;
        outY = y;
        outZ = z;

        int blockId = 0;
        int blockData = 0;
        if (TryGetWorldBlock(world, x, y, z, blockId, blockData) && blockId == 58)
            return true;

        if (face < 0 || face > 5)
            return false;

        outX = x;
        outY = y;
        outZ = z;
        switch (face)
        {
        case 0: ++outY; break;
        case 1: --outY; break;
        case 2: ++outZ; break;
        case 3: --outZ; break;
        case 4: ++outX; break;
        case 5: --outX; break;
        default: return false;
        }

        if (TryGetWorldBlock(world, outX, outY, outZ, blockId, blockData) && blockId == 58)
            return true;

        return false;
    }

    bool ShouldApplyDiodeTransition(
        World* world,
        int x,
        int y,
        int z,
        int expectedBlockId,
        int expectedBlockData)
    {
        int currentBlockId = 0;
        int currentBlockData = 0;
        return TryGetWorldBlock(world, x, y, z, currentBlockId, currentBlockData) &&
               currentBlockId == expectedBlockId &&
               currentBlockData == expectedBlockData;
    }

    bool TryResolveButtonRelease(
        World* world,
        int x,
        int y,
        int z,
        int blockId,
        int baseData,
        InteractionResult& outResult)
    {
        int currentBlockId = 0;
        int currentBlockData = 0;
        if (!TryGetWorldBlock(world, x, y, z, currentBlockId, currentBlockData) ||
            currentBlockId != blockId ||
            (currentBlockData & 0x8) == 0)
        {
            return false;
        }

        outResult.blockId = blockId;
        outResult.blockData = baseData;
        return true;
    }

    PressurePlateTickAction GetPressurePlateTickAction(
        World* world,
        int x,
        int y,
        int z,
        int expectedBlockId,
        bool occupied)
    {
        int currentBlockId = 0;
        int currentBlockData = 0;
        const bool samePressedPlate =
            TryGetWorldBlock(world, x, y, z, currentBlockId, currentBlockData) &&
            currentBlockId == expectedBlockId &&
            TileSupport::IsPressurePlateBlock(currentBlockId) &&
            (currentBlockData & 0x1) != 0;

        if (!samePressedPlate)
            return PressurePlateTickAction::NoChange;
        if (occupied)
            return PressurePlateTickAction::KeepPressed;
        return PressurePlateTickAction::Release;
    }
}

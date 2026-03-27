#pragma once

#include "../core/Types.h"

namespace LCEServer
{
    bool TryResolveLegacyItemSpec(const std::string& token, int& outItemId, int& outAux);
    bool TryResolveLegacyItemId(const std::string& token, int& outItemId);
}

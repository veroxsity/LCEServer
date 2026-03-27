// LCEServer — Core type definitions
// Standalone types so we don't drag in the full LCE stdafx.h
#pragma once

#define _HAS_STD_BYTE 0
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <array>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cassert>

namespace LCEServer
{
    struct ItemInstanceData
    {
        int16_t id = -1;
        uint8_t count = 0;
        int16_t aux = 0;

        bool IsEmpty() const
        {
            return id < 0 || count == 0;
        }
    };
}

# LCEServer

<p align="center">
  <img src="https://img.shields.io/github/license/veroxsity/LCEServer?style=for-the-badge" alt="License" />
  <img src="https://img.shields.io/github/last-commit/veroxsity/LCEServer?style=for-the-badge" alt="Last Commit" />
  <img src="https://img.shields.io/github/repo-size/veroxsity/LCEServer?style=for-the-badge" alt="Repo Size" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=c%2B%2B" alt="C++17" />
  <img src="https://img.shields.io/badge/Build-CMake-064F8C?style=flat-square&logo=cmake" alt="CMake" />
  <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows" alt="Windows" />
  <img src="https://img.shields.io/badge/Protocol-v78%20%7C%20Net%20560-2D6A4F?style=flat-square" alt="Protocol" />
</p>

LCEServer is a standalone dedicated server for Minecraft Legacy Console Edition, written in C++17.

It speaks the native LCE TCP protocol directly so unmodified legacy clients can connect without a translation layer.

## Workspace Role

- Use `LCEServer` when you want a native Legacy Console Edition multiplayer target instead of a Java bridge route
- Pair it with `LCEClient` or `LCEDebug` to test protocol behavior, gameplay flow, and multiplayer changes
- Use `LCEBridge` separately when the goal is to reach Minecraft Java Edition servers rather than host an LCE-native world

## Current Capabilities

- Native TCP networking with LCE packet framing
- Pre-login and login handshake for protocol `78` and net version `560`
- World join and spawn flow
- Chunk streaming
- Block breaking and placing with update broadcast
- Health, death, and respawn handling
- Multiplayer join, leave, and movement broadcasting
- Server administration features including config loading, ops, whitelist, bans, and RCON

## Status

LCEServer has a working multiplayer foundation and is usable for active protocol and gameplay testing. Broader gameplay systems, deeper entity support, and inventory behavior are still being expanded.

## Repository Layout

| Path | Purpose |
|------|---------|
| `src/core/` | Startup, config, logging, networking, and packet handling |
| `src/world/` | NBT, region files, world loading, and save management |
| `src/access/` | Operators, whitelist, and bans |
| `src/console/` | Interactive server console commands |
| `PACKETS.md` | Packet tracking and wire format reference |

## Requirements

- Windows
- CMake `3.20+`
- A C++17-capable compiler
- Visual Studio 2022 or Visual Studio 2022 Build Tools

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

With a multi-config generator, the executable is written to:

```text
build/Release/LCEServer.exe
```

## Run

Run from the repository root so `server.properties` and `worlds/` resolve correctly:

```powershell
.\build\Release\LCEServer.exe
```

## Related Repositories

- Hub repo: https://github.com/veroxsity/MinecraftLCE
- Bridge repo: https://github.com/veroxsity/LCEBridge
- Client repo: https://github.com/veroxsity/LCEClient
- Debug client repo: https://github.com/veroxsity/LCEDebug
- Launcher repo: https://github.com/veroxsity/LCELauncher
- Save converter repo: https://github.com/veroxsity/LCE-Save-Converter

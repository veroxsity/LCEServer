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

It speaks the native LCE TCP protocol so unmodified legacy console clients can connect directly.

## Status

| Phase | Description | State |
|-------|-------------|-------|
| P1 | Core networking, login, world, block interaction, player state, multiplayer join and leave | Complete |
| P2 | Entity movement packets and broader multiplayer sync | Implemented |
| P2 | AddPlayer entity data block | Bug: crash on second player join |
| P3 | Inventory system | Not started |

## Highlights

- Native TCP server with LCE-style length-prefixed packet framing
- Pre-login and login handshake for protocol version `78` and net version `560`
- Full join and spawn flow with chunk streaming
- Block breaking and placing with update broadcast
- Health, damage, death, and respawn cycle
- Multiplayer join, leave, and movement broadcasting
- RCON, config loading, ops, whitelist, and ban list support

## Repository Layout

| Path | Purpose |
|------|---------|
| `src/core/` | Startup, config, logging, networking, packet handling |
| `src/world/` | NBT, region files, world loading, save management |
| `src/access/` | Ban list, whitelist, and operator logic |
| `src/console/` | Interactive server console commands |
| `PACKETS.md` | Packet tracker and wire format notes |

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

With a multi-config generator, the executable is at `build/Release/LCEServer.exe`.

## Run

Run from the repository root so `server.properties` and `worlds/` resolve correctly:

```powershell
.\build\Release\LCEServer.exe
```

## Related Repositories

- Hub repo: https://github.com/veroxsity/MinecraftLCE
- Client repo: https://github.com/veroxsity/LCEClient
- Bridge repo: https://github.com/veroxsity/LCEBridge

## Current Limitations

- AddPlayer entity data crash on second player join
- No inventory system yet
- No plugin loading yet
- Some console commands are scaffolding only
- LAN advertising config exists but broadcast is not implemented

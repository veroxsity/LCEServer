# LCEServer

LCEServer is a standalone dedicated server for Minecraft Legacy Console Edition, written in C++17.

It speaks the native LCE TCP protocol so unmodified legacy console clients can connect directly.

## Related Repositories

- Hub repo: https://github.com/veroxsity/MinecraftLCE
- Client repo: https://github.com/veroxsity/LCEClient
- Bridge repo: https://github.com/veroxsity/LCEBridge

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

## Configuration

The server reads `server.properties` from the working directory. On first launch it creates the file if it does not already exist.

## Current Limitations

- AddPlayer entity data crash on second player join
- No inventory system yet
- No plugin loading yet
- Some console commands are scaffolding only
- LAN advertising config exists but broadcast is not implemented

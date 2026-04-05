# Agent Guide – halflife-gm

This document gives AI agents and automated tooling a concise overview of the repository so they can work with the code effectively.

## Project Overview

**halflife-gm** is a fork of the official Half-Life 1 SDK extended with content from **Gunman Chronicles**, a standalone Half-Life modification developed by Rewolf Software and published by Valve/Sierra.

The goal is to provide a code framework and entity definitions that allow mappers and modders to recreate Gunman Chronicles gameplay on the Half-Life engine.

## Repository Structure

```
halflife-gm/
├── cl_dll/          # Client-side DLL (HUD, input, rendering helpers)
├── common/          # Shared headers used by both client and server
├── devtools/        # Build helper scripts
├── dlls/            # Server-side DLL (weapons, monsters, entities, game logic)
├── dmc/             # Deathmatch Classic game module
├── engine/          # Engine interface headers
├── external/        # Third-party libraries
├── game/
│   ├── gunman/      # Gunman Chronicles FGD file and README
│   ├── dmc/         # DMC game data
│   └── ricochet/    # Ricochet game data
├── game_shared/     # Code shared between client and server at compile time
├── lib/             # Pre-built libraries
├── linux/           # Linux Makefiles
├── network/         # Network protocol helpers
├── pm_shared/       # Player movement code shared between client and server
├── projects/
│   ├── vs2010/      # Visual Studio 2010 project files (tools only)
│   └── vs2019/      # Visual Studio 2019 project files (main DLLs)
├── public/          # Public SDK headers
├── ricochet/        # Ricochet game module
├── utils/           # SDK utilities (qbsp, light, vis, etc.)
├── GUNMAN_CHANGES.md  # Detailed change-log for Gunman Chronicles additions
└── README.md        # Original Half-Life SDK README + build instructions
```

## Gunman Chronicles Extensions

All Gunman-specific additions are documented in [`GUNMAN_CHANGES.md`](GUNMAN_CHANGES.md). The key additions are:

| Area | File(s) | Status |
|------|---------|--------|
| Gauss Pistol weapon | `dlls/gausspistol.cpp` | ✅ Implemented |
| Weapon IDs & constants | `dlls/weapons.h` | ✅ Implemented |
| Smoke Trail effect entity | `dlls/effects.cpp` (`CEnvSmokeTrail`) | ✅ Implemented |
| Electrified surface entity | `dlls/effects.cpp` (`CEnvElectrified`) | ✅ Implemented |
| Game entity definitions (FGD) | `game/gunman/gunman.fgd` | ✅ Complete |
| Other weapons (Shot Cycler, Chemical Gun, etc.) | — | 🔄 Placeholder |
| Monster/NPC implementations | — | ❌ Requires assets |

## Building

### Windows

Requires **Visual Studio 2019** with:
- Workload: *Desktop development with C++*
- Individual component: *C++ MFC for latest v142 build tools (x86 & x64)*

Open and build:
```
projects/vs2019/hldll.vcxproj   # Server DLL
```

Tools (qbsp, etc.) use the VS2010 projects in `projects/vs2010/`.

### Linux

Build inside the [Steam Runtime "scout" SDK](https://gitlab.steamos.cloud/steamrt/scout/sdk):

```bash
cd linux
make -f Makefile.hldll              # Server DLL
make -f Makefile.hldll CREATE_OUTPUT_DIRS=1  # also create output directories
```

Output binaries are placed in a `game/` directory at the same level as the repository root.

## Key Files for Common Tasks

| Task | Relevant file(s) |
|------|-----------------|
| Add a new weapon | `dlls/weapons.h` (IDs), `dlls/<weapon>.cpp`, `dlls/weapons.cpp`, build files |
| Add a server-side entity | `dlls/` (new `.cpp`), register with `LINK_ENTITY_TO_CLASS` macro |
| Add a client-side HUD element | `cl_dll/hud.cpp`, `cl_dll/hud.h`, `cl_dll/hud_msg.cpp` |
| Add an FGD entity | `game/gunman/gunman.fgd` |
| Update Linux build | `linux/Makefile.hldll`, `dlls/Makefile` |
| Update Windows build | `projects/vs2019/hldll.vcxproj` |

## Coding Conventions

- The codebase follows the original Half-Life SDK style: C-style casts, `BOOL`/`TRUE`/`FALSE` macros, MFC-style naming.
- New weapon classes inherit from `CBasePlayerWeapon` (defined in `dlls/weapons.h`).
- New entity classes use the `LINK_ENTITY_TO_CLASS(entity_name, CClassName)` macro for registration.
- Use existing Half-Life SDK utilities (`UTIL_*`, `PRECACHE_*`, `EMIT_SOUND`) rather than direct engine calls.

## License

Half-Life 1 SDK © Valve Corp. Free distribution only. See [`LICENSE`](LICENSE) for the full terms.

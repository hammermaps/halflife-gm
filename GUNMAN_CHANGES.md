# Gunman Chronicles Integration for Half-Life

This document describes the Gunman Chronicles extensions that have been added to the Half-Life codebase.

## Overview

This implementation adds Gunman Chronicles-specific content to the Half-Life SDK, including weapons, entities, monsters, and environmental effects based on the Gunman Chronicles modification/standalone game.

## Files Added

### 1. game/gunman/gunman.fgd
A comprehensive Forge Game Data file for Gunman Chronicles that includes:
- Gunman-specific weapons
- Custom entities for environmental effects
- Gunman monsters and NPCs
- Modified trigger entities
- Vehicle and pushable entities with Gunman features

### Weapon Implementations

| File | Entity | Class | Description |
|------|--------|-------|-------------|
| `dlls/gausspistol.cpp` | `weapon_gausspistol` | `CGaussPistol` | Gauss Pistol - primary sidearm (20 rounds, 0.3s fire rate) |
| `dlls/shotcycler.cpp` | `weapon_shotcycler` | `CShotCycler` | Shot Cycler - shotgun variant (8 rounds, 6 pellets/shot) |
| `dlls/chemicalgun.cpp` | `weapon_chemicalgun` | `CChemicalGun` | Chemical Gun - rapid-fire chemical weapon (30 rounds, 0.1s fire rate) |
| `dlls/minigun.cpp` | `weapon_minigun` | `CMinigun` | Minigun - heavy weapon with spin-up (100 rounds, 0.05s fire rate) |
| `dlls/dml.cpp` | `weapon_dml` | `CDML` | DML - Dual Missile Launcher (4 rounds, rocket projectiles) |
| `dlls/beamgun.cpp` | `weapon_beamgun` | `CBeamGun` | Beam Gun - continuous beam weapon (no clip, drains ammo) |

### Monster Implementations

| File | Entity | Class | Description |
|------|--------|-------|-------------|
| `dlls/gm_houndeye.cpp` | `monster_houndeye_gm` | `CHoundeyeGM` | Houndeye variant with armor (120hp, sonic attack) |
| `dlls/gm_dinnerjacket.cpp` | `monster_dinnerjacket` | `CDinnerjacket` | Dinnerjacket creature (200hp, melee attacks) |
| `dlls/gm_geneworm.cpp` | `monster_geneworm` | `CGeneworm` | Gene Worm boss (500hp, melee+ranged) |
| `dlls/gm_xenome.cpp` | `monster_xenome` | `CXenome` | Xenome creature (80hp, leap attack) |
| `dlls/gm_shockroach.cpp` | `monster_shockroach` | `CShockroach` | Shock Roach (30hp, electric shock attack) |
| `dlls/gm_massasaur.cpp` | `monster_massasaur` | `CMassasaur` | Massasaur dinosaur (300hp, bite+tail whip) |

### Environmental Effects

| File | Entity | Class | Description |
|------|--------|-------|-------------|
| `dlls/gm_explosion.cpp` | `env_explosion_gm` | `CEnvExplosionGM` | Custom explosion with radius override and custom sprite |
| `dlls/gm_warpball.cpp` | `env_warpball` | `CEnvWarpBall` | Warp ball energy effect (green energy cylinder) |
| `dlls/gm_xenmaker.cpp` | `env_xenmaker` | `CEnvXenMaker` | Xen creature spawner (configurable type, count, delay) |

### Info Entities

| File | Entity | Class | Description |
|------|--------|-------|-------------|
| `dlls/gm_info.cpp` | `info_node_gunman` | `CInfoNodeGunman` | AI navigation node with combat/sniper flags |
| `dlls/gm_info.cpp` | `info_player_gunman` | `CInfoPlayerGunman` | Gunman player spawn point with master flag |

### Func Entities

| File | Entity | Class | Description |
|------|--------|-------|-------------|
| `dlls/gm_func_vehicle.cpp` | `func_vehicle_gm` | `CFuncVehicleGM` | Drivable vehicle along path |
| `dlls/gm_func_pushable.cpp` | `func_pushable_gm` | `CFuncPushableGM` | Enhanced pushable with breakable/explosive options |
| `dlls/gm_func_aliengrowth.cpp` | `func_alien_growth` | `CFuncAlienGrowth` | Alien infestation that damages on touch |

### Ammo Entities

| File | Entity | Class | Description |
|------|--------|-------|-------------|
| `dlls/gm_ammo.cpp` | `ammo_gaussclip` | `CGaussClipAmmo` | Gauss Pistol ammo (gives 20 rounds) |
| `dlls/gm_ammo.cpp` | `ammo_shotcycler` | `CShotCyclerAmmo` | Shot Cycler ammo (gives 8 rounds) |
| `dlls/gm_ammo.cpp` | `ammo_chemical` | `CChemicalAmmo` | Chemical Gun ammo (gives 30 rounds) |
| `dlls/gm_ammo.cpp` | `ammo_minigun` | `CMinigunAmmo` | Minigun ammo (gives 100 rounds) |
| `dlls/gm_ammo.cpp` | `ammo_dml` | `CDMLAmmo` | DML ammo (gives 4 rounds) |
| `dlls/gm_ammo.cpp` | `ammo_beamgun` | `CBeamGunAmmo` | Beam Gun ammo (gives 20 rounds) |

## Files Modified

### 1. dlls/weapons.h
- Added weapon IDs: `WEAPON_GAUSSPISTOL` (16) through `WEAPON_BEAMGUN` (21)
- Added weapon weight constants for auto-switching
- Added ammo capacity definitions (max carry amounts)
- Added clip size definitions for all Gunman weapons
- Added default give amounts for weapon pickups
- Added ammo give amounts for ammo pickups
- Added class declarations for all 6 weapon classes

### 2. dlls/effects.cpp
Added two environmental effect entities:
- **CEnvSmokeTrail** (`env_smoketrail`) - Configurable smoke trail effect
- **CEnvElectrified** (`env_electrified`) - Electrified surface with damage

### 3. Build System
All new source files added to:
- `dlls/Makefile` - Linux compilation (dlls directory)
- `linux/Makefile.hldll` - Linux build system
- `projects/vs2019/hldll.vcxproj` - Visual Studio 2019 project

## Implementation Status

✅ **Completed (Foundation):**
- All 25 Gunman Chronicles entities have foundation implementations
- Gunman Chronicles FGD file with full entity definitions
- All weapons: spawn, precache, primary/secondary attack, reload, deploy, holster, idle
- All monsters: spawn, precache, AI classification, attack handlers, sound arrays, save/restore
- All environmental effects: spawn, use/trigger, think functions, save/restore
- All info entities: spawn, node type handling
- All func entities: spawn, use, touch, save/restore
- All ammo pickups: spawn, precache, AddAmmo
- Build system integration for all platforms

🔄 **Uses Placeholder Assets:**
- Weapons use existing Half-Life models (v_9mmhandgun.mdl, v_shotgun.mdl, etc.)
- Weapons use existing Half-Life sounds (weapons/pl_gun*.wav, etc.)
- Monsters use existing Half-Life models (houndeye.mdl, zombie.mdl, etc.)
- Monsters use existing Half-Life sounds (zombie/, headcrab/, bullchicken/)

❌ **Requires Custom Assets for Full Implementation:**
- Custom models for Gunman weapons, monsters, and entities
- Custom sounds for Gunman weapons and creatures
- Custom sprites for environmental effects
- Full monster AI behaviors (custom schedules, tasks, activities)
- Monster animation event integration with custom models

## Building

The code has been integrated into the existing build system:

**Linux:**
```bash
cd linux
make -f Makefile.hldll
```

**Windows:**
Open `projects/vs2019/hldll.vcxproj` in Visual Studio 2019 and build.

## Usage in Mapping

To use Gunman Chronicles entities in your maps:
1. Load `game/gunman/gunman.fgd` in your map editor (Hammer/WorldCraft)
2. Place Gunman entities using the entity browser
3. Compile and test your map

## Notes

- The current implementation uses placeholder models from standard Half-Life
- Full Gunman Chronicles functionality would require the original game assets
- This implementation provides the complete code framework and entity definitions
- All entities support save/restore for proper game saving
- Monster variants (armored houndeye, alpha massasaur, etc.) are supported via KeyValue body types

## References

- Based on entity information from the Valve Developer Wiki
- Gunman Chronicles was an official Half-Life modification released as a standalone game
- Original game developed by Rewolf Software and published by Valve/Sierra

## License

This code follows the Half-Life SDK license. See LICENSE file in the repository root.

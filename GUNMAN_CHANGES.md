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

### 2. dlls/gausspistol.cpp
Implementation of the Gauss Pistol weapon, a signature weapon from Gunman Chronicles. Features:
- Primary fire: Single shot gauss projectile
- Secondary fire: Placeholder for charged shot
- Magazine capacity: 20 rounds
- Maximum ammo: 200 rounds

## Files Modified

### 1. dlls/weapons.h
Added weapon IDs and constants for Gunman Chronicles weapons:
- `WEAPON_GAUSSPISTOL` (16)
- `WEAPON_SHOTCYCLER` (17)
- `WEAPON_CHEMICALGUN` (18)
- `WEAPON_MINIGUN` (19)
- `WEAPON_DML` (20)
- `WEAPON_BEAMGUN` (21)

Added weapon weight constants and ammo capacity definitions.

Added weapon class declarations:
- `CGaussPistol`
- `CShotCycler`
- `CChemicalGun`

### 2. dlls/effects.cpp
Added two new Gunman Chronicles environmental effect entities:

#### CEnvSmokeTrail
Creates a smoke trail effect at the entity's position.
- Configurable sprite, framerate, lifetime, start/end size
- Can be toggled on/off
- Useful for damaged machinery, fires, or atmospheric effects

#### CEnvElectrified
Creates an electrified surface that damages entities on contact.
- Configurable damage and damage interval
- Creates electric spark effects
- Can be toggled on/off
- Useful for hazardous electrical areas

### 3. dlls/Makefile
Added `gausspistol.o` to the build targets for Linux compilation.

### 4. linux/Makefile.hldll
Added `$(HLDLL_OBJ_DIR)/gausspistol.o` to the Linux build system.

### 5. projects/vs2019/hldll.vcxproj
Added `gausspistol.cpp` to the Visual Studio 2019 project file.

## Gunman Chronicles Entities (FGD)

### Weapons
- `weapon_gausspistol` - Gauss Pistol (implemented)
- `weapon_shotcycler` - Shot Cycler shotgun variant
- `weapon_chemicalgun` - Chemical sprayer weapon
- `weapon_minigun` - Minigun/chaingun
- `weapon_dml` - DML rocket launcher
- `weapon_beamgun` - Beam weapon

### Ammo
- `ammo_gaussclip` - Gauss Pistol ammunition
- `ammo_shotcycler` - Shot Cycler ammunition
- `ammo_chemical` - Chemical Gun ammunition
- `ammo_minigun` - Minigun ammunition
- `ammo_dml` - DML ammunition
- `ammo_beamgun` - Beam Gun ammunition

### Monsters/NPCs
- `monster_houndeye_gm` - Gunman Chronicles variant of Houndeye with armor options
- `monster_dinnerjacket` - Dinnerjacket creature from Gunman
- `monster_geneworm` - Large boss creature Gene Worm
- `monster_xenome` - Xenome creature
- `monster_shockroach` - Shock Roach creature
- `monster_massasaur` - Massasaur dinosaur creature with alpha variant
- `monster_rustbot` - Rustbot enemy NPC
- `monster_trainingbot` - Training bot NPC
- `monster_human_unarmed` - Unarmed human NPC
- `monster_human_gunman` - Gunman human NPC
- `monster_gunner_friendly` - Friendly gunner NPC

### Environmental Effects
- `env_smoketrail` - Creates smoke trail effects (implemented)
- `env_electrified` - Electrified surface entity (implemented)
- `env_explosion_gm` - Gunman variant explosion entity
- `env_warpball` - Warp ball effect entity
- `env_xenmaker` - Spawns Xen-based creatures
- `sphere_explosion` - Sphere-shaped explosion effect
- `meteor_god` - Meteor effect entity

### Info Entities
- `info_node_gunman` - AI navigation node with Gunman-specific flags
- `info_player_gunman` - Gunman player spawn point
- `player_speaker` - Player-triggered speaker entity
- `random_speaker` - Randomly triggered speaker entity

### Cycler Entities
- `gunman_cycler` - Model cycler for displaying animated models

### Func Entities
- `func_vehicle_gm` - Drivable vehicle entity
- `func_pushable_gm` - Enhanced pushable entity with breakable/explosive options
- `func_alien_growth` - Alien infestation growth entity
- `vehicle_tank` - Drivable tank vehicle with optional turret

### Decoration Entities
- `decore_chair` - Decorative chair prop
- `decore_table` - Decorative table prop
- `decore_lamp` - Decorative lamp prop
- `decore_plant` - Decorative plant prop
- `decore_barrel` - Decorative barrel prop
- `decore_crate` - Decorative crate prop
- `decore_prop` - Generic decorative prop

## Implementation Status

✅ **Completed:**
- Gunman Chronicles FGD file with full entity definitions
- Gauss Pistol weapon implementation
- Smoke Trail environmental effect
- Electrified surface effect
- Build system updates (Makefile, VS2019 project)
- Syntax verification of all new code

🔄 **Partial/Placeholder:**
- Other weapon implementations (use existing models as placeholders)
- Monster implementations would require additional model and AI work

❌ **Not Implemented (would require additional assets):**
- Custom models for Gunman weapons
- Custom sounds for Gunman weapons
- Monster AI behaviors specific to Gunman creatures
- Custom textures and sprites

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

- The current implementation uses placeholder models from standard Half-Life for weapons
- Full Gunman Chronicles functionality would require the original game assets
- This implementation provides the code framework and entity definitions
- Monster implementations are defined in the FGD but would need additional C++ code for full functionality

## References

- Based on entity information from the Valve Developer Wiki
- Gunman Chronicles was an official Half-Life modification released as a standalone game
- Original game developed by Rewolf Software and published by Valve/Sierra

## License

This code follows the Half-Life SDK license. See LICENSE file in the repository root.

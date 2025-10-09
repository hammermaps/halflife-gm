# Gunman Chronicles Game Definition

This directory contains the Gunman Chronicles game definition file (FGD) for use with level editors.

## File

- **gunman.fgd** - Complete entity definitions for Gunman Chronicles

## Usage

### For Hammer/WorldCraft:
1. In your editor, go to Tools → Options
2. Under "Game Configurations", add a new configuration for Gunman Chronicles
3. Point the "Game Data files" to this `gunman.fgd` file
4. Set your mod directory to your Gunman Chronicles installation

### For J.A.C.K. Editor:
1. Go to Tools → Options → Game Profiles
2. Create a new profile or edit an existing one
3. Under "FGD Files", add `gunman.fgd`
4. Configure your game directory accordingly

## Entity Categories

The FGD includes:

### Weapons (6 Gunman-specific + 14 Half-Life compatible)
- Gauss Pistol, Shot Cycler, Chemical Gun, Minigun, DML, Beam Gun
- Plus all standard Half-Life weapons

### Ammo (6 types)
- Custom ammo types for Gunman weapons
- Compatible with Half-Life ammo types

### Monsters/NPCs (11 types)
- Houndeye (Gunman variant), Dinnerjacket, Gene Worm
- Xenome, Shock Roach, Massasaur
- Rustbot, Training Bot, Human Unarmed, Human Gunman, Friendly Gunner

### Environmental Effects (7 types)
- Smoke Trail, Electrified Surface, Explosion (Gunman variant)
- Warp Ball, Xen Maker, Sphere Explosion, Meteor

### Info Entities
- AI nodes, player spawns, speaker entities (player_speaker, random_speaker)

### Cycler Entities
- gunman_cycler - Model display and animation

### Func Entities
- Vehicles (func_vehicle_gm, vehicle_tank), enhanced pushables, alien growth

### Decoration Entities
- Chair, Table, Lamp, Plant, Barrel, Crate, Generic Prop (decore_*)

### Standard Entities
- All base Half-Life entities (triggers, lights, etc.)

## Notes

- This FGD is designed to work with the Gunman Chronicles code extensions
- Some entities require custom models and assets from Gunman Chronicles
- Compatible with Half-Life SDK codebase with Gunman extensions

## Version

- Created: 2024
- Based on: Valve Developer Wiki Gunman.fgd documentation
- Compatible with: Half-Life SDK 2.3+

## See Also

- Main repository: [halflife-gm](https://github.com/hammermaps/halflife-gm)
- GUNMAN_CHANGES.md in repository root for implementation details

# Gunman Chronicles — Map (BSP) Analysis

This document is the result of a programmatic scan of every `.bsp` file
in `game/maps/` (72 maps). The entity lump of each map was extracted
and the classnames / keyvalues were tallied. The findings below answer
the question "what do the new Gunman entities actually do in the
shipped maps, and what does our C++ port need to handle?"

## 1. Methodology

A small Python helper (run offline; not committed) reads each BSP,
seeks to lump 0 (entities), parses the `{ ... }` blocks and records
both classname frequency and per-class key usage. The full per-class
output was used to produce the tables in this document.

## 2. Coverage check — entities used in maps vs. our FGD / C++ port

### 2.1 Entities that **are** present in the FGD / C++ and used by maps

The following Gunman entities are placed by the official maps and have a
matching C++ stub in this repo. The instance counts indicate how
important each one is in practice.

| Entity | Map instances | Maps using it |
|---|---:|---:|
| `gunman_cycler`              | 373 | 31 |
| `random_speaker`             | 265 | 35 |
| `ammo_minigunClip`           | 194 | 38 |
| `ammo_gaussclip`             | 153 | 32 |
| `ammo_buckshot`              | 142 | 42 |
| `entity_spritegod`           |  82 | 27 |
| `ammo_beamgunclip`           |  70 | 19 |
| `ammo_dmlsingle`             |  55 | 23 |
| `decore_spacedebris`         |  42 |  1 |
| `ammo_chemical`              |  41 | 15 |
| `item_armor`                 |  35 | 27 |
| `weapon_shotgun`             |  24 | 18 |
| `weapon_minigun`             |  20 | 18 |
| `decore_asteroid`            |  14 |  3 |
| `weapon_dml`                 |  13 | 11 |
| `weapon_beamgun`             |  10 | 10 |
| `item_gascan`                |  10 |  3 |
| `monster_xenome`             |   8 |  4 |
| `monster_xenome_embryo`      |   8 |  1 | ⚠️ No FGD entry or `LINK_ENTITY_TO_CLASS` — treated here as used but missing (see §2.2). |
| `trigger_tank`               |   7 |  4 |
| `decore_gutspile`            |   9 |  7 |
| `decore_butterflyflock`      |   4 |  3 |
| `entity_digitgod`            |   3 |  1 |
| `weapon_gausspistol`         |   2 |  2 |
| `weapon_fists`               |   1 |  1 |
| `weapon_aicore`              |   1 |  1 |

> Note the case difference: maps use `ammo_minigunClip` (capital **C**),
> whereas our FGD declares `ammo_minigun`. GoldSrc classname lookup is
> **case-sensitive** — see §4.

### 2.2 Entities used by maps but **NOT** in the Gunman FGD nor C++

These entities are referenced by the BSPs but currently have no Hammer
entry and no engine implementation. Loading any of these maps in our
build will silently drop them.

#### Weapons / ammo

| Classname | Instances | Maps | Notes |
|---|---:|---:|---|
| `weapon_aicore`         |  1 | 1 | ✅ Added in this PR: `dlls/aicore.cpp` + FGD entry. |
| `weapon_SPchemicalgun`  |  7 | 5 | Single-player variant of `weapon_chemicalgun` (cannot be dropped). |
| `ammo_dmlclip`          | 27 | 18 | 8-round Mule magazine. (Distinct from `ammo_dmlsingle` and `ammo_dml`.) |
| `cust_2GaussPistolSniper` | 8 | n/a | "Custom-2" weapon variant — a pre-configured sniper-mode Gauss Pistol pickup. |
| `cust_2MinigunCooled`   |  4 | n/a | Pre-configured cooled minigun pickup. |

#### Monsters / NPCs (very large gap)

41 distinct monster classes are placed by the stock maps but missing
from our port. The most frequent are:

| Classname | Instances | Maps | Notes |
|---|---:|---:|---|
| `monster_furniture`           | 145 | 38 | |
| `monster_human_bandit`        | 109 |  32 | |
| `monster_scientist`           |  63 | 24 | ⚠️ HL has `dlls/scientist.cpp` but GC uses its own AI/model — needs a Gunman-specific variant. |
| `monster_beak`                |  60 | 16 | |
| `monster_generic`             |  53 | 26 | ⚠️ HL has `dlls/genericmonster.cpp` but GC-specific keyvalues/model need a Gunman variant. |
| `monster_xenome_embryo`       |   8 |  1 | No FGD entry, no C++ implementation. |
| `monster_rustbot`             |  43 | 14 | |
| `monster_scorpion`            |  43 |  9 | |
| `monster_rustbit`             |  35 | 11 | |
| `monster_human_demoman`       |  33 | 16 | |
| `monster_targetrocket`        |  32 |  6 | |
| `monster_tube`                |  27 | 10 | |
| `monster_hatchetfish`         |  25 |  5 | |
| `monster_human_unarmed`       |  23 |  7 | |
| `monster_human_gunman`        |  16 |  5 | |
| `monster_sentry`              |  16 |  6 | |
| `monster_raptor`              |  16 |  6 | |
| `monster_human_scientist`     |  15 |  8 | |
| `monster_rustbit_friendly`    |  15 |  4 | |
| `monster_dragonfly`           |  12 |  3 | |
| `monster_critter`             |  12 |  6 | |
| `monster_maggot`              |   9 |  5 | |
| `monster_trainingbot`         |   8 |  2 | |
| `monster_rustflier`           |   5 |  4 | |
| `monster_microraptor`         |   5 |  3 | |
| `monster_largescorpion`       |   5 |  5 | |
| `monster_aigirl`              |   4 |  3 | |
| `monster_gator`               |   4 |  1 | |
| `monster_rustgunr`            |   4 |  4 | |
| `monster_darttrap`            |   4 |  1 | |
| `monster_human_chopper`       |   2 |  2 | |
| `monster_tube_embryo`         |   2 |  1 | |
| `monster_ourano`              |   3 |  1 |
| `monster_gunner_friendly`     |   3 |  2 |
| `monster_rustbot_friendly`    |   3 |  2 |
| `monster_beakbirther`         |   3 |  3 |
| `monster_cricket`             |   7 |  4 |
| `monster_endboss`             |   1 |  1 |
| `monster_flashlight`          |   1 |  1 |
| `monster_renesaur`            |   1 |  1 |
| `monster_sentry_mini`         |   3 |  1 |
| `monster_tubequeen`           |   1 |  1 |
| `monster_tank`                |   2 |  1 |

Mappers also use `monster_houndeye` (stock HL) and `monstermaker` (408
instances) — those keep working through the existing HL implementation.

Note the typical key set:
- Humans: `gn_idle`, `gn_stare`, `gn_use`, `gn_unuse`, `gn_attack`,
  `gn_noshoot`, `gunstate`, `MechaGunBandit`, `healthvalue`,
  `StartDrawn`, `netname` (squad).
- Scientists (extended): `sc_idle`, `sc_use`, `sc_stare`, `sc_attack`,
  `sc_noshoot`, `sc_hello`, `sc_question`, `sc_answer`, `sc_unuse`,
  `knife`, `UseSentence`, `AllowInterrupt`, `clearvoice`, `blocksound`.
- AI in general: `spawndeadpose`, `TriggerCondition`, `TriggerTarget`,
  `body`, `skin`, `_skin`.
- Rust-flier: `targetdamage`, `startshieldstate`, `deathangle`,
  `timetillcrash`, `triggerondeath`, `deathpath`.

These all need to be parsed for a faithful port. They are out of scope
for this pass — see `GUNMAN_LUA_PORT_PLAN.md` for the staging plan.

#### Decoration entities (decore_*)

| Classname | Inst. | Maps | Notes |
|---|---:|---:|---|
| `decore_torch`            | 126 | 7  | ✅ `CDecoreTorch` — `models/Torch.mdl` + `EF_BRIGHTLIGHT` |
| `decore_swampplants`      |  89 | 6  | ✅ `CDecoreSwampplants` — `models/swampstuff.mdl`; `body` via engine |
| `decore_cactus`           |  64 | 12 | ✅ `CDecoreCactus` — `models/cactus.mdl`; solid, drops to floor, touch dmg 1/1s |
| `decore_prickle`          |  61 | 10 | ✅ `CDecorePrickle` — `models/prickle.mdl`; drops to floor |
| `decore_spacedebris`      |  42 | 1  | ✅ (already in our FGD; uses `debrislife`) |
| `decore_cam`              |  27 | 11 | ✅ `CDecoreCam` — `models/Camera.mdl`; sweeps ±45° on Y at 30°/s |
| `decore_ice`              |  19 | 1  | ✅ `CDecoreIce` — `models/ice.mdl`; solid, additive render, drops to floor |
| `decore_labstuff`         |  18 | 4  | ✅ `CDecoreLabstuff` — `models/labstuff.mdl`; `body` via engine |
| `decore_pteradon`         |  15 | 5  | ✅ `CDecorePteradon` — `models/pteradon2.mdl`; MOVETYPE_FLY, symmetric bbox |
| `decore_pipes`            |  10 | 8  | ✅ `CDecorePipes` — `models/pipes.mdl`; solid |
| `decore_gutspile`         |   9 | 7  | ✅ `CDecoreGutspile` — `models/Gutspile.mdl`; drops to floor |
| `decore_explodable`       |   5 | 3  | ✅ `CDecoreExplodable` — custom model/gib/sequences; damageable |
| `decore_sittingtubemortar`|   6 | 1  | ✅ `CDecoreSittingTubeMortar` — `models/tubemortar.mdl`; solid, drops to floor, frame 1 |
| `decore_eagle`            |   3 | 3  | ✅ `CDecoreEagle` — `models/eagle.mdl` |
| `decore_nest`             |   3 | 3  | ✅ `CDecoreNest` — `models/ornest.mdl`; drops to floor |
| `decore_baboon`           |   4 | 2  | ✅ `CDecoreBaboon` — `models/Baboon.mdl` |
| `decore_hatgib`           |   3 | 2  | ✅ `CDecoreHatgib` — `models/Hatgib.mdl`; drops to floor |
| `decore_bodygib`          |   3 | 2  | ✅ `CDecoreBodygib` — `models/Bodygib.mdl`; drops to floor |
| `decore_mushroom`         |   3 | 1  | ✅ `CDecoreMushroom` — `models/Mushroom.mdl`; solid |
| `decore_mushroom2`        |   2 | 1  | ✅ `CDecoreMushroom2` — `models/mushroom2.mdl`; solid |
| `decore_foot`             |   1 | 1  | ✅ `CDecoreFoot` — `models/renesaurfoot.mdl` |
| `decore_butterflyflock`   |   4 | 3  | ✅ (already in our FGD) |
| `decore_asteroid`         |  14 | 3  | ✅ (already in our FGD) |

Additional decore entities found in `eukara/freegunman` and `MisterCalvin/SvenCoop-GC` references:

| Classname | Notes |
|---|---|
| `decore_aicore`            | ✅ `CDecoreAicore` — `models/W_aicore.mdl`; random spin (ShouldRotate) |
| `decore_camflare`          | ✅ `CDecoreCamflare` — `models/cameracone.mdl` |
| `decore_icebeak`           | ✅ `CDecoreIceBeak` — `models/icebeak.mdl`; solid, drops to floor |
| `decore_torchflame`        | ✅ `CDecoreTorchFlame` — `sprites/flames.spr` additive animated sprite |
| `decore_goldskull`         | ✅ `CDecoreGoldskull` — `models/goldskull.mdl` |
| `decore_sack`              | ✅ `CDecoreSack` — `models/sack.mdl` |
| `decore_scripted_boulder`  | ✅ `CDecoreScriptedBoulder` — `models/boulder.mdl`; solid |
| `decore_corpse`            | ✅ `CDecoreCorpse` — model set by mapper; drops to floor |

All `decore_*` entities are derived from `CGunmanCycler` (itself a `CBaseAnimating`).
Simple props use the `CDecoreSimple` intermediate base class with virtual hooks for
`ShouldDropToFloor()`, `ShouldBeSolid()`, `TouchDamage()`, `StartFrame()`, `ShouldRotate()`,
and `DefaultRenderMode()`.  Entities with custom behaviour (cam sweep, explodable) retain
their own `Spawn`/`KeyValue`/`Save`+`Restore` implementations.

#### Triggers / map logic

| Classname | Inst. | Maps | Notes |
|---|---:|---:|---|
| `trigger_gunmanteleport` | 8  | n/a | GC teleporter (replaces `trigger_teleport` with destination handling). |
| `trigger_tank`           | 7  | 4 | ✅ Implemented — fires when `vehicle_tank_body` touches brush |
| `trigger_tankoutofgas`   | 3  | 3 | ❌ Not yet implemented |
| `trigger_tankshell`      | 10 | 4 | ❌ Not yet implemented |
| `random_trigger`         | 77 | 13 | ✅ `CRandomTrigger` — fires target at random interval (random_min…random_max); toggleable via Use() |
| `meteor_god`             | 51 | n/a | ❌ Not yet implemented |
| `meteor_target`          | 12 | n/a | ❌ Not yet implemented |
| `lava_god`               |  8 | n/a | ❌ Not yet implemented |
| `entity_clustergod`      |  3 | 1 | ❌ Not yet implemented |
| `sphere_explosion`       |  6 | n/a | ❌ Not yet implemented |
| `demoman_mine`           | 13 | n/a | ❌ Not yet implemented |
| `button_aiwallplug`      |  5 | n/a | ✅ Implemented in `dlls/aicore.cpp` |
| `hologram_beak`          |  3 | n/a | ❌ Not yet implemented |
| `hologram_damage`        |  6 | n/a | ❌ Not yet implemented |
| `aiscripted_sequence`    | 24 | n/a | ❌ Not yet implemented |
| `func_tanklaserrust`     |  1 | n/a | ❌ Not yet implemented |

#### Vehicles (`vehicle_tank*`)

These four entities form a single composite vehicle:

| Classname | Inst. | Maps |
|---|---:|---:|
| `vehicle_tank`        | 3 | 1 |
| `vehicle_tank_body`   | 8 | 4 |
| `vehicle_tank_turret` | 8 | 4 |
| `vehicle_tank_barrel` | 8 | 4 |

`trigger_tank` already in our FGD fires when touched by a
`vehicle_tank_body`. Maps clearly link them by `targetname` chains.
Porting the tank is a self-contained sub-project — it is not blocked
by any other work in this pass.

#### Player support entities

| Classname | Inst. | Notes |
|---|---:|---|
| `player_giveitems`  |  1 | Replaces `player_weaponstrip` with an additive give. |
| `player_loadsaved`  | 15 | Save-game checkpoint. |
| `player_speaker`    | 72 | Plays a sound on the player. |
| `player_togglehud`  | 15 | Shows / hides custom GC HUD. |

#### Auxiliary

| Classname | Inst. | Notes |
|---|---:|---|
| `info_node_air`             |    1 | Air navigation node. |
| `info_teleport_destination` |   14 | Standard HL — works out-of-the-box. |
| `cycler`                    |    3 | Stock HL. |
| `targ_speaker`              |   73 | Plays a sound at a target entity. |

## 3. Per-class key usage (Gunman-specific entities)

The exact key tables produced by the scan are too large to inline.
Selected highlights that drive concrete change requests:

- **`random_speaker`** (265 instances) → keys: `rsnoise`, `wait`,
  `volume`, `random`, **`attenuation`** (2x), **`noise`** (2x). The
  last two are unhandled in our C++. See `GUNMAN_FGD_VS_CPP.md`.
- **`entity_spritegod`** (82 instances) → keys: `spritename`,
  `spritefreq`, `spritecount`, `spritespeed`, `spritenoise`, `spritex`,
  `spritey`, `spritez`, plus **`spritestartstate`** (26x) and
  **`targetent`** (3x). The last two are unhandled.
- **`gunman_cycler`** (373 instances) → keys: `model`, `cyc_submodel1`,
  `cyc_submodel2`, `cyc_submodel3`, `rendercolor`, `rendermode`,
  `renderamt`, `angles`, `_minlight`, `renderfx`, `spawnflags`. All
  parsed by C++. ✅
- **`decore_asteroid`** (14 instances) → keys: `asteroidsize` (13x),
  **`size`** (10x — alias not in C++), `targetname`, `model`,
  `cyc_submodel1-3`, `body`, `rendercolor`, `_minlight`.
- **`decore_butterflyflock`** (4 instances) → keys: **`flFlockRadius`**
  (4x), **`iFlockSize`** (4x). Both unhandled in our C++.
- **`decore_spacedebris`** (42 instances, all in one map) → keys:
  `target`, `dirx`/`diry`/`dirz`, **`debrislife`** (42x — unhandled),
  `anglespeed`, `forwardspeed`, `modelname`, `targetname`. Default
  `modelname` is `models/decoregibs2.mdl` which **is present** in
  `game/models/`. Outstanding gap: `debrislife` key is not parsed by C++.
- **`entity_digitgod`** (3 instances) → keys: `target`, `maxdamage`,
  `angles`, `targetname`. All ✅.
- **`trigger_tank`** (7 instances) → keys: `model` (brush model
  reference), `target`, `spawnflags`. ✅.
- **`weapon_*` / `ammo_*` / `item_*`** → only `origin`, `angles`,
  occasionally `target`. No surprises.

## 4. Cross-cutting findings

### 4.1 Case-sensitivity — applied in this PR

`ammo_minigunClip` was used by 194 instances in 38 maps **with a
capital C** in `Clip`. GoldSrc `LINK_ENTITY_TO_CLASS()` is
case-sensitive on Linux dedicated servers.

✅ **Fixed:** `dlls/gm_ammo.cpp` now registers three aliases:
```cpp
LINK_ENTITY_TO_CLASS( ammo_gcminigunclip, CGCMinigunClipAmmo );
LINK_ENTITY_TO_CLASS( ammo_minigunClip,   CGCMinigunClipAmmo );
LINK_ENTITY_TO_CLASS( ammo_minigunclip,   CGCMinigunClipAmmo );
```
Similarly, `ammo_dmlclip` is now a separate `CDMLClipAmmo` class that
gives the correct 8-round magazine amount.

### 4.2 Missing assets referenced by maps

- `models/swampplant.mdl` — referenced by `decore_*` entities; not
  present in `game/models/`. Also: `models/torch.mdl`, `models/mushroom.mdl`,
  `models/cam.mdl`, `models/nest.mdl`, `models/hatgib.mdl`,
  `models/bodygib.mdl`, `models/foot.mdl`, `models/sittingtubemortar.mdl`,
  `models/mushroom2.mdl`.
- The following models are **present** in `game/models/` and do **not** need
  uploading: `models/cactus.mdl`, `models/prickle.mdl`, `models/eagle.mdl`,
  `models/Baboon.mdl`, `models/ice.mdl`, `models/pteradon.mdl`,
  `models/pipes.mdl`, `models/decoregibs2.mdl`.

### 4.3 `ammo_*` entity defaults must match the FGD give amounts

The maps assume the give amount declared in the FGD (e.g.
`ammo_minigunClip` gives 50 rounds). Our `dlls/weapons.h`
`AMMO_*_GIVE` constants currently use lower values inherited from the
HL placeholder set — verify and reconcile when implementing the real
ammo refill logic.

## 5. Stop list — what NOT to do

- Do **not** edit the BSP entity lumps to "match" our FGD. Maps
  should remain authoritative; the engine layer adapts.
- Do **not** rename `ammo_minigunClip` to lower-case. Add an alias
  registration instead (see §4.1).
- Do **not** delete the FGD entries for entities that maps don't use
  yet (`monster_houndeye_gm`, `monster_dinnerjacket`,
  `monster_geneworm`, `monster_shockroach`, `env_warpball`,
  `env_xenmaker`, `func_vehicle_gm`, `func_pushable_gm`,
  `func_alien_growth`, `info_node_gunman`, `info_player_gunman`).
  These are valid future-facing entries even if no shipped map
  currently uses them — multiplayer maps and new SP maps will.

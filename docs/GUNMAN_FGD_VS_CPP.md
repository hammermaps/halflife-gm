# Gunman Chronicles — FGD ↔ C++ Parameter Comparison

This document is a per-entity table of every parameter declared in
`game/gunman.fgd` against what the matching C++ class in `dlls/*.cpp`
actually understands (its `KeyValue()` handler and `m_SaveData[]`
table).

Legend:
- ✅ FGD key is handled by C++.
- ⚠️ Different name / type / default in C++ (clarification follows).
- ❌ Declared in the FGD but **not** parsed by C++ (silently dropped at load time).
- ➕ C++ understands the key, but the FGD does **not** expose it — mappers must type it manually.

For brevity, the inherited `Targetname / Target / Angles / RenderFields /
RenderFxChoices / ZHLT / Appearflags / Global` base classes are not
repeated; their keys (`targetname`, `target`, `angles`, `rendermode`,
`renderfx`, `renderamt`, `rendercolor`, `delay`, `killtarget`,
`globalname`, `_minlight`, `_fade`, `_falloff`, `zhlt_lightflags`,
`light_origin`, `spawnflags`) work out-of-the-box through the engine /
`CBaseEntity::KeyValue` and need no per-entity wiring.

---

## 1. Weapons

The nine Gunman weapon classes (`CGaussPistol`, `CShotCycler`,
`CChemicalGun`, `CMinigun`, `CDML`, `CBeamGun`, `CGCFists`,
`CGCShotgun`, `CAICore`) inherit from `CBasePlayerWeapon`. The FGD declares
them as `base(Weapon)` and lists **no extra keys** — only `angles`,
`targetname`, `spawnflags`. Nothing to compare.

> The interesting weapon configuration (fire modes, range, accuracy,
> power, etc.) is **not** stored on the entity at all in either FGD or
> C++ — it is per-player runtime state that is selected through the
> SWEP menu in the Lua reference. See `GUNMAN_LUA_PORT_PLAN.md`.

## 2. Ammo / item pickups

Same situation as weapons. `@PointClass base(Ammo)` exposes nothing
beyond `angles` + `spawnflags`. C++ ammo classes (`CGaussClipAmmo` &c.,
`CItemGasCan`, `CItemArmor`) only override `Spawn / Precache / AddAmmo`.
There is **no parameter mismatch**.

## 3. Monsters

### `monster_houndeye_gm` (`CHoundeyeGM`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `body` | `0` (`Normal` / `Armored`) | ✅ | Maps to `pev->body`. |
| (inherited Monster keys) | — | ✅ | `TriggerTarget`, `TriggerCondition`, `spawnflags` (WaitTillSeen / Gag / MonsterClip / Prisoner / WaitForScript / Pre-Disaster / Fade Corpse). |

### `monster_dinnerjacket` (`CDinnerjacket`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `body` | `0` (`Normal` / `Heavy`) | ✅ | |

### `monster_geneworm` (`CGeneworm`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `health` | `500` | ✅ | Handled directly (FGD default 500, C++ default also 500 in `Spawn()`). |

### `monster_xenome` (`CXenome`)

No extra keys in FGD. Matches C++. ✅

### `monster_shockroach` (`CShockroach`)

No extra keys in FGD. Matches C++. ✅

### `monster_massasaur` (`CMassasaur`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `body` | `0` (`Normal` / `Alpha`) | ✅ | |

### ❌ Missing in FGD but present in maps

The BSP analysis (see `GUNMAN_BSP_ANALYSIS.md`) finds many `monster_*`
classes used in actual maps that do **not** exist in either FGD or C++:
`monster_human_bandit`, `monster_human_gunman`, `monster_human_demoman`,
`monster_human_scientist`, `monster_human_unarmed`, `monster_human_chopper`,
`monster_beak`, `monster_raptor`, `monster_microraptor`, `monster_rustbit`,
`monster_rustbot`, `monster_rustflier`, `monster_rustgunr`, `monster_scorpion`,
`monster_largescorpion`, `monster_sentry`, `monster_sentry_mini`, `monster_tube`,
`monster_tubequeen`, `monster_aigirl`, `monster_ourano`, `monster_renesaur`,
`monster_endboss`, `monster_gator`, `monster_furniture`, `monster_targetrocket`,
`monster_trainingbot`, `monster_critter`, `monster_cricket`, `monster_dragonfly`,
`monster_flashlight`, `monster_hatchetfish`, `monster_maggot`, `monster_darttrap`,
`monster_xenome_embryo`, `monster_tube_embryo`, `monster_beakbirther`,
`monster_gunner_friendly`, `monster_rustbit_friendly`, `monster_rustbot_friendly`.

These are tracked in `GUNMAN_BSP_ANALYSIS.md` together with the
custom keys they use.

## 4. Environmental effects

### `env_smoketrail` (`CEnvSmokeTrail` in `dlls/effects.cpp`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `spritename`  | `sprites/smoke.spr` | ✅ | `pev->model = ALLOC_STRING(value)`. |
| `framerate`   | `10`  | ✅ | Stored in `m_iFramerate`. |
| `lifetime`    | `5`   | ✅ | Stored in `m_flLifetime`. |
| `startsize`   | `10`  | ✅ | Stored in `m_iStartSize`. |
| `endsize`     | `30`  | ✅ | Stored in `m_iEndSize`. |
| `rendercolor` | `255 255 255` | ✅ | Inherited via `pev->rendercolor`. |
| `renderamt`   | `255` | ✅ | Inherited via `pev->renderamt`. |
| `spawnflags 1 — Start On` | `0` | ✅ | Tested in `Spawn()`. |

### `env_electrified` (`CEnvElectrified` in `dlls/effects.cpp`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `dmg`  | `10`  | ✅ | Stored in `m_flDamage`. FGD declares as `integer`, C++ stores as `float` — no observable difference. |
| `wait` | `1.0` | ✅ | Stored in `m_flWait`. |
| `spawnflags 1 — Start On` | `1` | ✅ | |
| `spawnflags 2 — Toggle`   | `0` | ✅ | |

### `env_explosion_gm` (`CEnvExplosionGM`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `iMagnitude` | `100` | ✅ | |
| `iRadiusOverride` | `0` | ✅ | |
| `sprite` | `sprites/zerogxplode.spr` | ✅ | Stored on `pev->message`. |
| `rendermode` | `5 (Additive)` | ✅ | Inherited. |
| `spawnflags 1 — No Damage`    | ✅ | |
| `spawnflags 2 — Repeatable`   | ✅ | |
| `spawnflags 4 — No Fireball`  | ✅ | |
| `spawnflags 8 — No Smoke`     | ✅ | |
| `spawnflags 16 — No Decal`    | ✅ | |
| `spawnflags 32 — No Sparks`   | ✅ | |
| ➕ `spriteScale` | — | ➕ | C++ exposes `m_spriteScale` field but never reads it from a KeyValue; consider adding it to the FGD or removing from C++. |

### `env_xenmaker` (`CEnvXenMaker`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `monstertype`     | `monster_xenome` | ✅ | |
| `monstercount`    | `1` | ✅ | |
| `m_flDelay`       | `5` | ✅ | |
| `m_iMaxLiveChildren` | `5` | ⚠️ | C++ matches on the **lower-cased** string `"m_imaxlivechildren"`. GoldSrc compares case-insensitively so this works in practice, but the cosmetic mismatch is worth noting. |
| `netname`         | (target on spawn) | ➕ | Handled by base class through `pev->netname` — not parsed explicitly. Fires `pev->netname` target after each monster spawn. |
| `spawnflags 1 — Start On` | ✅ | |
| `spawnflags 2 — Cyclic`   | ✅ | |

### `env_warpball` (`CEnvWarpBall`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `target`      | — | ✅ | (inherited). |
| `radius`      | `256` | ✅ | |
| `duration`    | `5`   | ✅ | |
| `rendercolor` | `0 255 0` | ✅ | (inherited). FGD declares green; C++ uses `pev->rendercolor` directly. |
| ➕ `m_flDuration` | — | ➕ | Save/restore only — never read from KeyValue. The `duration` key feeds the same field. |

## 5. Info entities

### `info_node_gunman` (`CInfoNodeGunman` in `dlls/gm_info.cpp`)

| FGD key | C++ status | Notes |
|---|---|---|
| `spawnflags 1 — Crouch Cover` | ✅ | |
| `spawnflags 2 — Stand Cover`  | ✅ | |
| `spawnflags 4 — Sniper Spot`  | ✅ | |
| `spawnflags 8 — Combat Node`  | ✅ | |
| ➕ `nodetype` | ➕ | C++ accepts a string keyvalue `nodetype` (e.g. `air`, `land`, `water`). Not exposed in the FGD — mappers would have to add it via "SmartEdit Off". Consider documenting in FGD. |

### `info_player_gunman` (`CInfoPlayerGunman`)

| FGD key | C++ status | Notes |
|---|---|---|
| `spawnflags 1 — Master` | ✅ | |
| ➕ `master` | ➕ | C++ exposes a `master` string keyvalue (multisource gate) that is not in the FGD. |

## 6. Func entities

### `func_vehicle_gm` (`CFuncVehicleGM`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `target` | (first stop) | ✅ inherited | |
| `sounds` | `0` (None / V1 / V2) | ✅ | |
| `speed`  | `100` | ✅ | |
| `health` | `0` | ✅ | |
| `dmg`    | `0` | ✅ | |
| `volume` | `10` | ✅ | |
| `spawnflags 1 — No Pitch (X-rot)` | ✅ | Matches `func_tracktrain` semantics. |
| `spawnflags 2 — No User Control`  | ✅ | |
| `spawnflags 8 — Passable`         | ✅ | |
| ❌ `spawnflags 4` | — | The FGD skips the `4` bit on purpose (matches HL2 `func_tracktrain`). C++ does not test it. OK. |

### `func_pushable_gm` (`CFuncPushableGM`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `size`     | `0` (Point / Human / Large / Tiny) | ✅ | |
| `friction` | `50` | ✅ | |
| `buoyancy` | `20` | ✅ | |
| `health`   | `0`  | ✅ inherited | |
| `spawnflags 128 — Breakable` | ✅ | |
| `spawnflags 256 — Explosive` | ✅ | |

### `func_alien_growth` (`CFuncAlienGrowth`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `health` | `100` | ✅ | |
| `dmg`    | `10`  | ✅ | |
| `spawnflags 1 — Start Active`  | ✅ | |
| `spawnflags 2 — Grow Over Time` | ✅ | |

## 7. Map utility / decoration

### `random_speaker` (`CRandomSpeaker`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `rsnoise` | (sound) | ✅ | |
| `volume`  | `1.0`   | ✅ | |
| `wait`    | `5.0`   | ✅ | |
| `random`  | `100`   | ✅ | |
| ❌ `attenuation` | — | ❌ | **Used by stock GC maps** (`attenuation "2"`). Not parsed by current C++. Add a keyvalue to fix or document. |
| ❌ `noise` | — | ❌ | Stock maps also use `noise "world/drip3.wav"` as an alternative to `rsnoise`. Not parsed. |

### `gunman_cycler` (`CGunmanCycler`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `model`         | (studio) | ✅ inherited | |
| `sequence`      | `0` | ⚠️ | FGD declares `sequence` but C++ never reads it from a KeyValue — it accepts the sequence on the model directly. Stock maps don't set it either. |
| `cyc_submodel1` | `0` | ✅ | Maps to body group slot 1. |
| `cyc_submodel2` | `0` | ✅ | |
| `cyc_submodel3` | `0` | ✅ | |
| ➕ `body`       | — | ➕ | Map `decore_asteroid` sometimes sets `body` directly; falls through to `pev->body`. |

### `decore_asteroid` (`CDecoreAsteroid`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `model`         | (studio) | ✅ inherited | |
| `asteroidsize`  | `2` (Big / Medium / Small) | ✅ | |
| `maxrotation`   | `0.5` | ✅ | |
| `minrotation`   | `0.1` | ✅ | |
| ❌ `size`       | — | ❌ | The stock maps sometimes use `size` as an alias for `asteroidsize` (BSP analysis shows 10 instances). C++ should accept both. |

### `decore_spacedebris` (`CDecoreSpaceDebris`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `modelname`    | (studio) | ✅ | Note: not `model`, but `modelname`. |
| `dirx` `diry` `dirz` | `0 0 1` | ✅ | |
| `forwardspeed` | `200` | ✅ | |
| `anglespeed`   | `3.0` | ✅ | |
| ❌ `debrislife` | — | ❌ | Used by **all 42** instances in the stock RUST5A map (`debrislife "40"`). C++ does not parse it — debris currently lives forever. Should map to a `m_flLife` / `pev->dmgtime`. |

### `decore_butterflyflock` (`CDecoreButterflyFlock`)

| FGD key | C++ status | Notes |
|---|---|---|
| (no FGD keys exposed) | — | |
| ❌ `flFlockRadius`  | — | Used by all 4 stock-map instances (`flFlockRadius "64"`). C++ does not parse. |
| ❌ `iFlockSize`     | — | Used by all 4 instances (`iFlockSize "20"`). C++ does not parse. |

> Recommendation: extend both the FGD and `CDecoreButterflyFlock` to accept these two keys; flock radius and size are the core knobs of the entity.

### `decore_gutspile` (`CDecoreGutspile`)

No extra keys; matches C++. ✅

### `entity_spritegod` (`CEntitySpriteGod`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `spritename`  | (sprite) | ✅ | |
| `spritespeed` | `16`     | ✅ | |
| `spritecount` | `8`      | ✅ | |
| `spritefreq`  | `0`      | ✅ | |
| `spritenoise` | `100`    | ✅ | |
| `spritex` `spritey` `spritez` | `0 0 1` | ✅ | |
| ❌ `spritestartstate` | — | ❌ | Stock maps set this on 26 instances (`spritestartstate "1"`) to make the entity emit from spawn. C++ should treat this as “Start On”. |
| ❌ `targetent` | — | ❌ | Used by 3 instances to attach the sprite spray to another entity (`targetent "bloodspray_targ"`). |

### `trigger_tank` (`CTriggerTank`)

| FGD key | C++ status | Notes |
|---|---|---|
| (inherited Targetname/Target) | ✅ | The trigger only fires when touched by a `vehicle_tank_body`. |

> See `GUNMAN_BSP_ANALYSIS.md` for the broader `vehicle_tank_*` family that
> currently has **no FGD entry and no C++ implementation** but is used by
> 4 stock maps.

### `player_gcweaponstrip` (`CPlayerGCWeaponStrip`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `m_iAffected` | `0` (Activator only / All / All but activator) | ✅ | |

### `entity_digitgod` (`CEntityDigitGod`)

| FGD key | Default | C++ status | Notes |
|---|---|---|---|
| `target`    | (fire when threshold reached) | ✅ inherited | |
| `maxdamage` | `100` | ✅ | |

---

## Summary of concrete bugs / gaps found

The comparison above surfaces the following actionable items:

1. **`random_speaker`** — add `attenuation` and `noise` keys to both
   FGD and `dlls/gm_mapents.cpp::CRandomSpeaker::KeyValue` so the stock
   maps load cleanly.
2. **`decore_asteroid`** — accept `size` as an alias for `asteroidsize`
   in `CDecoreAsteroid::KeyValue`.
3. **`decore_spacedebris`** — add `debrislife` as a finite lifetime
   (set `pev->dmgtime = gpGlobals->time + life` and self-remove in the
   think function).
4. **`decore_butterflyflock`** — add `flFlockRadius` (default 64) and
   `iFlockSize` (default 20) to both FGD and C++.
5. **`entity_spritegod`** — accept `spritestartstate` (start-on flag)
   and `targetent` (attach origin to another entity).
6. **`env_explosion_gm`** — either expose `spriteScale` in the FGD or
   drop the field from C++; currently the field exists but cannot be
   set from a map.
7. **`info_node_gunman` / `info_player_gunman`** — expose `nodetype` /
   `master` in the FGD so they are visible in Hammer.

These items are documented but **intentionally not patched in this PR**;
each one needs a focused change with a regression-test pass against a
known map. They are scheduled for follow-up work and listed in
`GUNMAN_LUA_PORT_PLAN.md`.

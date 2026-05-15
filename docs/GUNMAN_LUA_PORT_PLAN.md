# Gunman Chronicles — Lua → Goldsource C++ Porting Plan

This document maps the Garry's Mod (Source) Lua reference implementation
in `game/lua/*` to the current Goldsource C++ stubs in `dlls/*.cpp`,
and lists the **concrete porting work** needed to reach feature
parity. It is intended as the working backlog for the next development
passes.

## 1. Source-of-truth files

| File | Purpose |
|---|---|
| `game/lua/gunman_data.lua` | All ammo types, sound channels, weapon damage tables, default settings, ammo costs, killicons, decals. |
| `game/lua/gunman_shared.lua` | Shared helpers (`EntityCreate`, `BlastDamage`, `ClampedTracerHitPos`, `DoSegmentedBeam`, gaussian-random spread). |
| `game/lua/weapons/gunman_base/` | SWEP base class (menu options, idle sequences, customize animation, view-model FOV). |
| `game/lua/weapons/gunman_weapon_gausspistol/` | Gauss Pistol — 4 fire modes (Pulse/Charge/Rapid/Sniper). |
| `game/lua/weapons/gunman_weapon_beamgun/` | Beam Gun — 3 menu axes (Range, Power/Accuracy, Lightning), tazer mode, power-ball mode, chain mode. |
| `game/lua/weapons/gunman_weapon_chemgun/` | Chemical Gun — Acid/Neutral/Base/Pressure axes. |
| `game/lua/weapons/gunman_weapon_mule/` | Mule (DML) — LaunchType / FlightPathType / DetonationType / PayloadType axes. |
| `game/lua/weapons/gunman_weapon_shotgun/` | Shotcycler / GC Shotgun — shell-count and spread. |
| `game/lua/weapons/gunman_weapon_mechagun/` | Minigun — spin-up state machine, cooling. |
| `game/lua/weapons/gunman_weapon_knife/` | Fists + knife toggle. |
| `game/lua/weapons/gunman_weapon_grenade/` | Hand grenade (3 detonation types). |
| `game/lua/weapons/gunman_weapon_aicore/` | The "Wrench" puzzle weapon. *(Not in C++ yet — see §3.)* |
| `game/lua/entities/gunman_*` | Server-side projectiles, explosions, ammo bases. |

## 2. Per-weapon parity gap

The current C++ stubs implement the weapon shell (spawn, precache,
deploy, holster, basic primary fire) but none of the Gunman fire-mode
selection, customization menu, ammo-cost tables, or projectile
behaviours. The table below makes the gap explicit so that the work can
be picked up incrementally.

### 2.1 Gauss Pistol (`weapon_gausspistol` / `CGaussPistol`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| Fire mode menu (Pulse / Charge / Rapid / Sniper) | ❌ none | Add an `int m_iFireMode` impulse-controlled selector with HUD overlay (analogous to `IMPULSE_FIRE_RPG`). Default 1 (Pulse). |
| Pulse: instant hit-scan, 25 hp dmg, 1 ammo, 0.2s cadence | ⚠️ partial | The current `PrimaryAttack()` does a single 9mm hitscan at 0.3s and consumes 1 round — replace with the GC numbers (`WeaponDamage.GaussPistol.Pulse`, `WeaponAmmoUsage.GaussPistol.Pulse`). |
| Rapid: 1 ammo / 0.1s, 40 hp, projectile at 2500 u/s with random ±2° cone | ❌ | Add a `CGaussProjectile` entity (model `models/dmlcluster.mdl` or custom sprite) with touch-damage + radius-damage from `EntityDamage.GaussPistol.Rapid`. |
| Charge: 10 ammo / 2.1s, 100 hp, projectile at 1500 u/s | ❌ | Same projectile class with `ProjectileType = Charge` flag; touch 50, blast 100, radius 96. |
| Sniper: 10 ammo (unzoomed), 20 ammo (zoomed), beam ring effects, FOV 80→20 zoom, 1.5s lock | ❌ | New `SecondaryAttack()` that switches to sniper mode (bodygroup `11`); a separate `m_iSniperZoom` state machine drives FOV transitions. |
| `customize` animation + `gunman_gausspistol_customize` sound on fire-mode change | ❌ | Tie into the menu code; HL has `SendWeaponAnim` for the playback. |
| Bodygroup swap on sniper deploy (`10` / `11`) | ❌ | `pev->body` toggle on draw. |

### 2.2 Beam Gun (`weapon_beamgun` / `CBeamGun`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| 3-axis menu: Range (1-4), Power/Accuracy (1-4), Lightning (1-3) | ❌ | Three impulses or a sub-menu. Damage scales per `WeaponDamage.Beamgun.Modes[]`. |
| Tazer mode | ❌ | Continuous-beam variant (`env_beam`/`TE_BEAMENTPOINT`) draining 1 ammo every think tick. |
| Power-ball secondary (20 ammo) | ❌ | `CBeamGunBall` projectile with touch=100, blast=100, radius=128, plus think-time blast `ThinkBlast=10, ThinkRadius=128`. |
| Chain mode | ❌ | Lightning chain (`TE_BEAMTORUS`) hitting up to N entities with falling damage (5 hp default). |
| Windup / windown sounds (`egon_windup2*`, `egon_off1`) | ❌ | `EMIT_SOUND` calls at `PrimaryAttack` / `Holster`. |
| Temperature ramp + malfunction lockout (`weaponTemp`, `Malfunction`) | ❌ | A `float m_flBeamTemp` plus a 3-second lockout via `m_flNextPrimaryAttack`. |
| Residual sound on stop (`residual1-4.wav`) | ❌ | One-shot random pick on release. |

### 2.3 Chemical Gun (`weapon_chemicalgun` / `CChemicalGun`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| 4-axis chemistry menu: Acid(0-4) / Neutral(0-4) / Base(0-4) / Pressure(1-5) | ❌ | Same menu pattern as Beam Gun. Defaults `Acid=4, Neutral=2, Base=3, Pressure=3`. |
| Projectile properties driven by chemistry (`skin`, `explode`, `bounce`, `stick`, `smokeBurn`, `airExpireTime`, `damageArea`, `ammoTake`) | ❌ | Add a `CChemBomb` projectile with these fields (see `gunman_weapon_chembomb.lua`). |
| Damage 8 per direct hit, area 32 u, blast on explode (60 dmg / 256 radius — `EntityDamage.Explosions.Chem`) | ❌ | Wire to `RadiusDamage()`. |
| Customize anim/sound (`Changemixture`, `gunman_chemgun_customize`) | ❌ | |

### 2.4 Mule / DML (`weapon_dml` / `CDML`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| 4-axis menu: LaunchType / FlightPathType / DetonationType / PayloadType (all 1-3 or similar) | ❌ | |
| Single vs. dual reload (`gunman_dml_reload` / `gunman_dml_dualreload`) | ❌ | Detect ammo count and pick anim accordingly. |
| Single-rocket vs. spiral launch (`LaunchStandard=1 ammo` vs. `LaunchSpiral=2 ammo`) | ❌ | Two projectile spawn pathways. |
| Cluster payload (`gunman_weapon_grenade_cluster`) | ❌ | Sub-projectile spawner on detonation. |
| Lock-on (`gunman_dml_lock` sound) | ❌ | Aim-trace + sound. |

### 2.5 Shotcycler / GC Shotgun (`weapon_shotcycler` & `weapon_gcshotgun`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| Configurable shell count per shot (1-4) | ⚠️ partial | `CGCShotgun` already exposes the shell-count and spread mode keyvalue; wire it to the actual `FireBulletsPlayer()` call. |
| Shotgun / Riot / Rifle spread profile | ⚠️ partial | Same as above. |
| `gunman_shotgunCock` per-shot sound | ❌ | |

### 2.6 Minigun / Mechagun (`weapon_minigun` / `CMinigun`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| Spin-up before fire (`MechaSpinUp`) | ❌ | `m_flSpinUpTime`. |
| Spin-down on release (`MechaSpinDown`) | ❌ | |
| 0.05 s cadence at full RPM | ❌ | Use `m_flNextPrimaryAttack`. |
| `cust_2MinigunCooled` pickup variant | ❌ | New entity; gives the minigun with `m_iCooled = TRUE` flag. |
| Tracers + per-shot sound (`hks3.wav`) | ❌ | |

### 2.7 Fists / Knife (`weapon_fists` / `CGCFists`)

| Lua feature | C++ status | Required port work |
|---|---|---|
| Hands (10 hp) vs Knife (30 hp) toggle | ⚠️ partial | The toggle exists in C++ — wire the damage values from `WeaponDamage.Knife`. |
| Alternating left-/right-punch animations + sounds | ⚠️ partial | Alternation flag exists; bind anim events. |
| `gunman_hands_idlekickass` idle sound | ❌ | |
| `gunman_hands_knifedraw` / `_knifeholster` sounds | ❌ | |

### 2.8 AI Core / Wrench (`weapon_aicore` — **new**)

This weapon is referenced by 1 map and exists in Lua and in the
uploaded model set (`models/V_aicore.mdl`, `models/W_aicore.mdl`).
There is **no FGD entry and no C++ implementation** today.

In this PR a minimal C++ stub is added so the entity at least spawns and
is interactive (deploys/holsters with the AI core models). The full Lua
behaviour (single hit-scan with 100 hp damage, `aicore_activate` /
`aicore_deactivate` / `aicore_activated` sounds, plug/unplug interaction
on `button_aiwallplug`) remains to be ported in a follow-up.

## 3. Per-entity parity gap (server-side)

The Lua `entities/` folder contains 24 entities. Mapping to existing
C++ stubs:

| Lua entity | Existing C++ counterpart | Status |
|---|---|---|
| `gunman_ammo_base`                          | `dlls/gm_ammo.cpp` (template) | conceptual base class |
| `gunman_ammo_gaussclip`                     | `CGaussClipAmmo`              | ✅ |
| `gunman_ammo_buckshot`                      | `CGCBuckshotAmmo`             | ✅ |
| `gunman_ammo_chemical`                      | `CChemicalAmmo`               | ✅ |
| `gunman_ammo_beamgunclip`                   | `CBeamGunAmmo`                | ✅ |
| `gunman_ammo_minigunclip`                   | `CMinigunAmmo`                | ✅ |
| `gunman_ammo_dmlclip`                       | `CDMLAmmo`                    | ✅ |
| `gunman_ammo_rocketpack`                    | *(none yet)*                  | ❌ — single-rocket pickup. |
| `gunman_explosion`                          | `CEnvExplosionGM` (standard variant) | ✅ |
| `gunman_explosion_small`                    | *(none yet)*                  | ❌ — 60 dmg / 256 radius variant. |
| `gunman_explosion_chem`                     | *(none yet)*                  | ❌ — chem-coloured 60/256 variant. |
| `gunman_weapon_gausspistol_projectile`      | *(none yet)*                  | ❌ — needed for Rapid/Charge fire modes. |
| `gunman_weapon_beamgun_ball`                | *(none yet)*                  | ❌ |
| `gunman_weapon_beamgun_ball_small`          | *(none yet)*                  | ❌ |
| `gunman_weapon_beamgun_chain`               | *(none yet)*                  | ❌ — beam temp-ent only. |
| `gunman_weapon_chembomb`                    | *(none yet)*                  | ❌ |
| `gunman_weapon_grenade_base`                | *(none yet)*                  | ❌ |
| `gunman_weapon_grenade_armed`               | *(none yet)*                  | ❌ |
| `gunman_weapon_grenade_cluster`             | *(none yet)*                  | ❌ |
| `gunman_weapon_grenade_tripmine`            | *(none yet)*                  | ❌ |
| `gunman_weapon_missile_armed`               | *(none yet)*                  | ❌ — Mule rocket in flight. |
| `gunman_aiwallplug`                         | *(none yet — `button_aiwallplug` is the map entity)* | ❌ |
| `gunman_physics_object`                     | uses HL pushable               | n/a |

The above gaps drive the next pass of C++ work. Note that none of
these blocks the **maps themselves** from loading — they only affect
weapon behaviour.

## 4. Shared infrastructure to port from Lua

These are not entity-bound and are needed by multiple weapons:

### 4.1 Damage tables (`gunman_data.lua → WeaponDamage`)

Currently scattered as magic numbers across the C++ stubs. Port to a
single header `dlls/gunman_damage.h`:

```cpp
namespace GunmanDamage {
    namespace GaussPistol {
        const int Pulse        = 8;
        const int Rapid        = 40;
        const int Charge       = 100;
        const int Sniper       = 100;
        const int SniperQuick  = 40;
    }
    namespace Beamgun {
        const int Standard     = 8;
        const int TazerBurst   = 160;
        const int Modes[4]     = {7, 10, 12, 15};
    }
    namespace Knife { const int Hands = 10; const int Knife = 30; }
    namespace AICore { const int Bullet = 100; }
    const int Shotgun   = 7;
    const int Mechagun  = 14;
    const int Mule      = 8;
    const int Grenade   = 8;
    const int Chemgun   = 8;
}
namespace GunmanEntityDamage {
    namespace GaussPistol {
        struct ZoneDmg { int touch, blast, radius; };
        const ZoneDmg Rapid  = { 13, 26, 16 };
        const ZoneDmg Charge = { 50, 100, 96 };
    }
    namespace Beamgun {
        const struct { int touch, blast, radius, thinkBlast, thinkRadius; }
            Ball      = { 100, 100, 128, 10, 128 };
        const struct { int touch, blast, radius; }
            BallSmall = {  20,  10,  64 };
        const int Chain = 5;
    }
    namespace Explosions {
        struct E { int damage, radius; };
        const E Standard = { 256, 512 };
        const E Small    = {  60, 256 };
        const E Chem     = {  60, 256 };
    }
}
```

### 4.2 Ammo-cost tables (`gunman_data.lua → WeaponAmmoUsage`)

| Weapon | Pulse / Std | Charge / PowerBall | Rapid | Sniper | SniperZoomed |
|---|---|---|---|---|---|
| GaussPistol | 1 | 10 | 1 | 10 | 20 |
| Beamgun | 1 | 20 | — | — | — |
| Chemgun | 1 (min) — 3 (max pressure) | — | — | — | — |
| Mule | LaunchStandard=1, LaunchSpiral=2; ReloadSingle=1, ReloadDouble=2 | — | — | — | — |
| Grenade | 1 | — | — | — | — |
| Mechagun | 1 | — | — | — | — |
| Shotgun | shells per shot ∈ {1,2,3,4} | — | — | — | — |

### 4.3 Sound channel definitions (`gunman_data.lua → AutoChannel / WeaponChannel / ItemChannel`)

The Lua file declares ~70 named sound entries with pitch ranges and
volumes. These should be ported as constants in `dlls/gunman_sounds.h`
and precached by the relevant weapon `Precache()` methods. Already
referenced in some stubs (`weapons/dml_fire.wav` etc.) but **not in a
centralized location**.

### 4.4 Decals + killicons

Lua `decals[]` and `killicons[]` are Garry's Mod concepts — they have
no direct GoldSrc equivalent and are out of scope. The decal **names**
(`gunman/decals/pulse`, `gunman/decals/balburn`, `gunman/decals/charge`,
`gunman/decals/cg_red/lime/green/brown`) should be added to
`decals.h` / `dlls/decals.cpp` once the decal textures are uploaded.

## 5. FGD additions identified in this analysis

The following entries are missing from `game/gunman.fgd` and should be
added to expose existing-or-planned behaviour to mappers:

1. `weapon_aicore` — added in this PR.
2. `weapon_SPchemicalgun` — single-player variant; either alias to
   `weapon_chemicalgun` or add a separate point class.
3. `ammo_dmlclip` — 8-round Mule magazine pickup.
4. `random_trigger`, `meteor_god`, `meteor_target`, `lava_god`,
   `entity_clustergod`, `sphere_explosion`, `aiscripted_sequence`,
   `button_aiwallplug`, `hologram_beak`, `hologram_damage`,
   `demoman_mine`, `func_tanklaserrust` — used by stock maps; need
   FGD entries even before C++ implementations exist so that Hammer
   can edit them.
5. `vehicle_tank`, `vehicle_tank_body`, `vehicle_tank_turret`,
   `vehicle_tank_barrel`, `trigger_tankoutofgas`, `trigger_tankshell`,
   `trigger_gunmanteleport` — see `GUNMAN_BSP_ANALYSIS.md`.

Plus the param-level fixes listed at the bottom of
`GUNMAN_FGD_VS_CPP.md`.

## 6. Phased implementation roadmap

1. **Phase 0 (this PR):** correct model paths, document the gap,
   add `weapon_aicore` stub + FGD entry.
2. **Phase 1:** add the missing FGD entries above so maps open
   cleanly in Hammer; alias `ammo_minigunClip` for case-correctness
   in C++.
3. **Phase 2:** port the damage/ammo tables to
   `dlls/gunman_damage.h` and replace the magic numbers in the eight
   weapon stubs.
4. **Phase 3:** implement the four fire modes on the Gauss Pistol
   and the menu state machine on the Beam Gun, Chemical Gun, Mule;
   add the projectile entities they spawn.
5. **Phase 4:** implement the human-NPC family (`monster_human_*`),
   which is what unlocks half the campaign maps.
6. **Phase 5:** implement the vehicle/tank chain, decoration
   entities, hologram and trigger families.
7. **Phase 6:** custom HUD (player_togglehud), save/restore polish,
   killicons/decals when assets are uploaded.

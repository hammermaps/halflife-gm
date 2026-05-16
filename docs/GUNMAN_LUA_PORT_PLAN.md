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
| `game/lua/weapons/gunman_weapon_aicore/` | The "Wrench" puzzle weapon. ✅ Added as `dlls/aicore.cpp` (100-dmg stub). Plug/unplug and sounds are follow-up. |
| `game/lua/entities/gunman_*` | Server-side projectiles, explosions, ammo bases. |

## 2. Per-weapon parity gap

The current C++ stubs implement the weapon shell (spawn, precache,
deploy, holster, basic primary fire) but none of the Gunman fire-mode
selection, customization menu, ammo-cost tables, or projectile
behaviours. The table below makes the gap explicit so that the work can
be picked up incrementally.

### 2.1 Gauss Pistol (`weapon_gausspistol` / `CGaussPistol`)

**Status: ✅ Phase-1 implemented** — see `dlls/gausspistol.cpp` and `dlls/weapons.h`.

| Lua feature | C++ status | Notes |
|---|---|---|
| Fire mode menu (Pulse / Charge / Rapid / Sniper) | ✅ done | `m_iFireMode` (1–4). `SecondaryAttack()` cycles modes (1→2→3→4→1) and plays `GAUSSPISTOL_CUSTOMIZE` anim + DryFire sound. |
| Pulse: instant hitscan, 25 hp dmg, 1 ammo, 0.2 s | ✅ done | `FireModePulse()` — `VECTOR_CONE_1DEGREES`, `gauss_fire4.wav` (pitch 96–112). |
| Rapid: 1 ammo / 0.1 s, projectile 2500 u/s, ±2° cone | ✅ done | `FireModeRapid()` — `CGaussPistolProjectile` (touch 13, blast 26, radius 16), `gauss_fire1.wav`. |
| Charge: 10 ammo / 2.1 s, projectile 1500 u/s | ✅ done | `FireModeCharge()` — `CGaussPistolProjectile` (touch 50, blast 100, radius 96), `gauss_fire2.wav`. |
| Sniper: hold to zoom (FOV 90→80→20), release-to-fire | ✅ done | `FireModeSniper()` / `SniperEnterMode()` / `SniperExitMode()`, `m_iSniperZoom` state machine. Unzoomed dmg 40 (10 ammo) / full-zoom dmg 100 (20 ammo). |
| `customize` animation + sound on fire-mode change | ✅ done | `SendWeaponAnim(GAUSSPISTOL_CUSTOMIZE)` + `DryFire.wav`. |
| `CGaussPistolProjectile` entity (Rapid + Charge) | ✅ done | `LINK_ENTITY_TO_CLASS(gausspistol_proj, …)`, `TE_BEAMFOLLOW` sprite trail, `RadiusDamage`, 5 s expire / water expire. |
| Ammo type → `gausspistol_ammo`, NOCLIP magazine | ✅ done | `GetItemInfo()` updated; old `"9mm"` / clip-20 removed. |
| Save/restore for new fields | ✅ done | `CGaussPistol::m_SaveData[]` in `weapons.cpp`. |
| Bodygroup swap on sniper mode (body 10 / 11) | ⚠️ partial | `SniperEnterMode` plays `GAUSSPISTOL_SNIPERDRAW`; actual body-group write via `pev->body` omitted pending confirmation of correct index in v_guasspistol.mdl. |
| Beam-ring visual effect (sniper shot) | ❌ follow-up | Requires `TE_BEAMTORUS` packet; documented in §5. |
| Per-fire-mode HUD overlay (sprites) | ❌ follow-up | Client-side HUD work scoped for Phase 3. |

### 2.2 Beam Gun (`weapon_beamgun` / `CBeamGun`)

**Status: ✅ Phase-2 implemented** — see `dlls/beamgun.cpp` and `dlls/weapons.h`.

| Lua feature | C++ status | Notes |
|---|---|---|
| 3-axis menu: Range (1-4), Power/Accuracy (1-4), Lightning (1-3) | ✅ done | `SecondaryAttack()` cycles axes (Range→P/A→Lightning→Range), plays `BEAMGUN_CONFIG` anim + `DryFire.wav`. Fields `m_iRange`, `m_iPowerAndAccuracy`, `m_iLightning` with Lua defaults (3, 2, 1). |
| Windup / windown sounds (`egon_windup2`, `egon_windup2_new`, `egon_off1`) | ✅ done | `BeginAttack()` plays windup (egon_windup2 for P/A≥2, egon_windup2_new otherwise); `EndAttack()` plays egon_off1. |
| Hold-to-fire state machine (windup 0.75 s) | ✅ done | `m_iFireState` (BGFIRE_OFF/WINDUP/FIRING). `PrimaryAttack()` advances states; `WeaponIdle()` calls `EndAttack()` on button release. |
| Tazer mode (Range ≤ 2, Lightning=1) | ✅ done | Continuous hitscan; entity within range gets `TazerBurst(160)/P/A` blast + `overheat.wav`; 2-second lockout per burst. |
| Standard beam (Range 3-4, Lightning=1-2) | ✅ done | Hitscan damage from `Modes[]={7,10,12,15}` driven by `m_iPowerAndAccuracy`; area blast (16 u) at impact point; ammo cost = `ceil((5-P/A)*0.5)`. |
| Chain mode (Lightning=2) | ✅ done | Spawns `CBeamGunChain` (new entity) at hit point; spawn rate = `√P/A × 0.25 s`; chain entity applies 5 hp blast each 0.5 s for 5 s then removes. |
| Power-ball secondary (Lightning=3, 20 ammo) | ✅ done | `LaunchPowerBall()` → `CBeamGunBall` projectile at 500 u/s. `CBeamGunBall`: touch=100, blast=100, radius=128, periodic think blast 10/128; spawns up to 18 `CBeamGunBallSmall` sub-projectiles + 3 `CBeamGunChain` arcs; expires after 10 s. |
| `CBeamGunBall` entity | ✅ done | `LINK_ENTITY_TO_CLASS(beamgun_ball, …)` in `beamgun.cpp`. Precaches ball-fly / ball-die sounds. |
| `CBeamGunBallSmall` entity | ✅ done | `LINK_ENTITY_TO_CLASS(beamgun_ball_small, …)` in `beamgun.cpp`. touch=20, blast=10, radius=64, expires 3 s. |
| `CBeamGunChain` entity | ✅ done | `LINK_ENTITY_TO_CLASS(beamgun_chain, …)` in `beamgun.cpp`. RadiusDamage 5 hp / random radius each 0.5 s; TE_SPARKS visual; removes after 10 ticks (~5 s). |
| Temperature ramp + malfunction lockout | ✅ done | `m_flBeamTemp` rises +0.2/tick while firing, falls −0.1/tick idle. At 140 → `m_bMalfunction=TRUE`, 3 s lockout, `electro4.wav`. Clears below 130 after delay. |
| Residual sound on stop (`residual1-4.wav`) | ✅ done | `EndAttack()` picks random residual1-4 on CHAN_ITEM. |
| Save/restore for new fields | ✅ done | `CBeamGun::m_SaveData[]` in `weapons.cpp`: Range, P/A, Lightning, BeamTemp, Malfunction, MalfunctionReset, BallLaunched. |
| Per-axis HUD display | ❌ follow-up | Client-side HUD work scoped for Phase 3. |
| `TE_BEAMENTPOINT` beam visual | ❌ follow-up | Requires client-side event; documented in §5. |

### 2.3 Chemical Gun (`weapon_chemicalgun` / `CChemicalGun`)

**Status: ✅ Phase-2 implemented** — see `dlls/chemicalgun.cpp` and `dlls/weapons.h`.

| Lua feature | C++ status | Notes |
|---|---|---|
| 4-axis chemistry menu: Acid(0-4) / Neutral(0-4) / Base(0-4) / Pressure(1-5) | ✅ done | `SecondaryAttack()` cycles axes (Acid→Neutral→Base→Pressure→Acid), plays `CHEMICALGUN_CUSTOMIZE` anim + `DryFire.wav`. Fields `m_iAcid`, `m_iNeutral`, `m_iBase`, `m_iPressure` with Lua defaults (4, 2, 3, 3). |
| Projectile properties driven by chemistry (`skin`, `explode`, `bounce`, `stick`, `smokeBurn`, `airExpireTime`, `damageArea`, `ammoTake`) | ✅ done | `ChemConfig()` helper derives all properties from Acid/Neutral/Base axes. `CChemBomb` projectile entity carries these flags. |
| Damage 8 per direct hit; blast on explode (60 dmg / 2×damageArea radius); normal expire (damageArea×0.2 dmg / damageArea radius) | ✅ done | `CChemBomb::Detonate()` and `BombTouch()` wired to `RadiusDamage()`. |
| `explode` behaviour (acid>0 && base>0) | ✅ done | Large blast on contact/expire; `TE_EXPLOSION` visual. |
| `bounce` behaviour (explode && neutral dominates) | ✅ done | `MOVETYPE_BOUNCE` on projectile; deflects off surfaces. |
| `stick` behaviour (neutral>2 && components && !bounce) | ✅ done | On first surface/entity touch: `MOVETYPE_NONE`, velocity zeroed. |
| `smokeBurn` behaviour (neutral>2 && (acid>2 || base && !explode)) | ✅ done | Periodic `RadiusDamage` every 0.1 s while stuck. |
| `airExpireTime` (explode→max(1,5-neutral); else 4 s) | ✅ done | `BombThink()` auto-detonates after expire time. |
| `damageArea` formula | ✅ done | smokeBurn→64; explode→clamp(16*(a+b),32,128); else→clamp(32*max(a,b,n),32,128). |
| `skin` (projectile colour) | ✅ done | `pev->skin` set at spawn: 0=green(acid), 1=lime(neutral), 2=brown(a+b), 3=red(base). |
| `ammoTake` per shot | ✅ done | `clamp(floor(1+(a+n+b)/6), 1, 3)`. Ammo deducted from pool (WEAPON_NOCLIP). |
| Launch speed scales with Pressure (200 + 150×P u/s) | ✅ done | `LaunchChemBomb()` uses `CG_LAUNCH_BASE + CG_LAUNCH_SCALE * m_iPressure`. |
| View punch proportional to Pressure | ✅ done | `pev->punchangle.x = -m_iPressure`. |
| Fire sound pitch varies with Pressure | ✅ done | `EMIT_SOUND_DYN` pitch = 90 + 4×P. |
| `Changemixture` anim + `DryFire.wav` on axis change | ✅ done | `SendWeaponAnim(CHEMICALGUN_CUSTOMIZE)` + `DryFire.wav`. |
| `CChemBomb` entity | ✅ done | `LINK_ENTITY_TO_CLASS(chembomb, …)` in `chemicalgun.cpp`. Model: `Tubeball.mdl`. |
| Save/restore for new fields | ✅ done | `CChemicalGun::m_SaveData[]` in `weapons.cpp`: Acid, Neutral, Base, Pressure, MenuAxis. `CChemBomb::m_SaveData[]` in `chemicalgun.cpp`: all gameplay flags, expire time, damage area. |
| Per-axis HUD display / vial sprites | ❌ follow-up | Client-side HUD work scoped for Phase 3. |

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
is interactive (deploys/holsters with the AI core models, fires a
100-damage hitscan attack matching `WeaponDamage.AICore.Bullet`). The
remaining Lua behaviour (`aicore_activate` / `aicore_deactivate` /
`aicore_activated` sounds, plug/unplug interaction on `button_aiwallplug`)
remains to be ported in a follow-up.

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
| `gunman_weapon_gausspistol_projectile`      | ✅ ported as `CGaussPistolProjectile` / `gausspistol_proj` in `dlls/gausspistol.cpp`. |
| `gunman_weapon_beamgun_ball`                | ✅ ported as `CBeamGunBall` / `beamgun_ball` in `dlls/beamgun.cpp`. |
| `gunman_weapon_beamgun_ball_small`          | ✅ ported as `CBeamGunBallSmall` / `beamgun_ball_small` in `dlls/beamgun.cpp`. |
| `gunman_weapon_beamgun_chain`               | ✅ ported as `CBeamGunChain` / `beamgun_chain` in `dlls/beamgun.cpp`. |
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
        const int Pulse        = 25;	// WeaponDamage.GaussPistol.Pulse (from gunman_data.lua)
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
2. **Phase 1 (this PR):** add `ammo_minigunClip` / `ammo_dmlclip` aliases
   for case-correctness in C++; implement Gauss Pistol four fire modes,
   `CGaussPistolProjectile` entity, release-to-fire sniper logic.
3. **Phase 2:** port the damage/ammo tables to
   `dlls/gunman_damage.h` and replace the magic numbers in the remaining
   seven weapon stubs.
4. **Phase 3:** implement the fire-mode state machines on the Beam Gun,
   Chemical Gun, and Mule; add the projectile entities they spawn.
5. **Phase 4:** implement the human-NPC family (`monster_human_*`),
   which is what unlocks half the campaign maps.
6. **Phase 5:** implement the vehicle/tank chain, decoration
   entities, hologram and trigger families.
7. **Phase 6:** custom HUD (player_togglehud), save/restore polish,
   killicons/decals when assets are uploaded.

# Gunman Chronicles — Model Path Mapping

> Generated from a comparison of the model paths hard-coded in the current
> Gunman C++ stubs (`dlls/*.cpp`) against the actual model files that ship
> in this repository under `game/models/`.
>
> **TL;DR:** every existing Gunman C++ file currently references
> `models/gunmanchronicles/<name>.mdl`, but the GoldSrc engine expects
> the models in the location they were uploaded — directly in
> `models/<name>.mdl` (no `gunmanchronicles` subfolder). The mapping
> below documents the correct path for every entity and is the single
> source of truth for fixing the SET_MODEL / PRECACHE_MODEL calls.

Conventions:
- "Old path" — what the C++ currently references (broken: file does not exist).
- "New path" — what the C++ should reference (file exists in `game/models/`).
- A blank "New path" means **no matching asset is present in `game/models/`**;
  the entity has to keep a placeholder (HL stock model) until the GC asset
  is uploaded.
- File-name case must match the on-disk case (some uploaded files start with
  an uppercase letter, e.g. `W_armor.mdl`, `W_beam.mdl`, `V_aicore.mdl`,
  `Gutspile.mdl`, `Xenome.mdl`, `Raptor.mdl`). Linux dedicated servers are
  case-sensitive — this matters in practice.

## Weapons (view / world / player)

| Entity / file | Slot | Old path | New path | Notes |
|---|---|---|---|---|
| `weapon_gausspistol` `gausspistol.cpp` | v | `models/v_9mmhandgun.mdl` *(placeholder)* | `models/v_guasspistol.mdl` | Uploaded file is mis-spelt "guass". Keep as-is or `view` rename later. |
|  | w | `models/w_9mmhandgun.mdl` *(placeholder)* | `models/w_gauss.mdl` | `w_gausst.mdl` is the throwable variant. |
|  | p | `models/p_9mmhandgun.mdl` *(placeholder)* | *(none)* | No `p_gausspistol.mdl`; reuse `models/p_357.mdl` as placeholder. |
| `weapon_shotcycler` `shotcycler.cpp` | v | `models/gunmanchronicles/v_shotcycler.mdl` | `models/v_shotgun.mdl` | The shotcycler/shotgun share one model in this drop. |
|  | w | `models/gunmanchronicles/w_shotcycler.mdl` | `models/w_shotgun.mdl` | |
|  | p | `models/gunmanchronicles/p_shotgun.mdl` | `models/p_shotgun.mdl` | |
| `weapon_chemicalgun` `chemicalgun.cpp` | v | `models/gunmanchronicles/v_chemicalgun.mdl` | `models/v_chemgun.mdl` | Filename uses `chemgun`, not `chemicalgun`. |
|  | w | `models/gunmanchronicles/w_chemicalgun.mdl` | `models/w_chemgun.mdl` | |
|  | p | `models/gunmanchronicles/p_chemicalgun.mdl` | *(none)* | Fall back to `models/p_crossbow.mdl`. |
| `weapon_minigun` `minigun.cpp` | v | `models/gunmanchronicles/v_minigun.mdl` | `models/v_mechagun.mdl` | The GC name for the minigun is **mechagun**. |
|  | w | `models/gunmanchronicles/w_minigun.mdl` | `models/w_mechagun.mdl` | |
|  | p | `models/gunmanchronicles/p_minigun.mdl` | *(none)* | Fall back to `models/p_9mmar.mdl`. |
| `weapon_dml` `dml.cpp` | v | `models/gunmanchronicles/v_dml.mdl` | `models/v_dml.mdl` | |
|  | w | `models/gunmanchronicles/w_dml.mdl` | `models/w_dml.mdl` | |
|  | p | `models/gunmanchronicles/p_crossbow.mdl` | `models/p_crossbow.mdl` | |
|  | projectile | `models/gunmanchronicles/dmlrocket.mdl` | `models/dmlrocket.mdl` | |
|  | cluster | `models/gunmanchronicles/dmlcluster.mdl` | `models/dmlcluster.mdl` | |
| `weapon_beamgun` `beamgun.cpp` | v | `models/gunmanchronicles/v_beam.mdl` | `models/v_beam.mdl` | |
|  | w | `models/gunmanchronicles/w_beam.mdl` | `models/W_beam.mdl` | **Capital W** on disk. |
|  | p | `models/gunmanchronicles/p_egon.mdl` | `models/p_egon.mdl` | |
| `weapon_fists` `gm_fists.cpp` | v | `models/gunmanchronicles/v_hands.mdl` | `models/v_hands.mdl` | |
|  | w (knife) | `models/gunmanchronicles/w_knife.mdl` | `models/w_knife.mdl` | A separate `models/knife.mdl` exists for the world-thrown blade. |
|  | p | `models/gunmanchronicles/p_crowbar.mdl` | `models/p_crowbar.mdl` | |
| `weapon_gcshotgun` `gm_gcshotgun.cpp` | v | `models/gunmanchronicles/v_gcshotgun.mdl` | `models/v_shotgun.mdl` | No dedicated `v_gcshotgun.mdl` was uploaded; reuse shotgun. |
|  | w | `models/gunmanchronicles/w_shotgun.mdl` | `models/w_shotgun.mdl` | |
|  | p | `models/gunmanchronicles/p_shotgun.mdl` | `models/p_shotgun.mdl` | |
| `weapon_aicore` *(NEW — see GUNMAN_LUA_PORT_PLAN.md)* | v | n/a | `models/V_aicore.mdl` | **Capital V**. |
|  | w | n/a | `models/W_aicore.mdl` | **Capital W**. |

## Ammo & item pickups

| Entity | Old path | New path | Notes |
|---|---|---|---|
| `ammo_gaussclip` (`CGaussClipAmmo`) | `models/gunmanchronicles/w_gausspistolclip.mdl` | `models/guassammo.mdl` | "guass" typo on disk. |
| `ammo_shotcycler` (`CShotCyclerAmmo`) | `models/w_shotbox.mdl` | `models/shotgunammo.mdl` | |
| `ammo_chemical` (`CChemicalAmmo`) | `models/gunmanchronicles/chem_ammo.mdl` | `models/chem_ammo.mdl` | |
| `ammo_minigun` (`CMinigunAmmo`) | `models/w_chainammo.mdl` | `models/mechammo.mdl` | Mechagun → mechammo. |
| `ammo_dml` (`CDMLAmmo`) | `models/gunmanchronicles/mechammo.mdl` *(wrong)* | `models/dmlammo.mdl` | The correct DML magazine model. |
| `ammo_beamgun` (`CBeamGunAmmo`) | `models/w_gaussammo.mdl` *(wrong)* | `models/beamgunammo.mdl` | |
| `ammo_gcbuckshot` (`CGCBuckshotAmmo`) | `models/gunmanchronicles/shotgunammo.mdl` | `models/shotgunammo.mdl` | |
| `ammo_gcgaussclip` (`CGCGaussClipAmmo`) | `models/gunmanchronicles/guassammo.mdl` | `models/guassammo.mdl` | |
| `ammo_gcminigunclip` (`CGCMinigunClipAmmo`) | `models/gunmanchronicles/mechammo.mdl` | `models/mechammo.mdl` | |
| `ammo_dmlsingle` (`CDMLSingleAmmo`) | `models/gunmanchronicles/dmlrocket.mdl` | `models/dmlrocket.mdl` | |
| `ammo_beamgunclip` (`CBeamGunClipAmmo`) | `models/gunmanchronicles/beamgunammo.mdl` | `models/beamgunammo.mdl` | |
| `item_gascan` (`CItemGasCan`) | `models/gunmanchronicles/gastank.mdl` | `models/gastank.mdl` | |
| `item_armor` (`CItemArmor`) | `models/gunmanchronicles/w_armor.mdl` | `models/W_armor.mdl` | **Capital W**. |

## Monsters / NPCs

Most monster stubs still point at stock Half-Life models (`zombie.mdl`, `houndeye.mdl`, `bullsquid.mdl`, etc.). The Gunman model set uploaded to `game/models/` contains only a few that can be drop-in replacements; the rest remain placeholders until the proper GC monster MDLs are uploaded.

| Entity | Current placeholder | Best available GC model | Notes |
|---|---|---|---|
| `monster_houndeye_gm` | `models/houndeye.mdl` | *(none uploaded)* | Keep placeholder. |
| `monster_dinnerjacket` | `models/zombie.mdl` | *(none uploaded)* | Keep placeholder. |
| `monster_geneworm` | `models/big_mom.mdl` | *(none uploaded)* | Keep placeholder. The GC end-boss `models/endboss.mdl` is a *different* entity (`monster_endboss`). |
| `monster_xenome` | `models/headcrab.mdl` | `models/Xenome.mdl` | **Capital X** on disk. Gibs: `Xenomegibs.mdl`. Variants: `xenome2.mdl`, `Xenomex.mdl`, `xenomei.mdl`. |
| `monster_shockroach` | `models/w_squeak.mdl` | *(none uploaded)* | Keep placeholder. |
| `monster_massasaur` | `models/bullsquid.mdl` | `models/Raptor.mdl` | "Massasaur" is the raptor in GC. Gibs: `raptorgibs.mdl`. |

## Map-utility / decoration entities

| Entity | Old path | New path | Notes |
|---|---|---|---|
| `decore_butterflyflock` | `models/gunmanchronicles/butterfly.mdl` | `models/butterfly.mdl` | |
| `decore_gutspile` | `models/gunmanchronicles/gutspile.mdl` | `models/Gutspile.mdl` | **Capital G**. |
| `entity_digitgod` (digit sprite) | `models/gunmanchronicles/digits.mdl` | `models/digits.mdl` | Used as a sprite via `CSprite::SpriteCreate`, not as a studio model. |
| `gunman_cycler` | *user-provided via `model` key* | n/a | Uses any of the 300+ uploaded studio models. |
| `decore_asteroid` | *user-provided via `model` key* | `models/Asteroid.mdl` (default) | Maps frequently set `model "models/Asteroid.mdl"`. |
| `decore_spacedebris` | *user-provided via `modelname` key* | typical: `models/decoregibs.mdl`, `models/decoregibs2.mdl` (latter is **not** uploaded — see `GUNMAN_BSP_ANALYSIS.md`). | |

## Projectile / effect entities seen in GC Lua (no C++ yet)

These are the Lua server-side entities listed in `game/lua/entities/*`.
They are documented here because porting them to C++ will require these
exact model paths to match the GC asset set.

| Lua entity | C++ port should use |
|---|---|
| `gunman_weapon_gausspistol_projectile` | (sprite-based — `sprites/rocket.spr` + glow) |
| `gunman_weapon_beamgun_ball` / `_ball_small` | `models/dmlcluster.mdl` (closest match — small energy ball) or custom sprite |
| `gunman_weapon_beamgun_chain` | beam temp-ent only, no model |
| `gunman_weapon_chembomb` | `models/chem_ammo.mdl` (re-used as the canister) |
| `gunman_weapon_missile_armed` (Mule/DML rocket in flight) | `models/dmlrocket.mdl` |
| `gunman_weapon_grenade_*` | `models/grenadecore.mdl`, `models/grande.mdl`, `models/grande_tank.mdl` |
| `gunman_weapon_grenade_cluster` | `models/dmlcluster.mdl` |
| `gunman_weapon_grenade_tripmine` | `models/demomine.mdl` |
| `gunman_aiwallplug` (AI core door plug) | `models/aiwallplug.mdl` |
| `gunman_physics_object` | re-uses world's pushable model |

## Action items applied to the C++ side

The above mappings have been propagated to the following source files in
this branch (every `models/gunmanchronicles/...` reference replaced with
the on-disk path):

- `dlls/gausspistol.cpp`
- `dlls/shotcycler.cpp`
- `dlls/chemicalgun.cpp`
- `dlls/minigun.cpp`
- `dlls/dml.cpp`
- `dlls/beamgun.cpp`
- `dlls/gm_fists.cpp`
- `dlls/gm_gcshotgun.cpp`
- `dlls/gm_ammo.cpp`
- `dlls/gm_mapents.cpp` (butterfly, gutspile, digits)
- `dlls/gm_xenome.cpp` (monster_xenome → `models/Xenome.mdl`)
- `dlls/gm_massasaur.cpp` (monster_massasaur → `models/Raptor.mdl`)

Files that still keep an HL placeholder model (no GC replacement on disk):

- `dlls/gm_dinnerjacket.cpp`
- `dlls/gm_houndeye.cpp`
- `dlls/gm_geneworm.cpp`
- `dlls/gm_shockroach.cpp`

These are tagged with a `// TODO: GC asset missing — keep placeholder`
comment so the next pass over the project can fix them when assets land.

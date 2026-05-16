/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// gunman_sounds.h -- Centralised sound-path constants
// ported from game/lua/gunman_data.lua (AutoChannel,
// WeaponChannel, ItemChannel).
//
// All paths are relative to game/sound/ (the standard
// GoldSrc sound directory).  Use with PRECACHE_SOUND()
// and EMIT_SOUND().
//=========================================================

#ifndef GUNMAN_SOUNDS_H
#define GUNMAN_SOUNDS_H

// -----------------------------------------------------------------------
// AutoChannel  (CHAN_AUTO — fire-and-forget ambient sounds)
// -----------------------------------------------------------------------
// HUD sounds are in common/, not weapons/
#define GMSND_HUDON                "common/wpn_hudon.wav"
#define GMSND_HUDOFF               "common/wpn_hudoff.wav"

// Debris impact (random 1-3)
#define GMSND_DEBRIS1              "weapons/debris1.wav"
#define GMSND_DEBRIS2              "weapons/debris2.wav"
#define GMSND_DEBRIS3              "weapons/debris3.wav"

// Large explosion (standard gunman_explosion)
#define GMSND_KABAM1               "weapons/kabam1.wav"
#define GMSND_KABAM2               "weapons/kabam2.wav"
#define GMSND_KABAM3               "weapons/kabam3.wav"

// Medium explosion (gunman_explosion_small)
#define GMSND_KABOOM1              "weapons/kaboom1.wav"
#define GMSND_KABOOM2              "weapons/kaboom2.wav"
#define GMSND_KABOOM3              "weapons/kaboom3.wav"

// Chemical explosion (gunman_explosion_chem)
#define GMSND_EXPLODE3             "weapons/explode3.wav"
#define GMSND_EXPLODE4             "weapons/explode4.wav"
#define GMSND_EXPLODE5             "weapons/explode5.wav"

// Misc
#define GMSND_DML_LOCK             "weapons/dml_lock.wav"
// NOTE: mainframe_hurt02.wav / mainframe007.wav / mainframe001.wav are not present
// in game/sound/mainframe/. These names are placeholders matching the Lua data file;
// add the constants back once the assets land in game/sound/mainframe/.
// #define GMSND_MAINFRAME_HURT    "mainframe/mainframe_hurt02.wav"
// #define GMSND_MAINFRAME_FOUND   "mainframe/mainframe007.wav"
// #define GMSND_MAINFRAME_HIT     "mainframe/mainframe001.wav"

// -----------------------------------------------------------------------
// WeaponChannel  (CHAN_WEAPON — primary weapon fire sounds)
// -----------------------------------------------------------------------

// DML / Mule
#define GMSND_MULE_FIRE            "weapons/dml_fire.wav"

// Grenade / Mine
#define GMSND_MINE_CHARGE          "weapons/mine_charge.wav"

// Gauss Pistol
#define GMSND_GAUSS_PULSE          "weapons/gauss_fire4.wav"
#define GMSND_GAUSS_CHARGE         "weapons/gauss_fire2.wav"
#define GMSND_GAUSS_RAPID          "weapons/gauss_fire1.wav"
#define GMSND_GAUSS_SNIPER_ZOOM    "weapons/gsnipe_zoom.wav"
#define GMSND_GAUSS_SNIPER_RUNFIRE "weapons/sniperunzoom.wav"
#define GMSND_GAUSS_SNIPER_FIRE    "weapons/sniperzoom.wav"

// Shotgun
#define GMSND_SHOTGUN_FIRE         "weapons/sbarrel1.wav"
#define GMSND_SHOTGUN_COCK         "weapons/shotgun_cock_heavy.wav"

// Minigun
#define GMSND_MECHA_FIRE           "weapons/hks3.wav"
#define GMSND_MECHA_SPINUP         "weapons/MechaSpinUp.wav"
#define GMSND_MECHA_SPINDOWN       "weapons/MechaSpinDown.wav"

// Beam Gun
#define GMSND_BEAMGUN_TAZE         "weapons/overheat.wav"
#define GMSND_BEAMGUN_WINDUP       "weapons/egon_windup2.wav"
#define GMSND_BEAMGUN_WINDUP2      "weapons/egon_windup2_new.wav"
#define GMSND_BEAMGUN_OFF          "weapons/egon_off1.wav"
// Beam Gun electro: only weapons/electro4.wav exists in game/sound/weapons/.
// electro5.wav and electro6.wav are referenced by the Lua data file but the
// GoldSrc assets were not ported yet — all three aliases point at electro4 for now.
#define GMSND_BEAMGUN_ELECTRO1     "weapons/electro4.wav"
#define GMSND_BEAMGUN_ELECTRO2     "weapons/electro4.wav"
#define GMSND_BEAMGUN_ELECTRO3     "weapons/electro4.wav"
#define GMSND_BEAMGUN_CONFIG       "weapons/beamgun_config.wav"

// Shared / UI
#define GMSND_DRYFIRE              "weapons/DryFire.wav"
#define GMSND_DML_CUSTOMIZE        "weapons/dml_customize.wav"
#define GMSND_DML_RELOAD           "weapons/dml_reload.wav"
#define GMSND_DML_DUALRELOAD       "weapons/dml_dualreload.wav"

// Fists / Knife
#define GMSND_RIGHTPUNCH1          "weapons/RightPunch.wav"
#define GMSND_RIGHTPUNCH2          "weapons/RightPunch2.wav"
#define GMSND_RIGHTPUNCH3          "weapons/RightPunch3.wav"
#define GMSND_LEFTPUNCH1           "weapons/LeftPunch.wav"
#define GMSND_LEFTPUNCH2           "weapons/LeftPunch2.wav"
#define GMSND_LEFTPUNCH3           "weapons/LeftPunch3.wav"
#define GMSND_HANDS_MISS           "weapons/cbar_miss1.wav"
#define GMSND_HANDS_IDLEKICKASS    "weapons/Hands_IdleKickAss_F0.wav"
#define GMSND_KNIFE_ATTACK1        "weapons/KnifeAttack1.wav"
#define GMSND_KNIFE_ATTACK2        "weapons/KnifeAttack2.wav"
#define GMSND_KNIFE_DRAW           "weapons/KnifeDraw.wav"
#define GMSND_KNIFE_HOLSTER        "weapons/KnifeHolster.wav"

// Chemical Gun
#define GMSND_CHEMGUN_FIZZY        "weapons/fizzy1.wav"
#define GMSND_CHEMGUN_SPROG1       "weapons/sprog1.wav"
#define GMSND_CHEMGUN_SPROG2       "weapons/sprog2.wav"
#define GMSND_CHEMGUN_CGBOUNCE     "weapons/cgbounce.wav"
#define GMSND_CHEMGUN_FIRE         "weapons/cg-fire.wav"
#define GMSND_CHEMGUN_EMPTY        "weapons/empty.wav"
#define GMSND_CHEMGUN_CUSTOMIZE    "weapons/chemgun_customize.wav"

// AI Core / Mainframe
#define GMSND_AICORE_AIPLUG        "mainframe/aiplug_deactivate_gs.wav"

// -----------------------------------------------------------------------
// ItemChannel  (CHAN_ITEM — secondary / item sounds)
// -----------------------------------------------------------------------

// DML
#define GMSND_MULE_ROCKETFLY       "weapons/rocket1.wav"

// Grenade / Mine
#define GMSND_MINE_ACTIVATE        "weapons/mine_activate.wav"
#define GMSND_MINE_DEPLOY          "weapons/mine_deploy.wav"
#define GMSND_GRENADE_HIT1         "weapons/grenade_hit1.wav"
#define GMSND_GRENADE_HIT2         "weapons/grenade_hit2.wav"
#define GMSND_GRENADE_HIT3         "weapons/grenade_hit3.wav"
#define GMSND_DML_FRAGMENT         "weapons/dml_fragment.wav"

// Beam Gun
#define GMSND_BEAMGUN_RESIDUAL1    "weapons/residual1.wav"
#define GMSND_BEAMGUN_RESIDUAL2    "weapons/residual2.wav"
#define GMSND_BEAMGUN_RESIDUAL3    "weapons/residual3.wav"
#define GMSND_BEAMGUN_RESIDUAL4    "weapons/residual4.wav"
#define GMSND_BEAMGUN_BALLFLY      "weapons/ball-fly.wav"
#define GMSND_BEAMGUN_BALLDIE      "weapons/ball-die.wav"

// Pickups — gun pickup is under items/, not weapons/
#define GMSND_GUN_PICKUP           "items/gunpickup2.wav"
#define GMSND_AMMO_PICKUP          "items/9mmclip1.wav"

// Shell casings (random 1-3)
#define GMSND_SSHELL1              "weapons/sshell1.wav"
#define GMSND_SSHELL2              "weapons/sshell2.wav"
#define GMSND_SSHELL3              "weapons/sshell3.wav"

// Gauss Pistol projectile impacts
#define GMSND_GAUSS_SPRITEHIT      "weapons/gauss_spritesmall.wav"
#define GMSND_GAUSS_SPRITEBIGHIT   "weapons/gauss_spritebig.wav"

// AI Core / Mainframe
#define GMSND_AICORE_DEACTIVATE    "mainframe/aiplug_deactivate_gs.wav"
#define GMSND_AICORE_ACTIVATE      "mainframe/aiplug_activate_gs.wav"
// rebar_psychomotorstartup.wav lives in ambience/, not mainframe/
#define GMSND_AICORE_ACTIVATED     "ambience/rebar_psychomotorstartup.wav"

#endif // GUNMAN_SOUNDS_H

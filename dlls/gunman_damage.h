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
// gunman_damage.h -- Centralised damage and ammo-cost tables
// ported from game/lua/gunman_data.lua (WeaponDamage,
// EntityDamage, WeaponAmmoUsage).
//
// Include this header instead of scattering magic numbers
// across weapon files.
//=========================================================

#ifndef GUNMAN_DAMAGE_H
#define GUNMAN_DAMAGE_H

// -----------------------------------------------------------------------
// WeaponDamage  (direct / hitscan damage dealt by each weapon)
// -----------------------------------------------------------------------
namespace GunmanDamage
{
	namespace GaussPistol
	{
		const int Pulse       = 25;   // Pulse fire hitscan (corrected from Lua 8)
		const int Rapid       = 40;   // Rapid projectile touch
		const int Charge      = 100;  // Charge projectile touch
		const int Sniper      = 100;  // Sniper full-zoom shot
		const int SniperQuick = 40;   // Sniper unzoomed / partial-zoom shot
	}

	namespace Beamgun
	{
		const int Standard   = 8;        // base hitscan tick damage
		const int TazerBurst = 160;      // tazer single-burst damage
		// P/A 1-4 mode damage escalation
		const int Modes[4]   = { 7, 10, 12, 15 };
	}

	namespace Knife
	{
		const int Hands = 10;  // fists
		const int Knife = 30;  // knife
	}

	namespace AICore
	{
		const int Bullet = 100;  // plug-hit hitscan
	}

	const int Shotgun  = 7;   // per-pellet damage
	const int Mechagun = 14;  // per-bullet damage
	const int Mule     = 8;   // DML direct touch
	const int Grenade  = 8;   // grenade direct touch
	const int Chemgun  = 8;   // chem-bomb direct hit
}

// -----------------------------------------------------------------------
// EntityDamage  (damage dealt by spawned projectile / explosion entities)
// -----------------------------------------------------------------------
namespace GunmanEntityDamage
{
	namespace GaussPistol
	{
		struct ZoneDmg { int touch, blast, radius; };
		const ZoneDmg Rapid  = { 13, 26,  16 };
		const ZoneDmg Charge = { 50, 100, 96 };
	}

	namespace Beamgun
	{
		struct BallDmg { int touch, blast, radius, thinkBlast, thinkRadius; };
		const BallDmg Ball      = { 100, 100, 128, 10, 128 };

		struct SmallDmg { int touch, blast, radius; };
		const SmallDmg BallSmall = { 20, 10, 64 };

		const int Chain = 5;  // chain-arc RadiusDamage per tick
	}

	namespace Explosions
	{
		struct E { int damage, radius; };
		const E Standard = { 256, 512 };  // gunman_explosion
		const E Small    = {  60, 256 };  // gunman_explosion_small
		const E Chem     = {  60, 256 };  // gunman_explosion_chem
	}

	namespace DML
	{
		const int ClusterDmg    = 40;
		const float ClusterRad  = 100.0f;
		const int ExplosiveDmg  = 100;
		const float ExplosiveRad = 150.0f;
	}
}

// -----------------------------------------------------------------------
// WeaponAmmoUsage  (rounds consumed per action)
// -----------------------------------------------------------------------
namespace GunmanAmmoUsage
{
	namespace GaussPistol
	{
		const int Pulse       = 1;   // ammo per Pulse shot
		const int Rapid       = 1;   // ammo per Rapid shot
		const int Charge      = 10;  // ammo per Charge shot
		const int Sniper      = 10;  // ammo for unzoomed sniper shot
		const int SniperZoomed = 20; // ammo for full-zoom sniper shot
	}

	namespace Beamgun
	{
		const int Standard  = 1;   // ammo per continuous-fire tick
		const int PowerBall = 20;  // ammo for power-ball launch
	}

	// Chemgun: clamped 1-3 depending on acid+neutral+base mix (see ChemConfig())

	namespace Mule
	{
		const int LaunchStandard = 1;  // single missile
		const int LaunchSpiral   = 2;  // spiral (two missiles)
		const int ReloadSingle   = 1;
		const int ReloadDouble   = 2;
	}

	namespace Grenade
	{
		const int Standard = 1;  // one grenade per throw
	}

	namespace Mechagun
	{
		const int Standard = 1;  // one bullet per trigger tick
	}

	namespace Shotgun
	{
		// ShellLevels: 1, 2, 3, or 4 — player-configurable.
		// Pellets fired = 5 * shell_count  (matches Lua bullet.Num = 5 * CurrentShellCost)
		const int ShellMin = 1;
		const int ShellMax = 4;
	}
}

#endif // GUNMAN_DAMAGE_H

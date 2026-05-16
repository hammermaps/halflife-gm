/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// weapon_minigun — Gunman Chronicles Mechagun (Minigun)
//
// Ported from:
//   game/lua/weapons/gunman_weapon_mechagun/shared.lua
//   game/lua/gunman_data.lua  (damage / ammo-usage tables)
//
// Two fire modes:
//   Normal  — single burst, 0.2 s cadence, plays "firenormal" sequence.
//   Turbo   — SecondaryAttack spins up (2.07 s "spinup" anim), then
//             continuous fire at 0.05 s cadence with looping "fireloop"
//             / "idleloop" sequences.  SecondaryAttack again spins down
//             (2.07 s "spindown" anim) and returns to normal mode.
//
// Temperature system:
//   Rises +2 per 0.1 s think while turbo-firing.
//   Falls -1 per 0.1 s think otherwise.
//   Malfunction lockout (3 s) when temp reaches 162.
//   The "cooled" pickup variant (cust_2MinigunCooled) sets m_iCooled=TRUE
//   which halves the temperature rise rate.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "gunman_damage.h"
#include "gunman_sounds.h"

// -----------------------------------------------------------------------
// Fire-state constants
// -----------------------------------------------------------------------
#define MGSTATE_NORMAL       0   // single-shot normal mode
#define MGSTATE_SPINUP       1   // spinning up toward turbo mode
#define MGSTATE_TURBO        2   // spinning / turbo mode active
#define MGSTATE_SPINDOWN     3   // spinning down, returning to normal
#define MGSTATE_MALFUNCTION  4   // overheated — locked out

// -----------------------------------------------------------------------
// Timing constants  (derived from v_mechagun.mdl sequence data)
//   spinup:    31 frames @ 15 fps → 2.067 s  (fire enabled at 2.067−0.15)
//   spindown:  31 frames @ 15 fps → 2.067 s
//   fireloop:  31 frames @ 45 fps → 0.689 s
//   idleloop:  31 frames @ 45 fps → 0.689 s
//   arming:    31 frames @ 20 fps → 1.550 s
//   holster:   31 frames @ 20 fps → 1.550 s
// -----------------------------------------------------------------------
#define MG_SPINUP_TIME          1.917f   // seconds until turbo fires (2.067 - 0.15)
#define MG_SPINDOWN_TIME        2.067f   // seconds for spindown to complete
#define MG_MALFUNCTION_LOCKOUT  3.0f     // lockout duration on overheat (seconds)
#define MG_NORMAL_FIRE_RATE     0.2f     // seconds between shots in normal mode
#define MG_TURBO_FIRE_RATE      0.05f    // seconds between shots in turbo mode
#define MG_ANIM_LOOP_TIME       0.689f   // fireloop / idleloop loop period
#define MG_DEPLOY_TIME          1.550f   // arming animation duration
#define MG_HOLSTER_TIME         1.550f   // holster animation duration

// -----------------------------------------------------------------------
// Temperature constants
// -----------------------------------------------------------------------
#define MG_TEMP_RISE        2.0f    // temp added per 0.1 s while turbo-firing
#define MG_TEMP_FALL        1.0f    // temp removed per 0.1 s otherwise
#define MG_TEMP_MALFUNCTION 162.0f  // threshold that triggers malfunction
#define MG_TEMP_RESET       160.0f  // temperature after malfunction clears
#define MG_TEMP_THINK       0.1f    // temperature polling interval (seconds)

// -----------------------------------------------------------------------
// Animation indices  (v_mechagun.mdl — verified from model header)
// -----------------------------------------------------------------------
enum minigun_e
{
	MINIGUN_IDLE        = 0,   // "idle"         — 31 frames @ 15 fps
	MINIGUN_IDLEINSPECT = 1,   // "idleinspect"  — 53 frames @ 20 fps
	MINIGUN_FIRENORMAL  = 2,   // "firenormal"   —  6 frames @ 15 fps
	MINIGUN_SPINUP      = 3,   // "spinup"       — 31 frames @ 15 fps
	MINIGUN_FIRELOOP    = 4,   // "fireloop"     — 31 frames @ 45 fps
	MINIGUN_SPINDOWN    = 5,   // "spindown"     — 31 frames @ 15 fps
	MINIGUN_DRAW        = 6,   // "arming"       — 31 frames @ 20 fps
	MINIGUN_IDLELOOP    = 7,   // "idleloop"     — 31 frames @ 45 fps
	MINIGUN_HOLSTER     = 8    // "holster"      — 31 frames @ 20 fps
};

LINK_ENTITY_TO_CLASS( weapon_minigun, CMinigun );

void CMinigun::Spawn( )
{
	Precache();
	m_iId = WEAPON_MINIGUN;
	SET_MODEL( ENT(pev), "models/w_mechagun.mdl" );

	m_iDefaultAmmo = MINIGUN_DEFAULT_GIVE;

	m_iFireState       = MGSTATE_NORMAL;
	m_flSpinTime       = 0;
	m_flWeaponTemp     = 0;
	m_flMalfunctionEnd = 0;
	m_flNextTempThink  = 0;
	m_flNextAnimTime   = 0;
	m_iCooled          = FALSE;

	FallInit();
}

void CMinigun::Precache( void )
{
	PRECACHE_MODEL( "models/v_mechagun.mdl" );
	PRECACHE_MODEL( "models/w_mechagun.mdl" );
	PRECACHE_MODEL( "models/p_9mmar.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );

	PRECACHE_SOUND( GMSND_MECHA_FIRE );
	PRECACHE_SOUND( GMSND_MECHA_SPINUP );
	PRECACHE_SOUND( GMSND_MECHA_SPINDOWN );

	m_usMinigun = PRECACHE_EVENT( 1, "events/mp51.sc" );
}

int CMinigun::GetItemInfo( ItemInfo *p )
{
	p->pszName   = STRING( pev->classname );
	p->pszAmmo1  = "minigun_ammo";
	p->iMaxAmmo1 = MINIGUN_MAX_CARRY;
	p->pszAmmo2  = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip  = WEAPON_NOCLIP;
	p->iSlot     = 4;
	p->iPosition = 1;
	p->iFlags    = 0;
	p->iId       = m_iId = WEAPON_MINIGUN;
	p->iWeight   = MINIGUN_WEIGHT;

	return 1;
}

int CMinigun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CMinigun::Deploy( )
{
	m_iFireState   = MGSTATE_NORMAL;
	m_flWeaponTemp = 0;
	m_flNextAnimTime = 0;
	return DefaultDeploy( "models/v_mechagun.mdl", "models/p_9mmar.mdl", MINIGUN_DRAW, "mp5" );
}

// -----------------------------------------------------------------------
// Helper — begin the spin-down sequence
// -----------------------------------------------------------------------
static void MinigunBeginSpinDown( CMinigun *pGun )
{
	pGun->m_iFireState = MGSTATE_SPINDOWN;
	pGun->m_flSpinTime = gpGlobals->time + MG_SPINDOWN_TIME;
	EMIT_SOUND( ENT( pGun->m_pPlayer->pev ), CHAN_WEAPON,
		GMSND_MECHA_SPINDOWN, 1.0f, ATTN_NORM );
	pGun->SendWeaponAnim( MINIGUN_SPINDOWN );
	pGun->m_flNextPrimaryAttack = pGun->m_flNextSecondaryAttack =
		UTIL_WeaponTimeBase() + MG_SPINDOWN_TIME + 0.1f;
}

void CMinigun::PrimaryAttack( void )
{
	// Don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	// Block fire during transitions / malfunction
	if ( m_iFireState == MGSTATE_SPINUP   ||
	     m_iFireState == MGSTATE_SPINDOWN ||
	     m_iFireState == MGSTATE_MALFUNCTION )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = BRIGHT_GUN_FLASH;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= GunmanAmmoUsage::Mechagun::Standard;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// Shell eject
	Vector vecShellVelocity = m_pPlayer->pev->velocity
		+ gpGlobals->v_right * RANDOM_FLOAT( 50, 70 )
		+ gpGlobals->v_up    * RANDOM_FLOAT( 100, 150 )
		+ gpGlobals->v_forward * 25;
	EjectBrass( pev->origin + m_pPlayer->pev->view_ofs
		+ gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6,
		vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL );

	Vector vecSrc    = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON,
		GMSND_MECHA_FIRE, 1.0f, ATTN_NORM, 0, 100 );

	// Spread: randomised narrow cone matching Lua (0.02–0.04 cone each axis)
	float cone = RANDOM_FLOAT( 0.02f, 0.04f );
	Vector vecSpread( cone, cone, 0 );

	m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread,
		8192, BULLET_PLAYER_MP5, 0, GunmanDamage::Mechagun,
		m_pPlayer->pev, m_pPlayer->random_seed );

	m_pPlayer->pev->punchangle.x = -1.0f;

	if ( m_iFireState == MGSTATE_NORMAL )
	{
		// Normal mode: play single-shot animation, slower cadence
		SendWeaponAnim( MINIGUN_FIRENORMAL );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack =
			UTIL_WeaponTimeBase() + MG_NORMAL_FIRE_RATE;
	}
	else // MGSTATE_TURBO
	{
		// Turbo mode: animation driven by ItemPostFrame loop
		m_flNextPrimaryAttack = m_flNextSecondaryAttack =
			UTIL_WeaponTimeBase() + MG_TURBO_FIRE_RATE;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0f;
}

void CMinigun::SecondaryAttack( void )
{
	// Ignore during transitions or malfunction
	if ( m_iFireState == MGSTATE_SPINUP   ||
	     m_iFireState == MGSTATE_SPINDOWN ||
	     m_iFireState == MGSTATE_MALFUNCTION )
	{
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	if ( m_iFireState == MGSTATE_NORMAL )
	{
		// Spin up to turbo mode
		m_iFireState = MGSTATE_SPINUP;
		m_flSpinTime = gpGlobals->time + MG_SPINUP_TIME;
		m_flNextAnimTime = 0;
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
			GMSND_MECHA_SPINUP, 1.0f, ATTN_NORM );
		SendWeaponAnim( MINIGUN_SPINUP );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack =
			UTIL_WeaponTimeBase() + MG_SPINUP_TIME + 0.1f;
	}
	else // MGSTATE_TURBO → spin down
	{
		MinigunBeginSpinDown( this );
	}
}

void CMinigun::ItemPostFrame( void )
{
	// ---- Temperature update (every MG_TEMP_THINK seconds) ----
	if ( m_pPlayer && gpGlobals->time >= m_flNextTempThink )
	{
		m_flNextTempThink = gpGlobals->time + MG_TEMP_THINK;

		if ( m_iFireState == MGSTATE_TURBO &&
		     ( m_pPlayer->pev->button & IN_ATTACK ) &&
		     m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
		{
			// Temperature rises while turbo-firing;
			// cooled variant warms up at half rate
			float rise = m_iCooled ? ( MG_TEMP_RISE * 0.5f ) : MG_TEMP_RISE;
			m_flWeaponTemp += rise;

			if ( m_flWeaponTemp >= MG_TEMP_MALFUNCTION )
			{
				// Overheat — lock out and spin down
				m_flWeaponTemp     = MG_TEMP_RESET;
				m_iFireState       = MGSTATE_MALFUNCTION;
				m_flMalfunctionEnd = gpGlobals->time + MG_MALFUNCTION_LOCKOUT;
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
					GMSND_MECHA_SPINDOWN, 1.0f, ATTN_NORM );
				SendWeaponAnim( MINIGUN_SPINDOWN );
				m_flNextPrimaryAttack = m_flNextSecondaryAttack =
					UTIL_WeaponTimeBase() + MG_MALFUNCTION_LOCKOUT + MG_SPINDOWN_TIME;
			}
		}
		else if ( m_flWeaponTemp > 0 )
		{
			m_flWeaponTemp -= MG_TEMP_FALL;
			if ( m_flWeaponTemp < 0 )
				m_flWeaponTemp = 0;
		}
	}

	// ---- State transitions ----
	if ( m_iFireState == MGSTATE_SPINUP && gpGlobals->time >= m_flSpinTime )
	{
		// Spin-up complete — enter turbo mode
		m_iFireState     = MGSTATE_TURBO;
		m_flNextAnimTime = 0;  // trigger immediate idleloop
		m_flNextPrimaryAttack  = UTIL_WeaponTimeBase();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
	else if ( m_iFireState == MGSTATE_SPINDOWN && gpGlobals->time >= m_flSpinTime )
	{
		// Spin-down complete — back to normal mode
		m_iFireState = MGSTATE_NORMAL;
		m_flNextPrimaryAttack  = UTIL_WeaponTimeBase();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
	else if ( m_iFireState == MGSTATE_MALFUNCTION && gpGlobals->time >= m_flMalfunctionEnd )
	{
		// Malfunction lockout expired — spin down to normal
		MinigunBeginSpinDown( this );
	}

	// ---- Looping animation in turbo mode ----
	if ( m_iFireState == MGSTATE_TURBO && gpGlobals->time >= m_flNextAnimTime )
	{
		BOOL bFiring = ( m_pPlayer->pev->button & IN_ATTACK ) != 0 &&
		               m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0;
		SendWeaponAnim( bFiring ? MINIGUN_FIRELOOP : MINIGUN_IDLELOOP );
		m_flNextAnimTime = gpGlobals->time + MG_ANIM_LOOP_TIME;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CMinigun::Holster( int skiplocal /* = 0 */ )
{
	// Force spin-down before holstering if active
	if ( m_iFireState != MGSTATE_NORMAL )
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
			GMSND_MECHA_SPINDOWN, 1.0f, ATTN_NORM );
		m_iFireState = MGSTATE_NORMAL;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + MG_HOLSTER_TIME;
	SendWeaponAnim( MINIGUN_HOLSTER );
}

void CMinigun::WeaponIdle( void )
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// Don't play idle animations while spinning
	if ( m_iFireState != MGSTATE_NORMAL )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		return;
	}

	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );

	if ( flRand <= 0.6f )
	{
		SendWeaponAnim( MINIGUN_IDLE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 31.0f / 15.0f;   // ~2.07 s
	}
	else
	{
		SendWeaponAnim( MINIGUN_IDLEINSPECT );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 53.0f / 20.0f;   // ~2.65 s
	}
}

// -----------------------------------------------------------------------
// cust_2MinigunCooled — pickup that gives the minigun with m_iCooled=TRUE.
// The cooled variant has half the temperature rise rate, allowing longer
// sustained turbo fire before malfunction.
// -----------------------------------------------------------------------
class CCust2MinigunCooled : public CMinigun
{
public:
	void Spawn( void )
	{
		CMinigun::Spawn();
		m_iCooled = TRUE;
	}
};
LINK_ENTITY_TO_CLASS( cust_2MinigunCooled, CCust2MinigunCooled );


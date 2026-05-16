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
// weapon_gcshotgun - Gunman Chronicles Shotgun
// Configurable spread mode (shotgun / riotgun / rifle) and
// shell count (1-4 shells per shot).
// Ported from MisterCalvin/SvenCoop-GC AngelScript scripts
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

LINK_ENTITY_TO_CLASS( weapon_gcshotgun, CGCShotgun );

enum gcshotgun_anim_e
{
	GCSHOTGUN_DRAW			= 0,
	GCSHOTGUN_IDLE,
	GCSHOTGUN_IDLE_INSPECT,
	GCSHOTGUN_SHOOT1,		// 1 shell
	GCSHOTGUN_SHOOT2,		// 2 shells
	GCSHOTGUN_SHOOT3,		// 3 shells
	GCSHOTGUN_SHOOT4,		// 4 shells
	GCSHOTGUN_CUSTOMIZE
};

enum gcshotgun_spreadmode_e
{
	GCSHOTGUN_MODE_SHOTGUN	= 0,  // standard spread
	GCSHOTGUN_MODE_RIOTGUN,       // wide spread
	GCSHOTGUN_MODE_RIFLE          // tight spread (accurate)
};

// Spread vectors per mode
// DM_SHOTGUN  = 10 deg H x 5 deg V
// DM_RIOTGUN  = 20 deg H x 5 deg V
// RIFLE       = 2 deg  H x 2 deg V
static const Vector s_vecSpreadShotgun   = Vector( 0.08716f, 0.04362f, 0.0f );
static const Vector s_vecSpreadRiotgun   = Vector( 0.17365f, 0.04362f, 0.0f );
static const Vector s_vecSpreadRifle     = Vector( 0.01745f, 0.01745f, 0.0f );

void CGCShotgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_GCSHOTGUN;
	SET_MODEL( ENT(pev), "models/w_shotgun.mdl" );

	m_iDefaultAmmo = GC_BUCKSHOT_GIVE;
	m_iSpreadMode  = GCSHOTGUN_MODE_SHOTGUN;
	m_iShellCount  = 2;		// Lua default: CurrentShellCost = 2
	m_iMenuAxis    = 1;
	m_flCockTime   = 0.0f;

	FallInit();
}

void CGCShotgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_shotgun.mdl" );
	PRECACHE_MODEL( "models/w_shotgun.mdl" );
	PRECACHE_MODEL( "models/p_shotgun.mdl" );

	PRECACHE_MODEL( "models/shotgunshell.mdl" );

	PRECACHE_SOUND( "gunmanchronicles/weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/shotgun_cock_heavy.wav" );

	// Fallback sounds in case Gunman assets are missing
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/sbarrel2.wav" );
}

int CGCShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName   = STRING(pev->classname);
	p->pszAmmo1  = "buckshot";
	p->iMaxAmmo1 = GC_BUCKSHOT_MAX_CARRY;
	p->pszAmmo2  = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip  = GCSHOTGUN_MAX_CLIP;
	p->iSlot     = 2;
	p->iPosition = 6;
	p->iFlags    = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_NOAUTOSWITCHEMPTY;
	p->iId       = m_iId = WEAPON_GCSHOTGUN;
	p->iWeight   = GCSHOTGUN_WEIGHT;

	return 1;
}

int CGCShotgun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGCShotgun::Deploy( )
{
	ShotgunReconfigure();
	return DefaultDeploy( "models/v_shotgun.mdl",
		"models/p_shotgun.mdl", GCSHOTGUN_DRAW, "shotgun" );
}

void CGCShotgun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	// GC shotgun has no explicit holster animation; fade out with draw
	SendWeaponAnim( GCSHOTGUN_IDLE, skiplocal );
}

void CGCShotgun::ShotgunReconfigure( void )
{
	// Clamp shell count to 1-4
	if ( m_iShellCount < 1 ) m_iShellCount = 1;
	if ( m_iShellCount > 4 ) m_iShellCount = 4;

	// Clamp spread mode
	if ( m_iSpreadMode < GCSHOTGUN_MODE_SHOTGUN ) m_iSpreadMode = GCSHOTGUN_MODE_SHOTGUN;
	if ( m_iSpreadMode > GCSHOTGUN_MODE_RIFLE )   m_iSpreadMode = GCSHOTGUN_MODE_RIFLE;
}

void CGCShotgun::PrimaryAttack( void )
{
	// don't fire when completely submerged
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	FireShotgun();
}

void CGCShotgun::FireShotgun( void )
{
	int iShells = m_iShellCount;

	// Reject the shot (dry-fire) if ammo is insufficient for the configured shell count.
	// This matches Lua: if Ammo1() < ammoCosts[CurrentShellCost] then DryFire() end
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iShells )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= iShells;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Play shoot animation (1-4 shells)
	int iAnim = GCSHOTGUN_SHOOT1 + ( iShells - 1 );
	SendWeaponAnim( iAnim );

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/sbarrel1.wav", 1.0, ATTN_NORM );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc    = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	// Select spread cone based on mode
	Vector vecSpread;
	switch ( m_iSpreadMode )
	{
		case GCSHOTGUN_MODE_RIOTGUN: vecSpread = s_vecSpreadRiotgun;  break;
		case GCSHOTGUN_MODE_RIFLE:   vecSpread = s_vecSpreadRifle;    break;
		default:                     vecSpread = s_vecSpreadShotgun;  break;
	}

	// 5 pellets per shell in one call (matches Lua: bullet.Num = 5 * CurrentShellCost)
	m_pPlayer->FireBulletsPlayer( 5 * iShells, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT,
		0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	// Schedule per-shot cock sound (Lua: gunman_shotgunCock, CustomizeSoundDelay = 0.8 s)
	m_flCockTime = gpGlobals->time + 0.8f;

	m_pPlayer->pev->punchangle.x = -5.0f * iShells;
}

void CGCShotgun::SecondaryAttack( void )
{
	// 2-axis customisation menu (mirrors Lua FireModes: ShellUsage and SpreadAdjust)
	// Advance the current axis value; when it wraps, move to the next axis.
	switch ( m_iMenuAxis )
	{
	case 1: // ShellUsage: 1-4
		m_iShellCount++;
		if ( m_iShellCount > 4 ) { m_iShellCount = 1; m_iMenuAxis = 2; }
		break;
	case 2: // SpreadAdjust: 0=shotgun, 1=riotgun, 2=rifle (wraps back to 0)
		m_iSpreadMode = ( m_iSpreadMode + 1 ) % 3;
		if ( m_iSpreadMode == 0 ) m_iMenuAxis = 1;  // wrapped back to shotgun → return to axis 1
		break;
	default:
		m_iMenuAxis = 1;
		break;
	}

	ShotgunReconfigure();

	SendWeaponAnim( GCSHOTGUN_CUSTOMIZE );
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
		"gunmanchronicles/weapons/shotgun_cock_heavy.wav", 0.8f, ATTN_NORM );

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle      = UTIL_WeaponTimeBase() + 1.0f;
}

void CGCShotgun::WeaponIdle( void )
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	int iAnim;
	float flNextIdle;
	if ( flRand <= 0.8f )
	{
		iAnim      = GCSHOTGUN_IDLE;
		flNextIdle = 30.0f / 15.0f;
	}
	else
	{
		iAnim      = GCSHOTGUN_IDLE_INSPECT;
		flNextIdle = 30.0f / 15.0f;
	}

	SendWeaponAnim( iAnim );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flNextIdle;
}


void CGCShotgun::ItemPostFrame( void )
{
	// Per-shot cock sound — polled every frame so it fires even while attack buttons are held
	if ( m_flCockTime > 0.0f && gpGlobals->time >= m_flCockTime )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/shotgun_cock_heavy.wav", 0.8f, ATTN_NORM );
		m_flCockTime = 0.0f;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

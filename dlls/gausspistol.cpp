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
// weapon_gausspistol — Gunman Chronicles Gauss Pistol
//
// Four fire modes ported from:
//   game/lua/weapons/gunman_weapon_gausspistol/shared.lua
//   game/lua/gunman_data.lua  (damage / ammo-usage tables)
//
//   Mode 1 — Pulse  : instant hitscan, 25 dmg, 1 ammo, 0.2 s cadence
//   Mode 2 — Charge : projectile 1500 u/s, 100 dmg, 10 ammo, 2.1 s cadence
//   Mode 3 — Rapid  : projectile 2500 u/s ±2° cone, 40 dmg, 1 ammo, 0.1 s cadence
//   Mode 4 — Sniper : hold IN_ATTACK to zoom (FOV 90→20), release to fire
//                     hitscan 40 dmg (unzoomed) / 100 dmg (zoomed), 10/20 ammo
//
// SecondaryAttack cycles through modes (1→2→3→4→1) and plays "customize" anim.
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

LINK_ENTITY_TO_CLASS( weapon_gausspistol, CGaussPistol );

// -----------------------------------------------------------------------
// Damage / ammo-usage tables  (from game/lua/gunman_data.lua)
// -----------------------------------------------------------------------
#define GP_DMG_PULSE			25		// Pulse hitscan damage
#define GP_DMG_RAPID_TOUCH		13		// Rapid projectile direct-hit damage
#define GP_DMG_RAPID_BLAST		26		// Rapid projectile blast damage
#define GP_DMG_RAPID_RADIUS		16.0f	// Rapid blast radius (units)
#define GP_DMG_CHARGE_TOUCH		50		// Charge projectile direct-hit damage
#define GP_DMG_CHARGE_BLAST		100		// Charge projectile blast damage
#define GP_DMG_CHARGE_RADIUS	96.0f	// Charge blast radius (units)
#define GP_DMG_SNIPER_QUICK		40		// Sniper damage without full zoom
#define GP_DMG_SNIPER_FULL		100		// Sniper damage with full zoom

#define GP_AMMO_PULSE			1		// ammo per Pulse shot
#define GP_AMMO_RAPID			1		// ammo per Rapid shot
#define GP_AMMO_CHARGE			10		// ammo per Charge shot
#define GP_AMMO_SNIPER_QUICK	10		// ammo for unzoomed sniper shot
#define GP_AMMO_SNIPER_FULL		20		// ammo for fully-zoomed sniper shot

#define GP_PROJ_SPEED_RAPID		2500.0f	// Rapid projectile speed (u/s)
#define GP_PROJ_SPEED_CHARGE	1500.0f	// Charge projectile speed (u/s)
#define GP_PROJ_LIFE			5.0f	// Auto-expire time (seconds)

#define GP_SNIPER_FOV_PARTIAL	80		// FOV after first zoom press
#define GP_SNIPER_FOV_FULL		20		// FOV after second zoom step
#define GP_SNIPER_ZOOM_DELAY	0.5f	// Seconds until full zoom activates

// -----------------------------------------------------------------------
// Animation indices  (model: models/v_guasspistol.mdl)
// -----------------------------------------------------------------------
enum gausspistol_e
{
	GAUSSPISTOL_IDLE = 0,
	GAUSSPISTOL_IDLERESTLESS,
	GAUSSPISTOL_DRAW,
	GAUSSPISTOL_HOLSTER,
	GAUSSPISTOL_SINGLESHOT,
	GAUSSPISTOL_CHARGEFIRE,
	GAUSSPISTOL_RAPIDFIRE,
	GAUSSPISTOL_SNIPERIDLE,
	GAUSSPISTOL_SNIPERSHOOT,
	GAUSSPISTOL_SNIPERDRAW,
	GAUSSPISTOL_SNIPERHOLSTER,
	GAUSSPISTOL_CUSTOMIZE,
};

// -----------------------------------------------------------------------
// CGaussPistolProjectile — used by both Rapid and Charge fire modes
// -----------------------------------------------------------------------
class CGaussPistolProjectile : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT ProjectileTouch( CBaseEntity *pOther );
	void EXPORT ProjectileThink( void );

	static CGaussPistolProjectile *CreateProjectile(
		Vector vecOrigin, Vector vecDir, float flSpeed,
		int iTouchDmg, int iBlastDmg, float flBlastRadius,
		CBaseEntity *pOwner );

	virtual int  Save( CSave &save );
	virtual int  Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int		m_iTouchDmg;
	int		m_iBlastDmg;
	float	m_flBlastRadius;
	float	m_flExpireTime;

private:
	int		m_iBeamSprite;
};

TYPEDESCRIPTION	CGaussPistolProjectile::m_SaveData[] =
{
	DEFINE_FIELD( CGaussPistolProjectile, m_iTouchDmg,     FIELD_INTEGER ),
	DEFINE_FIELD( CGaussPistolProjectile, m_iBlastDmg,     FIELD_INTEGER ),
	DEFINE_FIELD( CGaussPistolProjectile, m_flBlastRadius, FIELD_FLOAT   ),
	DEFINE_FIELD( CGaussPistolProjectile, m_flExpireTime,  FIELD_TIME    ),
};
IMPLEMENT_SAVERESTORE( CGaussPistolProjectile, CBaseEntity );
LINK_ENTITY_TO_CLASS( gausspistol_proj, CGaussPistolProjectile );

//=========================================================
void CGaussPistolProjectile::Precache( void )
{
	// Reuse the built-in lightning sprite for the beam trail.
	m_iBeamSprite = PRECACHE_MODEL( "sprites/laserbeam.spr" );
}

//=========================================================
void CGaussPistolProjectile::Spawn( void )
{
	Precache();
	pev->movetype  = MOVETYPE_FLY;
	pev->solid     = SOLID_BBOX;
	pev->classname = MAKE_STRING( "gausspistol_proj" );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt  = 200;

	SET_MODEL( ENT(pev), "models/dmlcluster.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CGaussPistolProjectile::ProjectileTouch );
	SetThink( &CGaussPistolProjectile::ProjectileThink );
	pev->nextthink = gpGlobals->time + 0.05f;

	// Attach a glowing sprite trail.
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE ( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex()    );
		WRITE_SHORT( m_iBeamSprite );
		WRITE_BYTE ( 6  );		// trail life (0.6 s)
		WRITE_BYTE ( 3  );		// width
		WRITE_BYTE ( 160 );		// r
		WRITE_BYTE ( 255 );		// g
		WRITE_BYTE ( 160 );		// b
		WRITE_BYTE ( 220 );		// brightness
	MESSAGE_END();
}

//=========================================================
CGaussPistolProjectile *CGaussPistolProjectile::CreateProjectile(
	Vector vecOrigin, Vector vecDir, float flSpeed,
	int iTouchDmg, int iBlastDmg, float flBlastRadius,
	CBaseEntity *pOwner )
{
	CGaussPistolProjectile *pProj = GetClassPtr( (CGaussPistolProjectile *)NULL );

	UTIL_SetOrigin( pProj->pev, vecOrigin );
	pProj->pev->angles   = UTIL_VecToAngles( vecDir );
	pProj->pev->velocity = vecDir * flSpeed;
	pProj->pev->owner    = pOwner->edict();

	pProj->m_iTouchDmg     = iTouchDmg;
	pProj->m_iBlastDmg     = iBlastDmg;
	pProj->m_flBlastRadius = flBlastRadius;
	pProj->m_flExpireTime  = gpGlobals->time + GP_PROJ_LIFE;

	pProj->Spawn();
	return pProj;
}

//=========================================================
void CGaussPistolProjectile::ProjectileThink( void )
{
	// Expire after 5 seconds (mirrors Lua's PlasmaLife).
	if ( gpGlobals->time >= m_flExpireTime || pev->waterlevel > 0 )
	{
		::RadiusDamage( pev->origin, pev, pev,
			(float)m_iBlastDmg, m_flBlastRadius, CLASS_NONE,
			DMG_ENERGYBEAM | DMG_BLAST );
		UTIL_Remove( this );
		return;
	}
	pev->nextthink = gpGlobals->time + 0.05f;
}

//=========================================================
void CGaussPistolProjectile::ProjectileTouch( CBaseEntity *pOther )
{
	if ( pOther && pOther->edict() == pev->owner )
		return;

	if ( pOther )
		pOther->TakeDamage( pev, pev, (float)m_iTouchDmg, DMG_ENERGYBEAM );

	::RadiusDamage( pev->origin, pev, pev,
		(float)m_iBlastDmg, m_flBlastRadius, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_BLAST );

	// Electric spark at impact.
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE ( TE_SPARKS      );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	UTIL_Remove( this );
}


// -----------------------------------------------------------------------
// CGaussPistol implementation
// -----------------------------------------------------------------------

//=========================================================
void CGaussPistol::Spawn( void )
{
	Precache();
	m_iId         = WEAPON_GAUSSPISTOL;
	m_iFireMode   = FIREMODE_PULSE;
	m_bSniperMode = FALSE;
	m_iSniperZoom = 0;

	SET_MODEL( ENT(pev), "models/w_gauss.mdl" );
	m_iDefaultAmmo = GAUSSPISTOL_DEFAULT_GIVE;
	FallInit();
}

//=========================================================
void CGaussPistol::Precache( void )
{
	PRECACHE_MODEL( "models/v_guasspistol.mdl" );
	PRECACHE_MODEL( "models/w_gauss.mdl"        );
	PRECACHE_MODEL( "models/p_357.mdl"          );	// placeholder p-model
	PRECACHE_MODEL( "models/dmlcluster.mdl"     );	// projectile model
	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );

	// Fire sounds (Lua WeaponChannel entries).
	PRECACHE_SOUND( "gunmanchronicles/weapons/gauss_fire4.wav"   );	// Pulse
	PRECACHE_SOUND( "gunmanchronicles/weapons/gauss_fire2.wav"   );	// Charge
	PRECACHE_SOUND( "gunmanchronicles/weapons/gauss_fire1.wav"   );	// Rapid
	PRECACHE_SOUND( "gunmanchronicles/weapons/gsnipe_zoom.wav"   );	// Sniper zoom-in
	PRECACHE_SOUND( "gunmanchronicles/weapons/sniperunzoom.wav"  );	// Sniper quick fire
	PRECACHE_SOUND( "gunmanchronicles/weapons/sniperzoom.wav"    );	// Sniper full-zoom fire

	// Mode-switch / customize sound (closest available match).
	PRECACHE_SOUND( "gunmanchronicles/weapons/DryFire.wav"       );

	// Pickup.
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );

	// Trail sprite (built-in fallback).
	PRECACHE_MODEL( "sprites/laserbeam.spr" );

	m_usGaussPistol = PRECACHE_EVENT( 1, "events/glock1.sc" );
}

//=========================================================
int CGaussPistol::GetItemInfo( ItemInfo *p )
{
	p->pszName   = STRING( pev->classname );
	p->pszAmmo1  = "gausspistol_ammo";
	p->iMaxAmmo1 = GAUSS_PISTOL_MAX_CARRY;
	p->pszAmmo2  = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip  = WEAPON_NOCLIP;	// no physical clip — all ammo is pooled
	p->iSlot     = 1;
	p->iPosition = 0;
	p->iFlags    = 0;
	p->iId       = m_iId = WEAPON_GAUSSPISTOL;
	p->iWeight   = GAUSSPISTOL_WEIGHT;
	return 1;
}

//=========================================================
int CGaussPistol::AddToPlayer( CBasePlayer *pPlayer )
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

//=========================================================
BOOL CGaussPistol::Deploy( void )
{
	m_bSniperMode = FALSE;
	m_iSniperZoom = 0;
	// Ensure FOV is default when drawing the weapon.
	m_pPlayer->m_iFOV   = 0;
	m_pPlayer->pev->fov = 0;

	return DefaultDeploy( "models/v_guasspistol.mdl", "models/p_357.mdl",
		GAUSSPISTOL_DRAW, "onehanded" );
}

//=========================================================
void CGaussPistol::Holster( int skiplocal )
{
	if ( m_bSniperMode )
		SniperExitMode();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( GAUSSPISTOL_HOLSTER );
}

// -----------------------------------------------------------------------
// Fire mode management
// -----------------------------------------------------------------------

//=========================================================
// SetFireMode — change mode and play the "customize" animation/sound.
//=========================================================
void CGaussPistol::SetFireMode( int iMode )
{
	if ( m_bSniperMode && iMode != FIREMODE_SNIPER )
		SniperExitMode();

	m_iFireMode = iMode;

	SendWeaponAnim( GAUSSPISTOL_CUSTOMIZE );
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
		"gunmanchronicles/weapons/DryFire.wav", 0.8f, ATTN_NORM );

	m_flNextPrimaryAttack   = UTIL_WeaponTimeBase() + 1.0f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle      = UTIL_WeaponTimeBase() + 1.5f;
}

//=========================================================
// SecondaryAttack — cycles Pulse → Charge → Rapid → Sniper → Pulse.
//=========================================================
void CGaussPistol::SecondaryAttack( void )
{
	int iNext = ( m_iFireMode % FIREMODE_MAX ) + 1;
	SetFireMode( iNext );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

// -----------------------------------------------------------------------
// Mode 1 — Pulse
// -----------------------------------------------------------------------
void CGaussPistol::FireModePulse( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < GP_AMMO_PULSE )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects    = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc    = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming,
		VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_BUCKSHOT,
		0, GP_DMG_PULSE, m_pPlayer->pev, m_pPlayer->random_seed );

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON,
		"gunmanchronicles/weapons/gauss_fire4.wav",
		1.0f, ATTN_NORM, 0, RANDOM_LONG( 96, 112 ) );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= GP_AMMO_PULSE;
	m_pPlayer->pev->punchangle.x = -1.0f;

	SendWeaponAnim( GAUSSPISTOL_SINGLESHOT );
	m_flNextPrimaryAttack  = UTIL_WeaponTimeBase() + 0.2f;
	m_flTimeWeaponIdle     = UTIL_WeaponTimeBase() + 2.0f;
}

// -----------------------------------------------------------------------
// Mode 3 — Rapid
// -----------------------------------------------------------------------
void CGaussPistol::FireModeRapid( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < GP_AMMO_RAPID )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects    = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition();

	// ±2° random cone (matches Lua: Angle(math.Rand(-2, 2), math.Rand(-2, 2), 0)).
	const float flCone = 0.035f;	// tan(2°) ≈ 0.035
	Vector vecDir = gpGlobals->v_forward
		+ gpGlobals->v_right * RANDOM_FLOAT( -flCone, flCone )
		+ gpGlobals->v_up    * RANDOM_FLOAT( -flCone, flCone );
	vecDir = vecDir.Normalize();

	CGaussPistolProjectile::CreateProjectile(
		vecSrc, vecDir, GP_PROJ_SPEED_RAPID,
		GP_DMG_RAPID_TOUCH, GP_DMG_RAPID_BLAST, GP_DMG_RAPID_RADIUS,
		m_pPlayer );

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON,
		"gunmanchronicles/weapons/gauss_fire1.wav",
		1.0f, ATTN_NORM, 0, RANDOM_LONG( 100, 106 ) );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= GP_AMMO_RAPID;
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -0.2f, -0.1f );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -0.2f,  0.2f );

	SendWeaponAnim( GAUSSPISTOL_RAPIDFIRE );
	m_flNextPrimaryAttack  = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle     = UTIL_WeaponTimeBase() + 1.0f;
}

// -----------------------------------------------------------------------
// Mode 2 — Charge
// -----------------------------------------------------------------------
void CGaussPistol::FireModeCharge( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < GP_AMMO_CHARGE )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = BRIGHT_GUN_FLASH;
	m_pPlayer->pev->effects    = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES ).Normalize();

	CGaussPistolProjectile::CreateProjectile(
		vecSrc, vecDir, GP_PROJ_SPEED_CHARGE,
		GP_DMG_CHARGE_TOUCH, GP_DMG_CHARGE_BLAST, GP_DMG_CHARGE_RADIUS,
		m_pPlayer );

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON,
		"gunmanchronicles/weapons/gauss_fire2.wav", 1.0f, ATTN_NORM );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= GP_AMMO_CHARGE;
	m_pPlayer->pev->punchangle.x = -4.0f;
	m_pPlayer->pev->punchangle.y = -1.0f;

	SendWeaponAnim( GAUSSPISTOL_CHARGEFIRE );
	m_flNextPrimaryAttack  = UTIL_WeaponTimeBase() + 2.1f;
	m_flTimeWeaponIdle     = UTIL_WeaponTimeBase() + 2.5f;
}

// -----------------------------------------------------------------------
// Mode 4 — Sniper helpers
// -----------------------------------------------------------------------

//=========================================================
void CGaussPistol::SniperEnterMode( void )
{
	m_bSniperMode = TRUE;
	m_iSniperZoom = 0;
	SendWeaponAnim( GAUSSPISTOL_SNIPERDRAW );
}

//=========================================================
void CGaussPistol::SniperExitMode( void )
{
	m_bSniperMode        = FALSE;
	m_iSniperZoom        = 0;
	m_pPlayer->m_iFOV   = 0;
	m_pPlayer->pev->fov = 0;
	SendWeaponAnim( GAUSSPISTOL_SNIPERHOLSTER );
}

// -----------------------------------------------------------------------
// Mode 4 — Sniper fire (called when attack is released with zoom active)
// -----------------------------------------------------------------------
void CGaussPistol::FireModeSniper( void )
{
	int         iAmmoCost = ( m_iSniperZoom >= 2 ) ? GP_AMMO_SNIPER_FULL  : GP_AMMO_SNIPER_QUICK;
	int         iDmg      = ( m_iSniperZoom >= 2 ) ? GP_DMG_SNIPER_FULL   : GP_DMG_SNIPER_QUICK;
	const char *pszSnd    = ( m_iSniperZoom >= 2 )
		? "gunmanchronicles/weapons/sniperzoom.wav"
		: "gunmanchronicles/weapons/sniperunzoom.wav";

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iAmmoCost )
	{
		PlayEmptySound();
		// Abort zoom on dry fire.
		m_pPlayer->m_iFOV   = 0;
		m_pPlayer->pev->fov = 0;
		m_iSniperZoom       = 0;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects    = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc    = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );

	m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming,
		VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_BUCKSHOT,
		0, iDmg, m_pPlayer->pev, m_pPlayer->random_seed );

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, pszSnd, 1.0f, ATTN_NORM );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= iAmmoCost;
	m_pPlayer->pev->punchangle.x = -1.0f;
	m_pPlayer->pev->punchangle.y = -1.0f;

	// Return to default FOV after firing.
	m_pPlayer->m_iFOV   = 0;
	m_pPlayer->pev->fov = 0;
	m_iSniperZoom       = 0;
	m_flBeamEndTime     = gpGlobals->time + 1.5f;

	SendWeaponAnim( GAUSSPISTOL_SNIPERSHOOT );
	m_flNextPrimaryAttack  = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle     = UTIL_WeaponTimeBase() + 2.0f;
}

// -----------------------------------------------------------------------
// PrimaryAttack — dispatches to the active fire mode
// -----------------------------------------------------------------------
void CGaussPistol::PrimaryAttack( void )
{
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	switch ( m_iFireMode )
	{
	case FIREMODE_PULSE:
		FireModePulse();
		break;

	case FIREMODE_RAPID:
		FireModeRapid();
		break;

	case FIREMODE_CHARGE:
		FireModeCharge();
		break;

	case FIREMODE_SNIPER:
		// First press enters sniper scope, subsequent presses advance zoom,
		// and release-while-zoomed fires (handled below via WeaponIdle).
		if ( !m_bSniperMode )
		{
			SniperEnterMode();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5f;
		}
		else if ( m_iSniperZoom == 0 )
		{
			// Begin zoom.
			m_iSniperZoom      = 1;
			m_flSniperZoomTime = gpGlobals->time;
			m_pPlayer->m_iFOV  = GP_SNIPER_FOV_PARTIAL;
			m_pPlayer->pev->fov = GP_SNIPER_FOV_PARTIAL;
			EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
				"gunmanchronicles/weapons/gsnipe_zoom.wav", 0.8f, ATTN_NORM );
			SendWeaponAnim( GAUSSPISTOL_SNIPERIDLE );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + GP_SNIPER_ZOOM_DELAY;
		}
		else if ( m_iSniperZoom == 1 &&
			( gpGlobals->time - m_flSniperZoomTime ) >= GP_SNIPER_ZOOM_DELAY )
		{
			// Advance to full zoom.
			m_iSniperZoom       = 2;
			m_pPlayer->m_iFOV   = GP_SNIPER_FOV_FULL;
			m_pPlayer->pev->fov = GP_SNIPER_FOV_FULL;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		}
		else if ( m_iSniperZoom >= 2 )
		{
			// Fire at current zoom level.
			FireModeSniper();
		}
		break;

	default:
		FireModePulse();
		break;
	}

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
}

// -----------------------------------------------------------------------
// Reload — Gauss Pistol uses WEAPON_NOCLIP (no physical clip to reload).
// -----------------------------------------------------------------------
void CGaussPistol::Reload( void )
{
	// No physical clip; nothing to reload.
}

// -----------------------------------------------------------------------
// WeaponIdle
// -----------------------------------------------------------------------
void CGaussPistol::WeaponIdle( void )
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	if ( m_bSniperMode )
	{
		iAnim = GAUSSPISTOL_SNIPERIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	}
	else
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
		if ( flRand < 0.5f )
		{
			iAnim = GAUSSPISTOL_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
		}
		else
		{
			iAnim = GAUSSPISTOL_IDLERESTLESS;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
		}
	}
	SendWeaponAnim( iAnim, 1 );
}

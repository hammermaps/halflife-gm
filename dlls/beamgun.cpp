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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( weapon_beamgun, CBeamGun );

enum beamgun_e
{
	BEAMGUN_IDLE = 0,
	BEAMGUN_FIDGET,
	BEAMGUN_SHOOT,
	BEAMGUN_RELOAD,
	BEAMGUN_DRAW,
	BEAMGUN_HOLSTER
};

void CBeamGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BEAMGUN;
	SET_MODEL(ENT(pev), "models/w_egon.mdl"); // Using existing model as placeholder

	m_iDefaultAmmo = 100;
	m_flAmmoUseTime = 0;

	FallInit();// get ready to fall down.
}


void CBeamGun::Precache( void )
{
	PRECACHE_MODEL("models/v_egon.mdl"); // Using existing models as placeholders
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");

	PRECACHE_SOUND ("weapons/egon_windup2.wav");
	PRECACHE_SOUND ("weapons/egon_run3.wav");
	PRECACHE_SOUND ("weapons/egon_off1.wav");

	m_usBeamGun = PRECACHE_EVENT( 1, "events/egon.sc" );
}

int CBeamGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "beamgun_ammo";
	p->iMaxAmmo1 = BEAMGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BEAMGUN;
	p->iWeight = BEAMGUN_WEIGHT;

	return 1;
}

int CBeamGun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CBeamGun::Deploy( )
{
	m_flAmmoUseTime = 0;
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", BEAMGUN_DRAW, "egon" );
}

void CBeamGun::SecondaryAttack( void )
{
	// Alternate fire mode could be implemented here
}

void CBeamGun::PrimaryAttack( void )
{
	// Continuous beam weapon - consumes 1 ammo per 0.1 seconds
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.25;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// Trace the beam
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 2048, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

	// Apply damage along the beam
	if (tr.flFraction != 1.0)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if (pEntity)
		{
			ClearMultiDamage();
			pEntity->TraceAttack( m_pPlayer->pev, 14, vecAiming, &tr, DMG_ENERGYBEAM );
			ApplyMultiDamage( pev, m_pPlayer->pev );
		}
	}

	// Consume ammo at timed intervals
	if ( gpGlobals->time >= m_flAmmoUseTime )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_flAmmoUseTime = gpGlobals->time + 0.1;
	}

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_run3.wav", 0.6, ATTN_NORM );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_off1.wav", 0.8, ATTN_NORM );
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x = -1.0;
}


void CBeamGun::Reload( void )
{
	// No reload - uses WEAPON_NOCLIP
}


void CBeamGun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3 + 0 * 0.75)
	{
		iAnim = BEAMGUN_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
	}
	else if (flRand <= 0.6 + 0 * 0.875)
	{
		iAnim = BEAMGUN_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
	}
	else
	{
		iAnim = BEAMGUN_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	SendWeaponAnim( iAnim, 1 );
}


void CBeamGun::Holster( int skiplocal /* = 0 */ )
{
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, "weapons/egon_off1.wav", 0.8, ATTN_NORM );

	m_flAmmoUseTime = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( BEAMGUN_HOLSTER );
}

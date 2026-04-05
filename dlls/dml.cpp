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

LINK_ENTITY_TO_CLASS( weapon_dml, CDML );

enum dml_e
{
	DML_IDLE = 0,
	DML_FIDGET,
	DML_SHOOT,
	DML_RELOAD,
	DML_DRAW,
	DML_HOLSTER
};

void CDML::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DML;
	SET_MODEL(ENT(pev), "models/w_rpg.mdl"); // Using existing model as placeholder

	m_iDefaultAmmo = 4;

	FallInit();// get ready to fall down.
}


void CDML::Precache( void )
{
	PRECACHE_MODEL("models/v_rpg.mdl"); // Using existing models as placeholders
	PRECACHE_MODEL("models/w_rpg.mdl");
	PRECACHE_MODEL("models/p_rpg.mdl");

	PRECACHE_MODEL("models/rpgrocket.mdl"); // rocket model

	PRECACHE_SOUND ("weapons/rocketfire1.wav");
	PRECACHE_SOUND ("weapons/glauncher.wav");

	m_usDML = PRECACHE_EVENT( 1, "events/rpg.sc" );
}

int CDML::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "dml_ammo";
	p->iMaxAmmo1 = DML_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 4;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DML;
	p->iWeight = DML_WEIGHT;

	return 1;
}

int CDML::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CDML::Deploy( )
{
	return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", DML_DRAW, "rpg" );
}

void CDML::SecondaryAttack( void )
{
	// Alternate fire mode could be implemented here
}

void CDML::PrimaryAttack( void )
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

	// Simplified rocket attack: trace and explode at impact point
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + gpGlobals->v_forward * 4096, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 1.0, ATTN_NORM );

	// Create explosion at impact point
	if (tr.flFraction != 1.0)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// Apply damage at the impact point
		::RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev, 100, 150, CLASS_NONE, DMG_BLAST );
	}

	SendWeaponAnim( DML_SHOOT );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.8;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x = -5.0;
}


void CDML::Reload( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	int iResult;

	iResult = DefaultReload( 4, DML_RELOAD, 3.0 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}


void CDML::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3 + 0 * 0.75)
	{
		iAnim = DML_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
	}
	else if (flRand <= 0.6 + 0 * 0.875)
	{
		iAnim = DML_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
	}
	else
	{
		iAnim = DML_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	SendWeaponAnim( iAnim, 1 );
}


void CDML::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( DML_HOLSTER );
}

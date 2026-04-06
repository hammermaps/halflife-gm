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

LINK_ENTITY_TO_CLASS( weapon_chemicalgun, CChemicalGun );

enum chemicalgun_e
{
	CHEMICALGUN_IDLE = 0,
	CHEMICALGUN_FIDGET,
	CHEMICALGUN_SHOOT,
	CHEMICALGUN_RELOAD,
	CHEMICALGUN_DRAW,
	CHEMICALGUN_HOLSTER
};

void CChemicalGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CHEMICALGUN;
	SET_MODEL(ENT(pev), "models/gunmanchronicles/w_chemicalgun.mdl");

	m_iDefaultAmmo = CHEMICALGUN_DEFAULT_GIVE;

	FallInit();
}


void CChemicalGun::Precache( void )
{
	PRECACHE_MODEL("models/gunmanchronicles/v_chemicalgun.mdl");
	PRECACHE_MODEL("models/gunmanchronicles/w_chemicalgun.mdl");
	PRECACHE_MODEL("models/gunmanchronicles/p_chemicalgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");

	PRECACHE_SOUND("gunmanchronicles/weapons/ChemicalGun_fire1.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/ChemicalGun_reload.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/DryFire.wav");

	m_usChemicalGun = PRECACHE_EVENT( 1, "events/mp51.sc" );
}

int CChemicalGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "chemical_ammo";
	p->iMaxAmmo1 = CHEMICAL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 30;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CHEMICALGUN;
	p->iWeight = CHEMICALGUN_WEIGHT;

	return 1;
}

int CChemicalGun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CChemicalGun::Deploy( )
{
	return DefaultDeploy( "models/gunmanchronicles/v_chemicalgun.mdl", "models/gunmanchronicles/p_chemicalgun.mdl", CHEMICALGUN_DRAW, "mp5" );
}

void CChemicalGun::SecondaryAttack( void )
{
	// Alternate fire mode could be implemented here
}

void CChemicalGun::PrimaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL ); 

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/ChemicalGun_fire1.wav", 1.0, ATTN_NORM, 0, 100 );

	SendWeaponAnim( CHEMICALGUN_SHOOT );

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x = -2.0;
}


void CChemicalGun::Reload( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	int iResult;

	iResult = DefaultReload( 30, CHEMICALGUN_RELOAD, 1.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}


void CChemicalGun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3 + 0 * 0.75)
	{
		iAnim = CHEMICALGUN_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
	}
	else if (flRand <= 0.6 + 0 * 0.875)
	{
		iAnim = CHEMICALGUN_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
	}
	else
	{
		iAnim = CHEMICALGUN_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	SendWeaponAnim( iAnim, 1 );
}


void CChemicalGun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( CHEMICALGUN_HOLSTER );
}

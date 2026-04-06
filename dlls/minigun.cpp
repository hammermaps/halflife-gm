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

LINK_ENTITY_TO_CLASS( weapon_minigun, CMinigun );

enum minigun_e
{
	MINIGUN_IDLE = 0,
	MINIGUN_FIDGET,
	MINIGUN_SHOOT,
	MINIGUN_RELOAD,
	MINIGUN_DRAW,
	MINIGUN_HOLSTER
};

void CMinigun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_MINIGUN;
	SET_MODEL(ENT(pev), "models/gunmanchronicles/w_minigun.mdl");

	m_iDefaultAmmo = MINIGUN_DEFAULT_GIVE;
	m_bSpinning = FALSE;

	FallInit();
}


void CMinigun::Precache( void )
{
	PRECACHE_MODEL("models/gunmanchronicles/v_minigun.mdl");
	PRECACHE_MODEL("models/gunmanchronicles/w_minigun.mdl");
	PRECACHE_MODEL("models/gunmanchronicles/p_minigun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");

	PRECACHE_SOUND("gunmanchronicles/weapons/Minigun_fire1.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/Minigun_spinup.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/Minigun_spindown.wav");

	m_usMinigun = PRECACHE_EVENT( 1, "events/mp51.sc" );
}

int CMinigun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "minigun_ammo";
	p->iMaxAmmo1 = MINIGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 100;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MINIGUN;
	p->iWeight = MINIGUN_WEIGHT;

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
	m_bSpinning = FALSE;
	return DefaultDeploy( "models/gunmanchronicles/v_minigun.mdl", "models/gunmanchronicles/p_minigun.mdl", MINIGUN_DRAW, "mp5" );
}

void CMinigun::SecondaryAttack( void )
{
	// Toggle spin-up state
	m_bSpinning = !m_bSpinning;

	if (m_bSpinning)
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/Minigun_spinup.wav", 1.0, ATTN_NORM );
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CMinigun::PrimaryAttack( void )
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

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip -= 2;
	if (m_iClip < 0)
		m_iClip = 0;

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

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/Minigun_fire1.wav", 1.0, ATTN_NORM, 0, 100 );

	SendWeaponAnim( MINIGUN_SHOOT );

	Vector vecDir;
	// 2 bullets per shot at high fire rate
	vecDir = m_pPlayer->FireBulletsPlayer( 2, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.05;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x = -1.0;
}


void CMinigun::Reload( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	int iResult;

	m_bSpinning = FALSE;

	iResult = DefaultReload( 100, MINIGUN_RELOAD, 4.0 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}


void CMinigun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3 + 0 * 0.75)
	{
		iAnim = MINIGUN_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
	}
	else if (flRand <= 0.6 + 0 * 0.875)
	{
		iAnim = MINIGUN_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
	}
	else
	{
		iAnim = MINIGUN_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	SendWeaponAnim( iAnim, 1 );
}


void CMinigun::Holster( int skiplocal /* = 0 */ )
{
	m_bSpinning = FALSE;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( MINIGUN_HOLSTER );
}

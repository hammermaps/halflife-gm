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
// weapon_aicore - Gunman Chronicles "AI Core" / Wrench
//
// This is a stub implementation.  It registers the entity, sets the
// correct uploaded models and delivers a single 100-hp melee hit so
// the weapon is at least minimally functional when picked up.  See
// docs/GUNMAN_LUA_PORT_PLAN.md (§2.8) for the full Lua-driven
// behaviour that still needs to be ported.
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

LINK_ENTITY_TO_CLASS( weapon_aicore, CAICore );

// AICore animation enum (placeholders; v-model has more sequences
// — see game/lua/weapons/gunman_weapon_aicore for the full list).
enum aicore_e
{
	AICORE_IDLE = 0,
	AICORE_DRAW,
	AICORE_HOLSTER,
	AICORE_ATTACK
};

void CAICore::Spawn( void )
{
	Precache();
	m_iId = WEAPON_AICORE;
	// World model — uppercase 'W' matches the on-disk filename.
	SET_MODEL( ENT( pev ), "models/W_aicore.mdl" );

	m_iDefaultAmmo = 0;

	FallInit();
}

void CAICore::Precache( void )
{
	// View / world / player models — see docs/GUNMAN_MODELS_MAPPING.md.
	PRECACHE_MODEL( "models/V_aicore.mdl" );
	PRECACHE_MODEL( "models/W_aicore.mdl" );
	// No dedicated p_aicore.mdl in the upload; reuse the generic
	// crowbar p-model so third-person rendering does not crash.
	PRECACHE_MODEL( "models/p_crowbar.mdl" );

	// Generic activation sounds; the real GC sounds
	// ("aicore_activate.wav", "aicore_deactivate.wav",
	// "aicore_activated.wav") still need to be added.
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );
}

int CAICore::GetItemInfo( ItemInfo *p )
{
	p->pszName    = STRING( pev->classname );
	p->pszAmmo1   = NULL;
	p->iMaxAmmo1  = -1;
	p->pszAmmo2   = NULL;
	p->iMaxAmmo2  = -1;
	p->iMaxClip   = WEAPON_NOCLIP;
	p->iSlot      = 0;
	p->iPosition  = 2;
	p->iFlags     = 0;
	p->iId        = m_iId = WEAPON_AICORE;
	p->iWeight    = AICORE_WEIGHT;

	return 1;
}

int CAICore::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAICore::Deploy( void )
{
	return DefaultDeploy( "models/V_aicore.mdl", "models/p_crowbar.mdl",
		AICORE_DRAW, "crowbar" );
}

void CAICore::Holster( int /*skiplocal*/ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( AICORE_HOLSTER );
}

void CAICore::PrimaryAttack( void )
{
	// Simple melee swing: 100 hp direct hit within 64 units.
	// Matches the Lua "AICore.Bullet = 100" damage value.
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecSrc  = m_pPlayer->GetGunPosition();
	Vector vecEnd  = vecSrc + gpGlobals->v_forward * 64;

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters,
		ENT( m_pPlayer->pev ), &tr );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	SendWeaponAnim( AICORE_ATTACK );

	if ( tr.flFraction < 1.0 )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		if ( pHit )
		{
			ClearMultiDamage();
			pHit->TraceAttack( m_pPlayer->pev, 100,
				gpGlobals->v_forward, &tr, DMG_CLUB );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		}
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
			"weapons/cbar_hit1.wav", 1.0, ATTN_NORM );
	}
	else
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
			"weapons/cbar_miss1.wav", 1.0, ATTN_NORM );
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 2.0;
}

void CAICore::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( AICORE_IDLE );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
}

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
// Deploys with v_aicore.mdl (sequences: coreidle=0, coreplugin=1,
// coredraw=2).  Primary fire plays the coreplugin animation and, after
// a 0.6-second delay, executes a 100-hp hitscan within 128 units
// (matches WeaponDamage.AICore.Bullet = 100 from gunman_data.lua).
// If the struck entity is button_aiwallplug the weapon is stripped from
// the player, signalling that the puzzle interaction has completed.
// See docs/GUNMAN_LUA_PORT_PLAN.md (§2.8) for full design notes.
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

// Animation indices for V_aicore.mdl
// Sequence 0: coreidle  — idle loop
// Sequence 1: coreplugin — plug-in / attack animation
// Sequence 2: coredraw  — draw animation (also used for holster; no separate holster seq)
enum aicore_e
{
	AICORE_IDLE   = 0,  // coreidle
	AICORE_ATTACK = 1,  // coreplugin
	AICORE_DRAW   = 2,  // coredraw
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
	// View / world / player models
	PRECACHE_MODEL( "models/V_aicore.mdl" );
	PRECACHE_MODEL( "models/W_aicore.mdl" );
	// No dedicated p_aicore.mdl; reuse crowbar p-model for third-person rendering.
	PRECACHE_MODEL( "models/p_crowbar.mdl" );

	// AI plug activation sounds (from gunman_data.lua / mainframe/)
	PRECACHE_SOUND( "mainframe/aiplug_activate_gs.wav"   );
	PRECACHE_SOUND( "mainframe/aiplug_deactivate_gs.wav" );
	PRECACHE_SOUND( "mainframe/aiplug_loop_gs.wav"       );

	// Taze / electric sound used during plug-in (gunman_beamgun_taze = overheat.wav)
	PRECACHE_SOUND( "weapons/overheat.wav" );

	// Generic miss fallback
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
	// No separate holster sequence; play draw in reverse is not possible,
	// so just return to idle pose and let the item system handle the switch.
	SendWeaponAnim( AICORE_IDLE );
}

void CAICore::PrimaryAttack( void )
{
	// Play the plug-in animation immediately.
	SendWeaponAnim( AICORE_ATTACK );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Delay the actual hitscan by 0.6 s to match the animation keyframe
	// where the core is pushed into the socket (mirrors Lua timer.Simple(0.6,...)).
	SetThink( &CAICore::PlugHit );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.6;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 2.0;
}

//-----------------------------------------------------------------------------
// PlugHit — called 0.6 s after PrimaryAttack.
// Performs a 100-hp hitscan within 128 units (WeaponDamage.AICore.Bullet = 100).
// If the struck entity is button_aiwallplug the weapon is stripped from the
// player to signal that the puzzle plug-in interaction has completed.
//-----------------------------------------------------------------------------
void CAICore::PlugHit( void )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 128;

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters,
		ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction < 1.0 )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );

		// Play activation taze sound on any hit
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
			"weapons/overheat.wav", 0.9f, ATTN_NORM );

		if ( pHit )
		{
			ClearMultiDamage();
			pHit->TraceAttack( m_pPlayer->pev, 100,
				gpGlobals->v_forward, &tr, DMG_CLUB );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			// button_aiwallplug interaction: plug successful → strip weapon
			// to signal the puzzle completion, matching Lua:
			//   owner:StripWeapon(self:GetClass())
			if ( FClassnameIs( pHit->pev, "button_aiwallplug" ) )
			{
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM,
					"mainframe/aiplug_activate_gs.wav", 1.0f, ATTN_NORM );
				m_pPlayer->RemovePlayerItem( this );
			}
		}
	}
	else
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
			"weapons/cbar_miss1.wav", 1.0, ATTN_NORM );
	}
}

void CAICore::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( AICORE_IDLE );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
}

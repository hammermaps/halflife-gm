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
// weapon_fists - Gunman Chronicles fists / knife melee weapon
// Ported from MisterCalvin/SvenCoop-GC AngelScript scripts
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

// Declared in crowbar.cpp
extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );

LINK_ENTITY_TO_CLASS( weapon_fists, CGCFists );

enum fists_anim_e
{
	FISTS_IDLE			= 0,
	FISTS_IDLEJUDO,
	FISTS_IDLEKICKASS,
	FISTS_RIGHTPUNCH,
	FISTS_LEFTPUNCH,
	FISTS_DOUBLEPUNCH,
	FISTS_READY,
	FISTS_HOLSTER,
	FISTS_KNIFEDRAW,
	FISTS_KNIFEHOLSTER,
	FISTS_IDLEKNIFE,
	FISTS_IDLEKNIFEINSPECT,
	FISTS_KNIFEATTACK1,
	FISTS_KNIFEATTACK2
};

#define FISTS_BODYFISTS		0
#define FISTS_BODYKNIFE		1

#define FISTS_DAMAGE		8
#define KNIFE_DAMAGE		25
#define FISTS_RANGE			32.0f
#define KNIFE_RANGE			48.0f

void CGCFists::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FISTS;
	SET_MODEL( ENT(pev), "models/gunmanchronicles/w_knife.mdl" );

	m_iDefaultAmmo = 0;
	m_iWeaponMode  = 0; // start with fists
	m_iSwing       = 0;

	FallInit();
}

void CGCFists::Precache( void )
{
	PRECACHE_MODEL( "models/gunmanchronicles/v_hands.mdl" );
	PRECACHE_MODEL( "models/gunmanchronicles/w_knife.mdl" );
	PRECACHE_MODEL( "models/gunmanchronicles/p_crowbar.mdl" );

	PRECACHE_SOUND( "gunmanchronicles/weapons/KnifeAttack1.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/KnifeAttack1b.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/KnifeAttack2.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/KnifeAttack2b.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/KnifeDraw.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/KnifeHolster.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/LeftPunch.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/LeftPunch2.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/LeftPunch3.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/RightPunch.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/RightPunch2.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/RightPunch3.wav" );
	PRECACHE_SOUND( "gunmanchronicles/weapons/Hands_IdleKickAss_F0.wav" );

	// Fallback sounds in case Gunman assets are missing
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );
}

int CGCFists::GetItemInfo(ItemInfo *p)
{
	p->pszName    = STRING(pev->classname);
	p->pszAmmo1   = NULL;
	p->iMaxAmmo1  = -1;
	p->pszAmmo2   = NULL;
	p->iMaxAmmo2  = -1;
	p->iMaxClip   = WEAPON_NOCLIP;
	p->iSlot      = 0;
	p->iPosition  = 5;
	p->iFlags     = ITEM_FLAG_SELECTONEMPTY;
	p->iId        = m_iId = WEAPON_FISTS;
	p->iWeight    = FISTS_WEIGHT;

	return 1;
}

int CGCFists::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGCFists::Deploy( )
{
	if ( m_iWeaponMode == 1 )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/KnifeDraw.wav", 0.9, ATTN_NORM );
		return DefaultDeploy( "models/gunmanchronicles/v_hands.mdl", "models/gunmanchronicles/p_crowbar.mdl",
			FISTS_KNIFEDRAW, "onehanded", 0, FISTS_BODYKNIFE );
	}
	return DefaultDeploy( "models/gunmanchronicles/v_hands.mdl", "models/gunmanchronicles/p_crowbar.mdl",
		FISTS_READY, "onehanded", 0, FISTS_BODYFISTS );
}

void CGCFists::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_iWeaponMode == 1 )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/KnifeHolster.wav", 0.9, ATTN_NORM );
		SendWeaponAnim( FISTS_KNIFEHOLSTER, skiplocal, FISTS_BODYKNIFE );
	}
	else
	{
		SendWeaponAnim( FISTS_HOLSTER, skiplocal, FISTS_BODYFISTS );
	}
}

void CGCFists::PrimaryAttack( void )
{
	if ( !Swing( 1 ) )
	{
		SetThink( &CGCFists::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
	}
}

void CGCFists::SecondaryAttack( void )
{
	// Toggle between fists and knife
	m_iWeaponMode = ( m_iWeaponMode == 0 ) ? 1 : 0;

	// Re-deploy to show correct body group
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_iWeaponMode == 1 )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/KnifeDraw.wav", 0.9, ATTN_NORM );
		SendWeaponAnim( FISTS_KNIFEDRAW, 0, FISTS_BODYKNIFE );
	}
	else
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/KnifeHolster.wav", 0.9, ATTN_NORM );
		SendWeaponAnim( FISTS_READY, 0, FISTS_BODYFISTS );
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.8;
}

void CGCFists::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if ( m_iWeaponMode == 0 )
	{
		int iAnim;
		if ( flRand <= 0.33 )
			iAnim = FISTS_IDLE;
		else if ( flRand <= 0.66 )
			iAnim = FISTS_IDLEJUDO;
		else
		{
			EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "gunmanchronicles/weapons/Hands_IdleKickAss_F0.wav", 0.7, ATTN_NORM );
			iAnim = FISTS_IDLEKICKASS;
		}
		SendWeaponAnim( iAnim, 0, FISTS_BODYFISTS );
	}
	else
	{
		int iAnim = ( flRand <= 0.5 ) ? FISTS_IDLEKNIFE : FISTS_IDLEKNIFEINSPECT;
		SendWeaponAnim( iAnim, 0, FISTS_BODYKNIFE );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 5.0, 7.0 );
}

void CGCFists::Smack( void )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

int CGCFists::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc  = m_pPlayer->GetGunPosition();
	float  flRange = ( m_iWeaponMode == 1 ) ? KNIFE_RANGE : FISTS_RANGE;
	Vector vecEnd  = vecSrc + gpGlobals->v_forward * flRange;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr );
		if ( tr.flFraction < 1.0 )
		{
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;
		}
	}

	if ( fFirst )
	{
		// Play swing sound
		if ( m_iWeaponMode == 1 )
		{
			// Knife swing sounds
			int iSndIdx = RANDOM_LONG( 0, 3 );
			const char *szSnd;
			switch ( iSndIdx )
			{
				case 0:  szSnd = "gunmanchronicles/weapons/KnifeAttack1.wav"; break;
				case 1:  szSnd = "gunmanchronicles/weapons/KnifeAttack1b.wav"; break;
				case 2:  szSnd = "gunmanchronicles/weapons/KnifeAttack2.wav"; break;
				default: szSnd = "gunmanchronicles/weapons/KnifeAttack2b.wav"; break;
			}
			EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, szSnd, 0.9, ATTN_NORM );
		}
		else
		{
			// Punch sounds - alternate left/right
			m_iSwing++;
			const char *szSnd;
			if ( m_iSwing & 1 )
			{
				int iSndIdx = RANDOM_LONG( 0, 2 );
				switch ( iSndIdx )
				{
					case 0:  szSnd = "gunmanchronicles/weapons/RightPunch.wav"; break;
					case 1:  szSnd = "gunmanchronicles/weapons/RightPunch2.wav"; break;
					default: szSnd = "gunmanchronicles/weapons/RightPunch3.wav"; break;
				}
			}
			else
			{
				int iSndIdx = RANDOM_LONG( 0, 2 );
				switch ( iSndIdx )
				{
					case 0:  szSnd = "gunmanchronicles/weapons/LeftPunch.wav"; break;
					case 1:  szSnd = "gunmanchronicles/weapons/LeftPunch2.wav"; break;
					default: szSnd = "gunmanchronicles/weapons/LeftPunch3.wav"; break;
				}
			}
			EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, szSnd, 0.9, ATTN_NORM );
		}

		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash  = 0;

		int iAnim;
		if ( m_iWeaponMode == 1 )
		{
			iAnim = ( RANDOM_LONG(0,1) ) ? FISTS_KNIFEATTACK1 : FISTS_KNIFEATTACK2;
			SendWeaponAnim( iAnim, 0, FISTS_BODYKNIFE );
		}
		else
		{
			if ( m_iSwing % 3 == 0 )
				iAnim = FISTS_DOUBLEPUNCH;
			else if ( m_iSwing & 1 )
				iAnim = FISTS_RIGHTPUNCH;
			else
				iAnim = FISTS_LEFTPUNCH;
			SendWeaponAnim( iAnim, 0, FISTS_BODYFISTS );
		}

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}

	if ( tr.flFraction < 1.0 )
	{
		fDidHit = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		float flDamage = ( m_iWeaponMode == 1 ) ? (float)KNIFE_DAMAGE : (float)FISTS_DAMAGE;

		ClearMultiDamage();
		pEntity->TraceAttack( m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB );
		ApplyMultiDamage( pev, m_pPlayer->pev );

		m_trHit = tr;
	}

	return fDidHit;
}

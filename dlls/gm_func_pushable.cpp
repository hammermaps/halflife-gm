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
// func_pushable_gm - Gunman Chronicles pushable/breakable brush entity
// Based on CPushable from func_break.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "func_break.h"
#include "decals.h"
#include "explode.h"

#define SF_PUSH_BREAKABLE	128
#define SF_PUSH_EXPLOSIVE	256

class CFuncPushableGM : public CBreakable
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT Push( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int ObjectCaps( void ) { return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_CONTINUOUS_USE; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iFriction;
	int m_iBuoyancy;
};

TYPEDESCRIPTION CFuncPushableGM::m_SaveData[] =
{
	DEFINE_FIELD( CFuncPushableGM, m_iFriction, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncPushableGM, m_iBuoyancy, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncPushableGM, CBreakable );
LINK_ENTITY_TO_CLASS( func_pushable_gm, CFuncPushableGM );

void CFuncPushableGM::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "size" ) )
	{
		// Hull size - handled by brush
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "friction" ) )
	{
		m_iFriction = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "buoyancy" ) )
	{
		m_iBuoyancy = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBreakable::KeyValue( pkvd );
}

void CFuncPushableGM::Spawn( void )
{
	if ( pev->spawnflags & SF_PUSH_BREAKABLE )
	{
		CBreakable::Spawn();
		pev->movetype = MOVETYPE_PUSHSTEP;
	}
	else
	{
		Precache();
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSHSTEP;

		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	}

	// Default friction
	if ( m_iFriction <= 0 )
		m_iFriction = 50;
	if ( m_iBuoyancy <= 0 )
		m_iBuoyancy = 20;

	pev->friction = (float)( m_iFriction ) / 100.0;

	// Explosive pushables need health
	if ( pev->spawnflags & SF_PUSH_EXPLOSIVE )
	{
		if ( pev->health <= 0 )
			pev->health = 10;
		pev->takedamage = DAMAGE_YES;
	}

	SetTouch( &CFuncPushableGM::Push );
}

void CFuncPushableGM::Precache( void )
{
	CBreakable::Precache();
}

void CFuncPushableGM::Push( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	if ( FBitSet( pOther->pev->flags, FL_ONGROUND ) && pOther->pev->groundentity && VARS( pOther->pev->groundentity ) == pev )
	{
		// Player is standing on the pushable
		if ( pev->waterlevel > 0 )
			pev->velocity.z += pOther->pev->velocity.z * 0.1;
		return;
	}

	if ( !( pOther->pev->flags & FL_ONGROUND ) )
	{
		if ( pev->waterlevel < 1 )
			return;
	}

	// Push in the direction the player is looking
	float flSpeed = pOther->pev->velocity.Length();
	if ( flSpeed > 0 )
	{
		pev->velocity.x += pOther->pev->velocity.x * 0.25;
		pev->velocity.y += pOther->pev->velocity.y * 0.25;
	}
}

void CFuncPushableGM::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !pActivator->IsPlayer() )
		return;

	if ( pActivator->pev->velocity != g_vecZero )
	{
		pev->velocity.x += pActivator->pev->velocity.x * 0.25;
		pev->velocity.y += pActivator->pev->velocity.y * 0.25;
	}
}

int CFuncPushableGM::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pev->spawnflags & SF_PUSH_EXPLOSIVE )
	{
		pev->health -= flDamage;

		if ( pev->health <= 0 )
		{
			// Explode
			ExplosionCreate( Center(), pev->angles, edict(), ExplosionMagnitude(), TRUE );
			UTIL_Remove( this );
			return 0;
		}
	}

	if ( pev->spawnflags & SF_PUSH_BREAKABLE )
	{
		return CBreakable::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	return 1;
}

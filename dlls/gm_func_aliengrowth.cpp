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
// func_alien_growth - Gunman Chronicles alien growth brush entity
// A damaging organic brush that can grow and be destroyed
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"

#define SF_GROWTH_STARTACTIVE	1
#define SF_GROWTH_GROW			2

class CFuncAlienGrowth : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT GrowThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT GrowthTouch( CBaseEntity *pOther );
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iHealth;
	float m_flDamage;
	BOOL m_bActive;
};

TYPEDESCRIPTION CFuncAlienGrowth::m_SaveData[] =
{
	DEFINE_FIELD( CFuncAlienGrowth, m_iHealth, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncAlienGrowth, m_flDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncAlienGrowth, m_bActive, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CFuncAlienGrowth, CBaseEntity );
LINK_ENTITY_TO_CLASS( func_alien_growth, CFuncAlienGrowth );

void CFuncAlienGrowth::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "health" ) )
	{
		m_iHealth = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "dmg" ) )
	{
		m_flDamage = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CFuncAlienGrowth::Spawn( void )
{
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if ( m_iHealth <= 0 )
		m_iHealth = 100;
	if ( m_flDamage <= 0 )
		m_flDamage = 10;

	pev->health = m_iHealth;
	pev->takedamage = DAMAGE_YES;

	if ( pev->spawnflags & SF_GROWTH_STARTACTIVE )
	{
		m_bActive = TRUE;
		SetTouch( &CFuncAlienGrowth::GrowthTouch );
	}
	else
	{
		m_bActive = FALSE;
	}

	if ( pev->spawnflags & SF_GROWTH_GROW )
	{
		SetThink( &CFuncAlienGrowth::GrowThink );
		pev->nextthink = gpGlobals->time + 1.0;
	}
}

void CFuncAlienGrowth::GrowthTouch( CBaseEntity *pOther )
{
	if ( !m_bActive )
		return;

	if ( !pOther->pev->takedamage )
		return;

	pOther->TakeDamage( pev, pev, m_flDamage, DMG_ACID );
}

void CFuncAlienGrowth::GrowThink( void )
{
	if ( !m_bActive )
	{
		pev->nextthink = gpGlobals->time + 1.0;
		return;
	}

	// Gradually scale the brush
	if ( pev->scale < 1.0 )
		pev->scale += 0.01;

	pev->nextthink = gpGlobals->time + 1.0;
}

void CFuncAlienGrowth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_bActive )
	{
		m_bActive = FALSE;
		SetTouch( NULL );
	}
	else
	{
		m_bActive = TRUE;
		SetTouch( &CFuncAlienGrowth::GrowthTouch );
	}
}

int CFuncAlienGrowth::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->health -= flDamage;

	if ( pev->health <= 0 )
	{
		pev->takedamage = DAMAGE_NO;
		SetTouch( NULL );
		SetThink( NULL );
		UTIL_Remove( this );
		return 0;
	}

	return 1;
}

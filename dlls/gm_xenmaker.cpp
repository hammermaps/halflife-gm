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
// env_xenmaker - Gunman Chronicles Xen portal monster spawner
// Based on CMonsterMaker pattern from monstermaker.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"

#define SF_XENMAKER_START_ON	1
#define SF_XENMAKER_CYCLIC		2

class CEnvXenMaker : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink( void );
	void MakeMonster( void );
	void DeathNotice( entvars_t *pevChild );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMonsterClassname;
	int m_cNumMonsters;
	int m_cLiveChildren;
	int m_iMaxLiveChildren;
	float m_flGround;
	BOOL m_fActive;
};

TYPEDESCRIPTION CEnvXenMaker::m_SaveData[] =
{
	DEFINE_FIELD( CEnvXenMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CEnvXenMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvXenMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvXenMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvXenMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvXenMaker, m_fActive, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CEnvXenMaker, CBaseMonster );
LINK_ENTITY_TO_CLASS( env_xenmaker, CEnvXenMaker );

void CEnvXenMaker::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "monstertype" ) )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "monstercount" ) )
	{
		m_cNumMonsters = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flDelay" ) )
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_imaxlivechildren" ) )
	{
		m_iMaxLiveChildren = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CEnvXenMaker::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_cLiveChildren = 0;
	Precache();

	if ( !FStringNull( pev->targetname ) )
	{
		if ( pev->spawnflags & SF_XENMAKER_CYCLIC )
		{
			SetUse( &CEnvXenMaker::CyclicUse );
		}
		else
		{
			SetUse( &CEnvXenMaker::ToggleUse );
		}

		if ( FBitSet( pev->spawnflags, SF_XENMAKER_START_ON ) )
		{
			m_fActive = TRUE;
			SetThink( &CEnvXenMaker::MakerThink );
		}
		else
		{
			m_fActive = FALSE;
			SetThink( &CEnvXenMaker::SUB_DoNothing );
		}
	}
	else
	{
		pev->nextthink = gpGlobals->time + m_flDelay;
		m_fActive = TRUE;
		SetThink( &CEnvXenMaker::MakerThink );
	}

	m_flGround = 0;
}

void CEnvXenMaker::Precache( void )
{
	CBaseMonster::Precache();
}

void CEnvXenMaker::MakeMonster( void )
{
	edict_t *pent;
	entvars_t *pevCreate;

	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{
		return;
	}

	if ( !m_flGround )
	{
		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	if ( count )
	{
		return;
	}

	pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

	if ( FNullEnt( pent ) )
	{
		ALERT( at_console, "NULL Ent in XenMaker!\n" );
		return;
	}

	// Fire target on spawn
	if ( !FStringNull( pev->target ) )
	{
		FireTargets( STRING( pev->target ), this, this, USE_TOGGLE, 0 );
	}

	pevCreate = VARS( pent );
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

	DispatchSpawn( ENT( pevCreate ) );
	pevCreate->owner = edict();

	if ( !FStringNull( pev->netname ) )
	{
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++;
	m_cNumMonsters--;

	if ( m_cNumMonsters == 0 )
	{
		SetThink( NULL );
		SetUse( NULL );
	}
}

void CEnvXenMaker::CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	MakeMonster();
}

void CEnvXenMaker::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_fActive ) )
		return;

	if ( m_fActive )
	{
		m_fActive = FALSE;
		SetThink( NULL );
	}
	else
	{
		m_fActive = TRUE;
		SetThink( &CEnvXenMaker::MakerThink );
	}

	pev->nextthink = gpGlobals->time;
}

void CEnvXenMaker::MakerThink( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	MakeMonster();
}

void CEnvXenMaker::DeathNotice( entvars_t *pevChild )
{
	m_cLiveChildren--;
}

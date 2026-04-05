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
// func_vehicle_gm - Gunman Chronicles simplified vehicle entity
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"

#define SF_VEHICLE_NOPITCH		1
#define SF_VEHICLE_NOUSER		2
#define SF_VEHICLE_PASSABLE		8

class CFuncVehicleGM : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT VehicleThink( void );
	void Blocked( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_DIRECTIONAL_USE; }

	int m_iSpeed;
	int m_iHealth;
	int m_iDamage;
	float m_flVolume;
	int m_iSounds;
	BOOL m_bActive;
};

TYPEDESCRIPTION CFuncVehicleGM::m_SaveData[] =
{
	DEFINE_FIELD( CFuncVehicleGM, m_iSpeed, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVehicleGM, m_iHealth, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVehicleGM, m_iDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVehicleGM, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicleGM, m_iSounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVehicleGM, m_bActive, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CFuncVehicleGM, CBaseEntity );
LINK_ENTITY_TO_CLASS( func_vehicle_gm, CFuncVehicleGM );

void CFuncVehicleGM::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "speed" ) )
	{
		m_iSpeed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "health" ) )
	{
		m_iHealth = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "dmg" ) )
	{
		m_iDamage = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		m_flVolume = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "sounds" ) )
	{
		m_iSounds = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CFuncVehicleGM::Spawn( void )
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if ( pev->spawnflags & SF_VEHICLE_PASSABLE )
		pev->solid = SOLID_NOT;

	if ( m_flVolume == 0.0 )
		m_flVolume = 1.0;

	m_bActive = FALSE;
}

void CFuncVehicleGM::Precache( void )
{
}

void CFuncVehicleGM::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->spawnflags & SF_VEHICLE_NOUSER )
		return;

	if ( m_bActive )
	{
		m_bActive = FALSE;
		SetThink( NULL );
		pev->velocity = g_vecZero;
	}
	else
	{
		m_bActive = TRUE;
		SetThink( &CFuncVehicleGM::VehicleThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CFuncVehicleGM::VehicleThink( void )
{
	if ( !m_bActive )
		return;

	// Movement along path will be implemented with path_track support
	pev->nextthink = gpGlobals->time + 0.1;
}

void CFuncVehicleGM::Blocked( CBaseEntity *pOther )
{
	if ( m_iDamage )
	{
		pOther->TakeDamage( pev, pev, m_iDamage, DMG_CRUSH );
	}
}

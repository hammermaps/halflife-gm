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
// Gunman Chronicles info entities
// info_node_gunman - AI navigation node
// info_player_gunman - player spawn point
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"

//=========================================================
// info_node_gunman
//=========================================================
#define SF_NODE_CROUCH	1
#define SF_NODE_STAND	2
#define SF_NODE_SNIPER	4
#define SF_NODE_COMBAT	8

class CInfoNodeGunman : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iNodeType;
};

TYPEDESCRIPTION CInfoNodeGunman::m_SaveData[] =
{
	DEFINE_FIELD( CInfoNodeGunman, m_iNodeType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CInfoNodeGunman, CBaseEntity );
LINK_ENTITY_TO_CLASS( info_node_gunman, CInfoNodeGunman );

void CInfoNodeGunman::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "nodetype" ) )
	{
		m_iNodeType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CInfoNodeGunman::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	// Store node type from spawnflags if not set via keyvalue
	if ( !m_iNodeType )
		m_iNodeType = pev->spawnflags;

	SetThink( &CInfoNodeGunman::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// info_player_gunman
//=========================================================
#define SF_GUNMAN_MASTER	1

class CInfoPlayerGunman : public CPointEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsTriggered( CBaseEntity *pEntity );
};

LINK_ENTITY_TO_CLASS( info_player_gunman, CInfoPlayerGunman );

void CInfoPlayerGunman::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CInfoPlayerGunman::Spawn( void )
{
	pev->solid = SOLID_NOT;
}

BOOL CInfoPlayerGunman::IsTriggered( CBaseEntity *pEntity )
{
	BOOL master = UTIL_IsMasterTriggered( pev->netname, pEntity );
	return master;
}

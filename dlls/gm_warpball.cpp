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
// env_warpball - Gunman Chronicles warp ball energy effect
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CEnvWarpBall : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT WarpThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iRadius;
	float m_flDuration;
	BOOL m_bActive;

private:
	int m_iSpriteIndex;
};

TYPEDESCRIPTION CEnvWarpBall::m_SaveData[] =
{
	DEFINE_FIELD( CEnvWarpBall, m_iRadius, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvWarpBall, m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvWarpBall, m_bActive, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CEnvWarpBall, CBaseEntity );
LINK_ENTITY_TO_CLASS( env_warpball, CEnvWarpBall );

void CEnvWarpBall::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		m_iRadius = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		m_flDuration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvWarpBall::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if ( m_iRadius <= 0 )
		m_iRadius = 256;
	if ( m_flDuration <= 0 )
		m_flDuration = 5.0;

	m_bActive = FALSE;
}

void CEnvWarpBall::Precache( void )
{
	m_iSpriteIndex = PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "ambience/alien_powernode.wav" );
}

void CEnvWarpBall::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_bActive )
	{
		m_bActive = FALSE;
		SetThink( NULL );
	}
	else
	{
		m_bActive = TRUE;

		EMIT_SOUND( ENT( pev ), CHAN_BODY, "ambience/alien_powernode.wav", 1.0, ATTN_NORM );

		SetThink( &CEnvWarpBall::WarpThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CEnvWarpBall::WarpThink( void )
{
	if ( !m_bActive )
		return;

	// Green energy cylinder effect
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + m_iRadius );
		WRITE_SHORT( m_iSpriteIndex );
		WRITE_BYTE( 0 );	// starting frame
		WRITE_BYTE( 16 );	// framerate
		WRITE_BYTE( 2 );	// life in 0.1s
		WRITE_BYTE( 16 );	// width
		WRITE_BYTE( 0 );	// noise
		WRITE_BYTE( (int)pev->rendercolor.x );	// red
		WRITE_BYTE( (int)pev->rendercolor.y );	// green
		WRITE_BYTE( (int)pev->rendercolor.z );	// blue
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 0 );	// scroll speed
	MESSAGE_END();

	pev->nextthink = gpGlobals->time + 0.2;
}

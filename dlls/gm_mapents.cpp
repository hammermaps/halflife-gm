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
// Gunman Chronicles map utility entities
//
// random_speaker  - plays a configurable ambient sound on a
//                   periodic timer with optional randomness;
//                   can be toggled on/off via Use().
//
// gunman_cycler   - animated model prop that supports multiple
//                   bodygroups (sub-models), useful for placing
//                   decorated models in Gunman levels.
//
// Ported from MisterCalvin/SvenCoop-GC AngelScript scripts
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

//=========================================================
// random_speaker
//
// Key/values:
//   rsnoise  <soundfile>   - ambient sound to play
//   volume   <float>       - volume (0-1), default 1
//   wait     <float>       - base interval in seconds
//   random   <int>         - percentage randomness added to wait (0-100)
//=========================================================
class CRandomSpeaker : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT SpeakerThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	float	m_flVolume;    // playback volume
	float	m_flWait;      // base interval between sounds (seconds)
	int		m_iRandom;     // percentage randomness added to interval
};

TYPEDESCRIPTION CRandomSpeaker::m_SaveData[] =
{
	DEFINE_FIELD( CRandomSpeaker, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CRandomSpeaker, m_flWait,   FIELD_FLOAT ),
	DEFINE_FIELD( CRandomSpeaker, m_iRandom,  FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CRandomSpeaker, CBaseEntity );
LINK_ENTITY_TO_CLASS( random_speaker, CRandomSpeaker );

void CRandomSpeaker::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "rsnoise" ) )
	{
		// Store sound name in pev->message (a spare string field)
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "volume" ) )
	{
		m_flVolume = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "random" ) )
	{
		m_iRandom = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CRandomSpeaker::Precache( void )
{
	if ( pev->message )
	{
		const char *szSound = STRING(pev->message);
		if ( szSound && *szSound )
			PRECACHE_SOUND( (char*)szSound );
	}
}

void CRandomSpeaker::Spawn( void )
{
	Precache();

	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	// Apply defaults for unset fields
	if ( m_flVolume <= 0.0f )
		m_flVolume = 1.0f;
	if ( m_flWait <= 0.0f )
		m_flWait = 10.0f;
	if ( m_iRandom < 0 )
		m_iRandom = 0;
	if ( m_iRandom > 100 )
		m_iRandom = 100;

	// Start active by default (consistent with ambient_generic behaviour)
	SetThink( &CRandomSpeaker::SpeakerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRandomSpeaker::SpeakerThink( void )
{
	if ( pev->message )
	{
		EMIT_AMBIENT_SOUND( ENT(pev), pev->origin, STRING(pev->message),
			m_flVolume, ATTN_IDLE, 0, PITCH_NORM );
	}

	// Schedule next play: base wait + random fraction
	float flExtra = 0.0f;
	if ( m_iRandom > 0 )
		flExtra = RANDOM_FLOAT( 0.0f, m_flWait * ( m_iRandom * 0.01f ) );

	pev->nextthink = gpGlobals->time + m_flWait + flExtra;
}

void CRandomSpeaker::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	BOOL bIsActive = ( pev->nextthink > 0.0f );

	switch ( useType )
	{
	case USE_ON:
		if ( !bIsActive )
		{
			SetThink( &CRandomSpeaker::SpeakerThink );
			pev->nextthink = gpGlobals->time + 0.1f;
		}
		break;

	case USE_OFF:
		if ( bIsActive )
		{
			SetThink( NULL );
			pev->nextthink = -1.0f;
		}
		break;

	case USE_TOGGLE:
	default:
		if ( bIsActive )
		{
			SetThink( NULL );
			pev->nextthink = -1.0f;
		}
		else
		{
			SetThink( &CRandomSpeaker::SpeakerThink );
			pev->nextthink = gpGlobals->time + 0.1f;
		}
		break;
	}
}


//=========================================================
// gunman_cycler
//
// Animated model prop with up to three configurable
// bodygroups.  The entity auto-advances its animation
// sequence so the model plays continuously in-world.
//
// Key/values:
//   model         <mdl>   - model to display (standard "model" key)
//   cyc_submodel1 <int>   - body index for bodygroup 1
//   cyc_submodel2 <int>   - body index for bodygroup 2
//   cyc_submodel3 <int>   - body index for bodygroup 3
//=========================================================
class CGunmanCycler : public CBaseAnimating
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT CyclerThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	int m_iBodygroup[3];	// body values for bodygroups 1-3
	int m_iAnimate;			// 1 = auto-advance frames, 0 = frozen
};

TYPEDESCRIPTION CGunmanCycler::m_SaveData[] =
{
	DEFINE_ARRAY( CGunmanCycler, m_iBodygroup, FIELD_INTEGER, 3 ),
	DEFINE_FIELD( CGunmanCycler, m_iAnimate,   FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CGunmanCycler, CBaseAnimating );
LINK_ENTITY_TO_CLASS( gunman_cycler, CGunmanCycler );

void CGunmanCycler::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "cyc_submodel1" ) )
	{
		m_iBodygroup[0] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "cyc_submodel2" ) )
	{
		m_iBodygroup[1] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "cyc_submodel3" ) )
	{
		m_iBodygroup[2] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}

void CGunmanCycler::Spawn( void )
{
	pev->solid    = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->effects  = 0;
	pev->health   = 80000; // should never die
	pev->yaw_speed = 5;

	if ( pev->model )
		SET_MODEL( ENT(pev), STRING(pev->model) );

	// Apply bodygroup overrides using the CBaseAnimating member function
	SetBodygroup( 1, m_iBodygroup[0] );
	SetBodygroup( 2, m_iBodygroup[1] );
	SetBodygroup( 3, m_iBodygroup[2] );

	m_flFrameRate   = 75.0f;
	m_flGroundSpeed = 0.0f;

	ResetSequenceInfo();

	// If a non-default sequence/frame was set, freeze the animation
	if ( pev->sequence != 0 || pev->frame != 0 )
	{
		m_iAnimate   = 0;
		pev->framerate = 0.0f;
	}
	else
	{
		m_iAnimate   = 1;
		pev->framerate = 1.0f;
	}

	SetThink( &CGunmanCycler::CyclerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CGunmanCycler::CyclerThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if ( m_iAnimate )
		StudioFrameAdvance();

	// If the sequence ended and doesn't loop, restart it
	if ( m_fSequenceFinished && !m_fSequenceLoops )
	{
		pev->animtime  = gpGlobals->time;
		pev->framerate = 1.0f;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck  = gpGlobals->time;
		pev->frame         = 0;
		if ( !m_iAnimate )
			pev->framerate = 0.0f;
	}
}

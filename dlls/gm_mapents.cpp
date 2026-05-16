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
// random_speaker        - plays a configurable ambient sound on a
//                         periodic timer with optional randomness;
//                         can be toggled on/off via Use().
//
// gunman_cycler         - animated model prop that supports multiple
//                         bodygroups (sub-models), useful for placing
//                         decorated models in Gunman levels.
//
// decore_asteroid       - rotating asteroid prop derived from gunman_cycler.
// decore_spacedebris    - space debris launched on Use(), derived from gunman_cycler.
// decore_butterflyflock - butterfly flock animated prop.
// decore_gutspile       - guts pile static animated prop.
// entity_spritegod      - directional sprite spray emitter (toggleable via Use()).
// trigger_tank          - invisible trigger fired when the vehicle_tank_body brush
//                         enters the volume; fires targets then removes itself.
// player_gcweaponstrip  - strips weapons from one or all players via Use();
//                         optionally limited to the activator or everyone else.
// entity_digitgod       - three-digit damage counter displayed via env_sprite
//                         entities; fires targets when accumulated damage
//                         reaches a configured maximum.
//
// Ported from MisterCalvin/SvenCoop-GC AngelScript scripts
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "effects.h"

//=========================================================
// random_speaker
//
// Key/values:
//   rsnoise     <soundfile>   - ambient sound to play (also accepted as "noise")
//   volume      <float>       - volume (0-1), default 1
//   wait        <float>       - base interval in seconds
//   random      <int>         - percentage randomness added to wait (0-100)
//   attenuation <float>       - sound attenuation (0=everywhere, 1=normal, 2=idle,
//                               3=static/small, -1=no attenuation); default 1
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
	float	m_flVolume;        // playback volume
	float	m_flWait;          // base interval between sounds (seconds)
	int		m_iRandom;         // percentage randomness added to interval
	int		m_iAttenuationSel; // FGD selector: -1=unset, 0=none, 1=norm, 2=idle, 3=static
	float	m_flAttenuation;   // resolved ATTN_* float used during playback
};

TYPEDESCRIPTION CRandomSpeaker::m_SaveData[] =
{
	DEFINE_FIELD( CRandomSpeaker, m_flVolume,        FIELD_FLOAT   ),
	DEFINE_FIELD( CRandomSpeaker, m_flWait,          FIELD_FLOAT   ),
	DEFINE_FIELD( CRandomSpeaker, m_iRandom,         FIELD_INTEGER ),
	DEFINE_FIELD( CRandomSpeaker, m_iAttenuationSel, FIELD_INTEGER ),
	DEFINE_FIELD( CRandomSpeaker, m_flAttenuation,   FIELD_FLOAT   ),
};

IMPLEMENT_SAVERESTORE( CRandomSpeaker, CBaseEntity );
LINK_ENTITY_TO_CLASS( random_speaker, CRandomSpeaker );

void CRandomSpeaker::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "rsnoise" ) || FStrEq( pkvd->szKeyName, "noise" ) )
	{
		// Both "rsnoise" and "noise" accepted; store in pev->message
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
	else if ( FStrEq( pkvd->szKeyName, "attenuation" ) )
	{
		// Store as integer selector; resolved to ATTN_* float in Spawn().
		// Convention mirrors ambient_generic:
		//   0 = ATTN_NONE (everywhere)
		//   1 = ATTN_NORM (normal)
		//   2 = ATTN_IDLE (idle / short-range)
		//   3 = ATTN_STATIC (static / very short-range)
		m_iAttenuationSel = atoi( pkvd->szValue );
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
	// Resolve integer attenuation selector → engine ATTN_* float.
	// FGD choices: 1=ATTN_NORM (default), 2=ATTN_IDLE, 3=ATTN_STATIC,
	//              0=ATTN_NONE (everywhere).
	// m_iAttenuationSel is zero-initialized when the key is absent from the
	// BSP, so case 0 doubles as the "not set" path, which maps to ATTN_NORM.
	// (Hammer always writes the default value "1" to the BSP, so a zero here
	//  only appears for old maps written without this FGD key.)
	switch ( m_iAttenuationSel )
	{
	case 0: m_flAttenuation = ATTN_NORM;   break; // unset → default
	case 1: m_flAttenuation = ATTN_NORM;   break;
	case 2: m_flAttenuation = ATTN_IDLE;   break;
	case 3: m_flAttenuation = ATTN_STATIC; break;
	default: m_flAttenuation = ATTN_NONE;  break; // raw-BSP values ≤ -1
	}

	// Start active by default (consistent with ambient_generic behaviour)
	SetThink( &CRandomSpeaker::SpeakerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRandomSpeaker::SpeakerThink( void )
{
	if ( pev->message )
	{
		EMIT_AMBIENT_SOUND( ENT(pev), pev->origin, STRING(pev->message),
			m_flVolume, m_flAttenuation, 0, PITCH_NORM );
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

protected:
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


//=========================================================
// decore_asteroid
//
// Slowly rotating asteroid prop.  Extends gunman_cycler with
// per-frame angle increments and an optional size preset.
//
// Key/values:
//   model         <mdl>   - model to display
//   asteroidsize  <int>   - 0=big, 1=medium, 2=small (default small)
//   maxrotation   <float> - upper bound of per-frame angle delta (default 0.5)
//   minrotation   <float> - lower bound of per-frame angle delta (default 0.1)
//   cyc_submodel1/2/3     - inherited from gunman_cycler
//=========================================================
#define ASTEROID_SIZE_BIG		0
#define ASTEROID_SIZE_MEDIUM	1
#define ASTEROID_SIZE_SMALL		2

class CDecoreAsteroid : public CGunmanCycler
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT AsteroidThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iAsteroidSize;
	float	m_flMaxRotation;
	float	m_flMinRotation;
};

TYPEDESCRIPTION CDecoreAsteroid::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreAsteroid, m_iAsteroidSize,  FIELD_INTEGER ),
	DEFINE_FIELD( CDecoreAsteroid, m_flMaxRotation,  FIELD_FLOAT   ),
	DEFINE_FIELD( CDecoreAsteroid, m_flMinRotation,  FIELD_FLOAT   ),
};

IMPLEMENT_SAVERESTORE( CDecoreAsteroid, CGunmanCycler );
LINK_ENTITY_TO_CLASS( decore_asteroid, CDecoreAsteroid );

void CDecoreAsteroid::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "asteroidsize" ) || FStrEq( pkvd->szKeyName, "size" ) )
	{
		int size = atoi( pkvd->szValue );
		m_iAsteroidSize = ( size < ASTEROID_SIZE_BIG || size > ASTEROID_SIZE_SMALL ) ? ASTEROID_SIZE_SMALL : size;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "maxrotation" ) )
	{
		m_flMaxRotation = (float)atof( pkvd->szValue );
		if ( m_flMaxRotation < 0.0f ) m_flMaxRotation = 0.0f;
		if ( m_flMaxRotation > 360.0f ) m_flMaxRotation = 360.0f;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "minrotation" ) )
	{
		m_flMinRotation = (float)atof( pkvd->szValue );
		if ( m_flMinRotation < 0.0f ) m_flMinRotation = 0.0f;
		if ( m_flMinRotation > 360.0f ) m_flMinRotation = 360.0f;
		pkvd->fHandled = TRUE;
	}
	else
		CGunmanCycler::KeyValue( pkvd );
}

void CDecoreAsteroid::Spawn( void )
{
	// Apply defaults before calling base Spawn
	if ( m_flMaxRotation <= 0.0f ) m_flMaxRotation = 0.5f;
	if ( m_flMinRotation <= 0.0f ) m_flMinRotation = 0.1f;

	CGunmanCycler::Spawn();

	// If the model contains "Asteroid.mdl", select body/scale by size
	const char *szModel = ( pev->model ) ? STRING( pev->model ) : "";
	if ( strstr( szModel, "Asteroid.mdl" ) )
	{
		switch ( m_iAsteroidSize )
		{
		case ASTEROID_SIZE_BIG:
			pev->body  = 1;
			pev->scale = 5.0f;
			break;
		case ASTEROID_SIZE_MEDIUM:
			pev->body  = 1;
			pev->scale = 3.0f;
			break;
		default: // ASTEROID_SIZE_SMALL
			pev->body  = 0;
			pev->scale = 1.5f;
			break;
		}
	}

	pev->movetype = MOVETYPE_FLY;

	SetThink( &CDecoreAsteroid::AsteroidThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDecoreAsteroid::AsteroidThink( void )
{
	CGunmanCycler::CyclerThink();

	float flRotate = RANDOM_FLOAT( m_flMinRotation, m_flMaxRotation );
	pev->angles.x += flRotate;
	pev->angles.y += flRotate;
	pev->angles.z += flRotate;

	if ( pev->angles.x >= 360.0f ) pev->angles.x -= 360.0f;
	if ( pev->angles.y >= 360.0f ) pev->angles.y -= 360.0f;
	if ( pev->angles.z >= 360.0f ) pev->angles.z -= 360.0f;
}


//=========================================================
// decore_spacedebris
//
// A piece of space debris that begins hidden and is launched
// in a configured direction when Use() is fired.
//
// Key/values:
//   model / modelname  <mdl>   - model to display
//   dirx / diry / dirz <float> - direction vector components
//   forwardspeed       <float> - speed in units/s when launched
//   anglespeed         <float> - rotation rate (degrees per 0.1 s tick)
//   debrislife         <float> - lifetime in seconds after launch (0 = unlimited)
//   cyc_submodel1/2/3          - inherited from gunman_cycler
//=========================================================
class CDecoreSpaceDebris : public CGunmanCycler
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT SpaceDebrisThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	BOOL	m_blRotate;
	float	m_flAnglespeed;
	float	m_flLife;      // lifetime in seconds (0 = unlimited)
	float	m_flDieTime;   // absolute gpGlobals->time when to remove (set on Use())
};

TYPEDESCRIPTION CDecoreSpaceDebris::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreSpaceDebris, m_blRotate,     FIELD_BOOLEAN ),
	DEFINE_FIELD( CDecoreSpaceDebris, m_flAnglespeed, FIELD_FLOAT   ),
	DEFINE_FIELD( CDecoreSpaceDebris, m_flLife,       FIELD_FLOAT   ),
	DEFINE_FIELD( CDecoreSpaceDebris, m_flDieTime,    FIELD_TIME    ),
};

IMPLEMENT_SAVERESTORE( CDecoreSpaceDebris, CGunmanCycler );
LINK_ENTITY_TO_CLASS( decore_spacedebris, CDecoreSpaceDebris );

void CDecoreSpaceDebris::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "dirx" ) )
	{
		pev->movedir.x = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "diry" ) )
	{
		pev->movedir.y = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "dirz" ) )
	{
		pev->movedir.z = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "forwardspeed" ) )
	{
		pev->speed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "anglespeed" ) )
	{
		m_flAnglespeed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "debrislife" ) )
	{
		m_flLife = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "modelname" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CGunmanCycler::KeyValue( pkvd );
}

void CDecoreSpaceDebris::Spawn( void )
{
	if ( m_flAnglespeed <= 0.0f ) m_flAnglespeed = 3.0f;

	CGunmanCycler::Spawn();

	// Pick a random body variant (debris models often have several)
	pev->body = RANDOM_LONG( 0, 3 );

	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;  // hidden until Use() fires

	SetThink( &CDecoreSpaceDebris::SpaceDebrisThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDecoreSpaceDebris::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->effects  &= ~EF_NODRAW;
	pev->solid     = SOLID_BBOX;
	pev->movetype  = MOVETYPE_BOUNCEMISSILE;

	// Convert movedir to forward vector and scale by speed
	Vector vecAngles = UTIL_VecToAngles( pev->movedir );
	vecAngles.x = -vecAngles.x;
	vecAngles.y += 90.0f;
	MAKE_VECTORS( vecAngles );
	pev->velocity = pev->velocity + gpGlobals->v_forward * pev->speed;

	m_blRotate = TRUE;

	// Record removal time if a lifetime was configured
	if ( m_flLife > 0.0f )
		m_flDieTime = gpGlobals->time + m_flLife;
}

void CDecoreSpaceDebris::SpaceDebrisThink( void )
{
	CGunmanCycler::CyclerThink();

	// Auto-remove when lifetime expires
	if ( m_flDieTime > 0.0f && gpGlobals->time >= m_flDieTime )
	{
		UTIL_Remove( this );
		return;
	}

	if ( m_blRotate )
	{
		float flDelta = m_flAnglespeed * 0.1f;
		pev->angles.x += flDelta;
		pev->angles.y += flDelta;
		pev->angles.z += flDelta;

		if ( pev->angles.x >= 360.0f ) pev->angles.x -= 360.0f;
		if ( pev->angles.y >= 360.0f ) pev->angles.y -= 360.0f;
		if ( pev->angles.z >= 360.0f ) pev->angles.z -= 360.0f;
	}
}


//=========================================================
// decore_butterflyflock
//
// Butterfly flock animated prop.  Uses a fixed model
// (models/butterfly.mdl) with a random
// skin selection.  Starts hidden (EF_NODRAW); firing Use()
// toggles visibility so map triggers can reveal the flock.
//
// Key/values:
//   flFlockRadius  <float> - flock spawn radius (default 64)
//   iFlockSize     <int>   - number of butterflies to simulate (default 20)
//=========================================================
class CDecoreButterflyFlock : public CGunmanCycler
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float	m_flFlockRadius;
	int		m_iFlockSize;
};

TYPEDESCRIPTION CDecoreButterflyFlock::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreButterflyFlock, m_flFlockRadius, FIELD_FLOAT   ),
	DEFINE_FIELD( CDecoreButterflyFlock, m_iFlockSize,    FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CDecoreButterflyFlock, CGunmanCycler );
LINK_ENTITY_TO_CLASS( decore_butterflyflock, CDecoreButterflyFlock );

void CDecoreButterflyFlock::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "flFlockRadius" ) )
	{
		m_flFlockRadius = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "iFlockSize" ) )
	{
		m_iFlockSize = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CGunmanCycler::KeyValue( pkvd );
}

void CDecoreButterflyFlock::Precache( void )
{
	PRECACHE_MODEL( "models/butterfly.mdl" );
}

void CDecoreButterflyFlock::Spawn( void )
{
	Precache();

	pev->model = MAKE_STRING( "models/butterfly.mdl" );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	pev->skin = RANDOM_LONG( 0, 7 );

	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;

	pev->health    = 80000;
	pev->yaw_speed = 5;

	// Apply defaults for flock parameters
	if ( m_flFlockRadius <= 0.0f ) m_flFlockRadius = 64.0f;
	if ( m_iFlockSize <= 0 )       m_iFlockSize = 20;

	m_flFrameRate   = 75.0f;
	m_flGroundSpeed = 0.0f;

	ResetSequenceInfo();

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

void CDecoreButterflyFlock::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Toggle visibility each time the entity is fired
	if ( pev->effects & EF_NODRAW )
		pev->effects &= ~EF_NODRAW;
	else
		pev->effects |= EF_NODRAW;
}


//=========================================================
// decore_gutspile
//
// Guts pile static animated prop.  Uses a fixed model
// (models/Gutspile.mdl) placed in-world
// with a solid bounding box so it can serve as a prop
// that interacts with physics.
//=========================================================
class CDecoreGutspile : public CGunmanCycler
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( decore_gutspile, CDecoreGutspile );

void CDecoreGutspile::Precache( void )
{
	PRECACHE_MODEL( "models/Gutspile.mdl" );
}

void CDecoreGutspile::Spawn( void )
{
	Precache();

	pev->model = MAKE_STRING( "models/Gutspile.mdl" );

	// Use full CGunmanCycler spawn path (sets bodygroups, solid, etc.)
	CGunmanCycler::Spawn();
}


//=========================================================
// entity_spritegod
//
// Directional sprite spray emitter.  When active it fires a
// TE_SPRITE_SPRAY temp-entity packet every 0.1 s.  Use()
// toggles it on/off.
//
// Key/values:
//   spritename       <spr>   - sprite to spray
//   spritespeed      <float> - base speed of sprayed sprites
//   spritecount      <int>   - base number of sprites per burst
//   spritefreq       <int>   - adds random count on top of spritecount
//   spritenoise      <int>   - speed noise (0-255)
//   spritex          <float> - direction vector X component
//   spritey          <float> - direction vector Y component
//   spritez          <float> - direction vector Z component
//   spritestartstate <int>   - 1 = Start On (emitting from spawn)
//   targetent        <string>- attach origin to another entity by targetname
//=========================================================
class CEntitySpriteGod : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT SpriteGodThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	BOOL	m_blStartOn;       // spritestartstate: start emitting on Spawn()
	string_t m_iszTargetEnt;  // targetent: follow another entity's origin
	EHANDLE  m_hTargetEnt;    // resolved target entity handle (cached)
};

TYPEDESCRIPTION CEntitySpriteGod::m_SaveData[] =
{
	DEFINE_FIELD( CEntitySpriteGod, m_blStartOn,    FIELD_BOOLEAN ),
	DEFINE_FIELD( CEntitySpriteGod, m_iszTargetEnt, FIELD_STRING  ),
	DEFINE_FIELD( CEntitySpriteGod, m_hTargetEnt,   FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CEntitySpriteGod, CBaseEntity );

LINK_ENTITY_TO_CLASS( entity_spritegod, CEntitySpriteGod );

void CEntitySpriteGod::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "spritename" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritespeed" ) )
	{
		pev->speed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritecount" ) )
	{
		pev->iuser1 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritefreq" ) )
	{
		pev->iuser2 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritenoise" ) )
	{
		pev->iuser3 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritez" ) )
	{
		pev->movedir.z = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritey" ) )
	{
		pev->movedir.y = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritex" ) )
	{
		pev->movedir.x = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritestartstate" ) )
	{
		m_blStartOn = ( atoi( pkvd->szValue ) != 0 ) ? TRUE : FALSE;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "targetent" ) )
	{
		m_iszTargetEnt = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEntitySpriteGod::Precache( void )
{
	if ( pev->model )
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
}

void CEntitySpriteGod::Spawn( void )
{
	Precache();

	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	// Apply FGD defaults for any key that was not set by KeyValue
	if ( pev->speed  == 0.0f )  pev->speed  = 16.0f;  // spritespeed default
	if ( pev->iuser1 == 0 )     pev->iuser1 = 8;       // spritecount default
	if ( pev->iuser3 == 0 )     pev->iuser3 = 100;     // spritenoise default
	// If no direction was set, default to "straight up" (+Z)
	if ( pev->movedir == Vector( 0.0f, 0.0f, 0.0f ) )
		pev->movedir = Vector( 0.0f, 0.0f, 1.0f );

	// Start emitting immediately if spritestartstate was set
	if ( m_blStartOn )
	{
		SetThink( &CEntitySpriteGod::SpriteGodThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		pev->nextthink = 0.0f;
	}
}

void CEntitySpriteGod::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->nextthink == 0.0f )
	{
		SetThink( &CEntitySpriteGod::SpriteGodThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		SetThink( NULL );
		pev->nextthink = 0.0f;
	}
}

void CEntitySpriteGod::SpriteGodThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if ( !pev->model ) return;

	// If a target entity is configured, snap our origin to theirs each tick.
	// The EHANDLE is cached; re-resolve via name search only when it is NULL.
	if ( m_iszTargetEnt )
	{
		CBaseEntity *pTarget = m_hTargetEnt;
		if ( !pTarget )
		{
			pTarget = UTIL_FindEntityByTargetname( NULL, STRING( m_iszTargetEnt ) );
			m_hTargetEnt = pTarget;
		}
		if ( pTarget )
			UTIL_SetOrigin( pev, pTarget->pev->origin );
	}

	// Convert movedir to a forward direction vector
	Vector vecAngles = UTIL_VecToAngles( pev->movedir );
	vecAngles.x = -vecAngles.x;
	vecAngles.y += 90.0f;
	MAKE_VECTORS( vecAngles );
	Vector vecDir = gpGlobals->v_forward * 1.25f;

	int iCount = pev->iuser1 + RANDOM_LONG( 0, pev->iuser2 );
	if ( iCount < 1 ) iCount = 1;
	if ( iCount > 255 ) iCount = 255;

	int iSpeed = (int)pev->speed;
	if ( iSpeed < 0 ) iSpeed = 0;
	if ( iSpeed > 255 ) iSpeed = 255;

	int iNoise = pev->iuser3;
	if ( iNoise < 0 ) iNoise = 0;
	if ( iNoise > 255 ) iNoise = 255;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE ( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( vecDir.x );
		WRITE_COORD( vecDir.y );
		WRITE_COORD( vecDir.z );
		WRITE_SHORT( MODEL_INDEX( STRING( pev->model ) ) );
		WRITE_BYTE ( (byte)iCount );
		WRITE_BYTE ( (byte)iSpeed );
		WRITE_BYTE ( (byte)iNoise );
	MESSAGE_END();
}


//=========================================================
// trigger_tank
//
// Invisible trigger brush used by the Gunman Chronicles
// vehicle system.  When the entity "vehicle_tank_body"
// touches this volume, it fires the trigger's targets
// (passing the vehicle as activator) and removes itself.
//
// Key/values (standard): targetname, target
//=========================================================
class CTriggerTank : public CBaseEntity
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( trigger_tank, CTriggerTank );

void CTriggerTank::Spawn( void )
{
	pev->effects  |= EF_NODRAW;
	pev->solid     = SOLID_TRIGGER;
	pev->movetype  = MOVETYPE_NONE;

	if ( pev->model )
		SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetOrigin( pev, pev->origin );
}

void CTriggerTank::Touch( CBaseEntity *pOther )
{
	if ( !pOther ) return;

	if ( FClassnameIs( pOther->pev, "vehicle_tank_body" ) )
	{
		// The vehicle body's owner edict is the controlling entity
		CBaseEntity *pVehicle = CBaseEntity::Instance( pOther->pev->owner );
		SUB_UseTargets( pVehicle, USE_TOGGLE, 0.0f );
		UTIL_Remove( this );
	}
}


//=========================================================
// player_gcweaponstrip
//
// Strips all weapons from one or all players when Use() is
// called, then gives back a crowbar so the player is not
// left completely empty-handed.
//
// Key/values:
//   m_iAffected <int> - 0 = activator only (default)
//                       1 = all players
//                       2 = all players except the activator
//=========================================================
#define WEAPONSTRIP_AFFECTED_ACTIVATOR		0
#define WEAPONSTRIP_AFFECTED_ALL			1
#define WEAPONSTRIP_AFFECTED_NOTACTIVATOR	2

class CPlayerGCWeaponStrip : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	void StripAndRearm( CBasePlayer *pPlayer );

	int	m_iAffected;
};

TYPEDESCRIPTION CPlayerGCWeaponStrip::m_SaveData[] =
{
	DEFINE_FIELD( CPlayerGCWeaponStrip, m_iAffected, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CPlayerGCWeaponStrip, CBaseEntity );
LINK_ENTITY_TO_CLASS( player_gcweaponstrip, CPlayerGCWeaponStrip );

void CPlayerGCWeaponStrip::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iAffected" ) )
	{
		m_iAffected = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CPlayerGCWeaponStrip::Spawn( void )
{
	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}

void CPlayerGCWeaponStrip::StripAndRearm( CBasePlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() ) return;

	pPlayer->RemoveAllItems( FALSE );

	// Give back a basic melee weapon so the player is not empty-handed
	pPlayer->GiveNamedItem( "weapon_crowbar" );
}

void CPlayerGCWeaponStrip::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	switch ( m_iAffected )
	{
	case WEAPONSTRIP_AFFECTED_ALL:
	{
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = UTIL_FindEntityByClassname( pEnt, "player" ) ) != NULL )
			StripAndRearm( (CBasePlayer*)pEnt );
		break;
	}
	case WEAPONSTRIP_AFFECTED_NOTACTIVATOR:
	{
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = UTIL_FindEntityByClassname( pEnt, "player" ) ) != NULL )
		{
			if ( pEnt != pActivator )
				StripAndRearm( (CBasePlayer*)pEnt );
		}
		break;
	}
	default: // WEAPONSTRIP_AFFECTED_ACTIVATOR
	{
		if ( pActivator && pActivator->IsPlayer() )
			StripAndRearm( (CBasePlayer*)pActivator );
		break;
	}
	}
}


//=========================================================
// entity_digitgod
//
// A three-digit damage counter that spawns three env_sprite
// entities arranged side-by-side, each displaying one digit
// of the accumulated damage value using the Gunman digits model.
// When the accumulated damage reaches m_flMaxDmg the entity's
// targets are fired.
//
// Key/values:
//   maxdamage <float> - damage threshold that fires targets
//
// Use():
//   USE_SET with value > 0 : add value to counter
//   USE_ON / USE_OFF / USE_TOGGLE : reset counter to 0
//=========================================================
class CEntityDigitGod : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
	void UpdateOnRemove( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	void CreateSprites( void );
	void DestroySprites( void );
	void UpdateCounter( void );
	void ResetCounter( void );

	EHANDLE	m_hSprite[3];
	float	m_flCountedDmg;
	float	m_flMaxDmg;
	BOOL	m_blTriggered;
};

TYPEDESCRIPTION CEntityDigitGod::m_SaveData[] =
{
	DEFINE_FIELD( CEntityDigitGod, m_hSprite[0],   FIELD_EHANDLE ),
	DEFINE_FIELD( CEntityDigitGod, m_hSprite[1],   FIELD_EHANDLE ),
	DEFINE_FIELD( CEntityDigitGod, m_hSprite[2],   FIELD_EHANDLE ),
	DEFINE_FIELD( CEntityDigitGod, m_flCountedDmg, FIELD_FLOAT   ),
	DEFINE_FIELD( CEntityDigitGod, m_flMaxDmg,     FIELD_FLOAT   ),
	DEFINE_FIELD( CEntityDigitGod, m_blTriggered,  FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CEntityDigitGod, CBaseEntity );
LINK_ENTITY_TO_CLASS( entity_digitgod, CEntityDigitGod );

void CEntityDigitGod::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "maxdamage" ) )
	{
		m_flMaxDmg = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEntityDigitGod::Precache( void )
{
	PRECACHE_MODEL( "models/digits.mdl" );
}

void CEntityDigitGod::Spawn( void )
{
	Precache();

	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	// Default threshold to 100 if not set via KeyValue
	if ( m_flMaxDmg <= 0.0f )
		m_flMaxDmg = 100.0f;

	CreateSprites();
}

void CEntityDigitGod::UpdateOnRemove( void )
{
	DestroySprites();
	CBaseEntity::UpdateOnRemove();
}

void CEntityDigitGod::DestroySprites( void )
{
	for ( int i = 0; i < 3; i++ )
	{
		CBaseEntity *pEnt = m_hSprite[i];
		if ( pEnt )
		{
			UTIL_Remove( pEnt );
			m_hSprite[i] = NULL;
		}
	}
}

void CEntityDigitGod::CreateSprites( void )
{
	DestroySprites();

	// Build left/right offsets using the entity's own angles
	MAKE_VECTORS( pev->angles );
	// Sprite 0 = hundreds (left), sprite 1 = tens (center), sprite 2 = ones (right)
	Vector vecLeft  = gpGlobals->v_right * -16.0f;
	Vector vecRight = gpGlobals->v_right *  16.0f;

	Vector offsets[3] = { vecLeft, Vector(0,0,0), vecRight };

	for ( int i = 0; i < 3; i++ )
	{
		Vector vecOrigin = pev->origin + offsets[i];

		CSprite *pSprite = CSprite::SpriteCreate( "models/digits.mdl", vecOrigin, FALSE );
		if ( !pSprite ) break;

		pSprite->pev->angles = pev->angles;
		pSprite->pev->frame  = 0;  // start showing "0"

		m_hSprite[i] = pSprite;
	}
}

void CEntityDigitGod::UpdateCounter( void )
{
	// Extract digits most-significant-first so
	// m_hSprite[0] = hundreds (left), [1] = tens, [2] = ones (right)
	int digits[3] = { 0, 0, 0 };
	int n = (int)m_flCountedDmg;

	digits[2] = n % 10;  n /= 10;  // ones
	digits[1] = n % 10;  n /= 10;  // tens
	digits[0] = n % 10;             // hundreds (capped at 999)

	for ( int i = 0; i < 3; i++ )
	{
		CBaseEntity *pEnt = m_hSprite[i];
		if ( pEnt )
			pEnt->pev->frame = (float)digits[i];
	}
}

void CEntityDigitGod::ResetCounter( void )
{
	m_blTriggered  = FALSE;
	m_flCountedDmg = 0.0f;
}

void CEntityDigitGod::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )
	{
		ResetCounter();
	}
	else
	{
		m_flCountedDmg += value;
		if ( m_flCountedDmg < 0.0f )   m_flCountedDmg = 0.0f;
		if ( m_flCountedDmg > 999.0f )  m_flCountedDmg = 999.0f;
	}

	UpdateCounter();

	if ( !m_blTriggered && m_flCountedDmg >= m_flMaxDmg )
	{
		m_blTriggered = TRUE;
		SUB_UseTargets( pActivator, useType, value );
	}
}

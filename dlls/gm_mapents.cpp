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
// random_speaker           - plays a configurable ambient sound on a
//                            periodic timer with optional randomness;
//                            can be toggled on/off via Use().
//
// gunman_cycler            - animated model prop that supports multiple
//                            bodygroups (sub-models), useful for placing
//                            decorated models in Gunman levels.
//
// -- decore_* family (all derived from gunman_cycler unless noted) --
// decore_asteroid          - slowly rotating asteroid prop; optional size preset.
// decore_spacedebris       - space debris launched on Use(), with optional lifetime.
// decore_butterflyflock    - butterfly flock animated prop; flock radius + count keys.
// decore_gutspile          - guts pile static prop; drops to floor (models/Gutspile.mdl).
// decore_torch             - static torch with warm-light glow (models/Torch.mdl).
// decore_torchflame        - animated flame sprite companion (sprites/flames.spr).
// decore_swampplants       - swamp foliage prop; body variant selectable (models/swampstuff.mdl).
// decore_cactus            - cactus: solid, drops to floor, touch damage 1 (models/cactus.mdl).
// decore_prickle           - prickle: drops to floor (models/prickle.mdl).
// decore_cam               - sweeping security camera (models/Camera.mdl).
// decore_camflare          - camera cone/flare companion (models/cameracone.mdl).
// decore_ice               - ice block: solid, additive render, drops to floor (models/ice.mdl).
// decore_icebeak           - frozen beak creature: solid, drops to floor (models/icebeak.mdl).
// decore_labstuff          - lab equipment; body variant selectable (models/labstuff.mdl).
// decore_pteradon          - flying pteradon decoration (models/pteradon2.mdl).
// decore_pipes             - pipes: solid (models/pipes.mdl).
// decore_explodable        - destructible prop; custom model/gib/sequences.
// decore_sittingtubemortar - sitting tube mortar: solid, drops to floor, frame 1 (models/tubemortar.mdl).
// decore_eagle             - eagle decoration (models/eagle.mdl).
// decore_nest              - nest: drops to floor (models/ornest.mdl).
// decore_baboon            - baboon decoration (models/Baboon.mdl).
// decore_hatgib            - hat gib: drops to floor (models/Hatgib.mdl).
// decore_bodygib           - body gib: drops to floor (models/Bodygib.mdl).
// decore_mushroom          - mushroom: solid (models/Mushroom.mdl).
// decore_mushroom2         - mushroom variant 2: solid (models/mushroom2.mdl).
// decore_foot              - creature foot decoration (models/renesaurfoot.mdl).
// decore_aicore            - spinning AI Core prop (models/W_aicore.mdl).
// decore_goldskull         - golden skull trophy (models/goldskull.mdl).
// decore_sack              - sack/bag decoration (models/sack.mdl).
// decore_scripted_boulder  - solid boulder prop (models/boulder.mdl).
// decore_corpse            - corpse prop; drops to floor; model set by mapper.
//
// entity_spritegod      - directional sprite spray emitter (toggleable via Use()).
// trigger_tank          - invisible trigger fired when the vehicle_tank_body brush
//                         enters the volume; fires targets then removes itself.
// player_gcweaponstrip  - strips weapons from one or all players via Use();
//                         optionally limited to the activator or everyone else.
// entity_digitgod       - three-digit damage counter displayed via env_sprite
//                         entities; fires targets when accumulated damage
//                         reaches a configured maximum.
// random_trigger        - fires target at configurable random intervals;
//                         toggleable via Use(). Keys: random_min, random_max,
//                         start_state.
// decore_scripted_boulder - solid boulder prop (models/boulder.mdl).
// decore_corpse         - corpse prop; model selectable via "model" key.
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
// (models/Gutspile.mdl) placed in-world.  Drops to the
// floor on spawn; non-solid.
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

	CGunmanCycler::Spawn();

	pev->solid = SOLID_NOT;

	// Drop to the floor so guts settle naturally on surfaces
	DROP_TO_FLOOR( ENT(pev) );
}


//=========================================================
// CDecoreSimple — internal base for fixed-model decoration props.
//
// Subclasses override virtual methods to declare capabilities:
//   DefaultModel()         — model path (required)
//   ShouldDropToFloor()    — snap to ground on Spawn (default FALSE)
//   ShouldBeSolid()        — use SOLID_SLIDEBOX hull (default FALSE → SOLID_NOT)
//   TouchDamage()          — damage per touch (default 0 = no damage)
//   TouchDamageTime()      — cooldown between touches in seconds (default 1.0)
//   StartFrame()           — initial animation frame (-1 = no override)
//   ShouldRotate()         — random angular velocity on Spawn (default FALSE)
//   DefaultRenderMode()    — GoldSrc kRender* constant (-1 = no override)
//
// No custom keyvalues or save/restore fields are added; all
// per-prop state is handled by CGunmanCycler or set once in Spawn().
//=========================================================
class CDecoreSimple : public CGunmanCycler
{
public:
	virtual const char *DefaultModel( void )         { return NULL; }
	virtual BOOL        ShouldDropToFloor( void )    { return FALSE; }
	virtual BOOL        ShouldBeSolid( void )        { return FALSE; }
	virtual int         TouchDamage( void )          { return 0; }
	virtual float       TouchDamageTime( void )      { return 1.0f; }
	virtual int         StartFrame( void )           { return -1; }
	virtual BOOL        ShouldRotate( void )         { return FALSE; }
	virtual int         DefaultRenderMode( void )    { return -1; }

	void Precache( void )
	{
		if ( DefaultModel() )
			PRECACHE_MODEL( (char*)DefaultModel() );
	}

	void Spawn( void )
	{
		Precache();
		if ( !pev->model && DefaultModel() )
			pev->model = MAKE_STRING( DefaultModel() );
		CGunmanCycler::Spawn();

		// Apply solid override
		if ( !ShouldBeSolid() )
			pev->solid = SOLID_NOT;

		// Apply render mode override
		if ( DefaultRenderMode() >= 0 )
		{
			pev->rendermode = DefaultRenderMode();
			if ( pev->renderamt <= 0.0f )
				pev->renderamt = 255.0f;
		}

		// Apply initial animation frame
		if ( StartFrame() >= 0 )
			pev->frame = (float)StartFrame();

		// Apply random angular rotation (decorative spinning props)
		if ( ShouldRotate() )
		{
			pev->avelocity.x = RANDOM_FLOAT( -30.0f, 30.0f );
			pev->avelocity.y = RANDOM_FLOAT( -30.0f, 30.0f );
			pev->avelocity.z = RANDOM_FLOAT( -30.0f, 30.0f );
			pev->movetype    = MOVETYPE_FLY;
		}

		// Drop to floor last (after solid & position are set)
		if ( ShouldDropToFloor() )
			DROP_TO_FLOOR( ENT(pev) );
	}

	void Touch( CBaseEntity *pOther )
	{
		if ( TouchDamage() <= 0 ) return;
		if ( !pOther || !pOther->IsAlive() ) return;
		if ( gpGlobals->time < pev->dmgtime ) return;

		pOther->TakeDamage( pev, pev, (float)TouchDamage(), DMG_GENERIC );
		pev->dmgtime = gpGlobals->time + TouchDamageTime();
	}
};


//=========================================================
// decore_torch
//
// Static torch decoration.  Emits a warm bright-light glow.
// Default model: models/Torch.mdl
//=========================================================
class CDecoreTorch : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/Torch.mdl"; }
	void Spawn( void )
	{
		CDecoreSimple::Spawn();
		pev->effects |= EF_BRIGHTLIGHT;
	}
};

LINK_ENTITY_TO_CLASS( decore_torch, CDecoreTorch );


//=========================================================
// decore_torchflame
//
// Animated torch flame companion sprite.  Placed at the tip
// of a decore_torch to add the fire visual.  Spawns a
// looping additive sprite (sprites/flames.spr) and removes
// its placeholder entity after creation.
//=========================================================
class CDecoreTorchFlame : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_MODEL( "sprites/flames.spr" );
	}

	void Spawn( void )
	{
		Precache();
		pev->solid    = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;

		// Create a self-contained animated sprite
		CSprite *pFlame = CSprite::SpriteCreate( "sprites/flames.spr", pev->origin, FALSE );
		if ( pFlame )
		{
			pFlame->pev->rendermode  = kRenderTransAdd;
			pFlame->pev->rendercolor = Vector( 255.0f, 255.0f, 255.0f );
			pFlame->pev->renderamt   = 255.0f;
			pFlame->pev->framerate   = 10.0f;
			pFlame->TurnOn();
		}

		// Remove the placement stub — the CSprite carries on independently
		UTIL_Remove( this );
	}

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( decore_torchflame, CDecoreTorchFlame );


//=========================================================
// decore_swampplants
//
// Swamp foliage prop.  The mapper selects a body variant
// via the standard "body" key (engine-handled).
// Default model: models/swampstuff.mdl
//=========================================================
class CDecoreSwampplants : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/swampstuff.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_swampplants, CDecoreSwampplants );


//=========================================================
// decore_cactus
//
// Cactus decoration — solid (blocks movement) and deals
// 1 point of generic damage to anything that touches it,
// with a 1-second cooldown per-entity (stored on pev->dmgtime).
// Default model: models/cactus.mdl
//=========================================================
class CDecoreCactus : public CDecoreSimple
{
public:
	const char *DefaultModel( void )  { return "models/cactus.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
	BOOL        ShouldBeSolid( void ) { return TRUE; }
	int         TouchDamage( void )   { return 1; }
	float       TouchDamageTime( void ) { return 1.0f; }
};

LINK_ENTITY_TO_CLASS( decore_cactus, CDecoreCactus );


//=========================================================
// decore_prickle
//
// Prickle plant decoration — drops to the floor on spawn,
// non-solid.
// Default model: models/prickle.mdl
//=========================================================
class CDecorePrickle : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/prickle.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_prickle, CDecorePrickle );


//=========================================================
// decore_cam
//
// Rotating security camera decoration.  The camera sweeps
// ±45° on its Y axis (yaw) at a rate of 30°/s.
// Default model: models/Camera.mdl
//
// Save/restore fields: m_flBaseAngle, m_flCurOffset, m_iSweepDir
//=========================================================
#define CAM_SWEEP_HALF	45.0f	// half-arc in degrees
#define CAM_SPEED		3.0f	// degrees per 0.1 s think tick

class CDecoreCam : public CGunmanCycler
{
public:
	void Precache( void );
	void Spawn( void );
	void EXPORT CamThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float	m_flBaseAngle;
	float	m_flCurOffset;
	int		m_iSweepDir;
};

TYPEDESCRIPTION CDecoreCam::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreCam, m_flBaseAngle, FIELD_FLOAT   ),
	DEFINE_FIELD( CDecoreCam, m_flCurOffset, FIELD_FLOAT   ),
	DEFINE_FIELD( CDecoreCam, m_iSweepDir,   FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CDecoreCam, CGunmanCycler );
LINK_ENTITY_TO_CLASS( decore_cam, CDecoreCam );

void CDecoreCam::Precache( void )
{
	PRECACHE_MODEL( "models/Camera.mdl" );
}

void CDecoreCam::Spawn( void )
{
	Precache();
	if ( !pev->model )
		pev->model = MAKE_STRING( "models/Camera.mdl" );

	CGunmanCycler::Spawn();

	pev->solid = SOLID_NOT;  // camera is non-interactive decoration

	m_flBaseAngle = pev->angles.y;
	m_flCurOffset = 0.0f;
	m_iSweepDir   = 1;

	SetThink( &CDecoreCam::CamThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDecoreCam::CamThink( void )
{
	CGunmanCycler::CyclerThink();

	m_flCurOffset += CAM_SPEED * (float)m_iSweepDir;

	if ( m_flCurOffset >= CAM_SWEEP_HALF )
	{
		m_flCurOffset = CAM_SWEEP_HALF;
		m_iSweepDir   = -1;
	}
	else if ( m_flCurOffset <= -CAM_SWEEP_HALF )
	{
		m_flCurOffset = -CAM_SWEEP_HALF;
		m_iSweepDir   = 1;
	}

	pev->angles.y = m_flBaseAngle + m_flCurOffset;
	if ( pev->angles.y >= 360.0f ) pev->angles.y -= 360.0f;
	if ( pev->angles.y <    0.0f ) pev->angles.y += 360.0f;
}


//=========================================================
// decore_camflare
//
// Camera cone / light flare companion prop, placed at the
// lens of a decore_cam to indicate the camera's active FOV.
// Default model: models/cameracone.mdl
//=========================================================
class CDecoreCamflare : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/cameracone.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_camflare, CDecoreCamflare );


//=========================================================
// decore_ice
//
// Ice block decoration.  Solid (passable by bullets, blocks
// movement) with an additive render mode that gives it a
// translucent, glowing appearance.  Drops to the floor on
// spawn.  Default model: models/ice.mdl
//
// Note: use decore_icebeak for the ice-encased beak creature.
//=========================================================
class CDecoreIce : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/ice.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
	BOOL        ShouldBeSolid( void )     { return TRUE; }
	int         DefaultRenderMode( void ) { return kRenderTransAdd; }
};

LINK_ENTITY_TO_CLASS( decore_ice, CDecoreIce );


//=========================================================
// decore_icebeak
//
// Ice-encased beak creature prop — a frozen predator.
// Solid, drops to the floor.  Default model: models/icebeak.mdl
//=========================================================
class CDecoreIceBeak : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/icebeak.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
	BOOL        ShouldBeSolid( void )     { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_icebeak, CDecoreIceBeak );


//=========================================================
// decore_labstuff
//
// Laboratory equipment decoration.  The mapper selects
// a body variant via the standard "body" key (engine-handled).
// Default model: models/labstuff.mdl
//=========================================================
class CDecoreLabstuff : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/labstuff.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_labstuff, CDecoreLabstuff );


//=========================================================
// decore_pteradon
//
// Flying pteradon decoration prop.  Uses a symmetrical
// bounding box (floats in mid-air, not placed on the floor).
// Default model: models/pteradon2.mdl
//=========================================================
class CDecorePteradon : public CGunmanCycler
{
public:
	void Precache( void );
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( decore_pteradon, CDecorePteradon );

void CDecorePteradon::Precache( void )
{
	PRECACHE_MODEL( "models/pteradon2.mdl" );
}

void CDecorePteradon::Spawn( void )
{
	Precache();
	if ( !pev->model )
		pev->model = MAKE_STRING( "models/pteradon2.mdl" );

	CGunmanCycler::Spawn();

	// Non-interactive flying decoration — no solid collision
	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
}


//=========================================================
// decore_pipes
//
// Industrial pipe decoration — solid (blocks movement).
// Default model: models/pipes.mdl
//=========================================================
class CDecorePipes : public CDecoreSimple
{
public:
	const char *DefaultModel( void )  { return "models/pipes.mdl"; }
	BOOL        ShouldBeSolid( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_pipes, CDecorePipes );


//=========================================================
// decore_explodable
//
// Destructible decoration prop.  Accepts damage; when health
// reaches zero the entity spawns the configured gib model,
// triggers any targets and removes itself.  The "action"
// sequence is played when Use() is fired.
//
// Key/values:
//   decoremodel   <mdl>  - model to display (fallback for "model" key)
//   decoreidle    <int>  - idle animation sequence index (default 0)
//   decoreaction  <int>  - Use()-triggered sequence index (default 0)
//   decoregib     <mdl>  - gib model spawned on death
//                          (default: models/decoregibs2.mdl)
//   health        <int>  - hit points before exploding (default 50)
//=========================================================
class CDecoreExplodable : public CGunmanCycler
{
public:
	void Precache( void );
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	void ExplodeAndRemove( CBaseEntity *pAttacker );

	string_t	m_iszGibModel;
	int			m_iIdleSequence;
	int			m_iActionSequence;
};

TYPEDESCRIPTION CDecoreExplodable::m_SaveData[] =
{
	DEFINE_FIELD( CDecoreExplodable, m_iszGibModel,      FIELD_STRING  ),
	DEFINE_FIELD( CDecoreExplodable, m_iIdleSequence,    FIELD_INTEGER ),
	DEFINE_FIELD( CDecoreExplodable, m_iActionSequence,  FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CDecoreExplodable, CGunmanCycler );
LINK_ENTITY_TO_CLASS( decore_explodable, CDecoreExplodable );

void CDecoreExplodable::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "decoremodel" ) )
	{
		// Accept "decoremodel" as an alias for the standard "model" key
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "decoregib" ) )
	{
		m_iszGibModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "decoreidle" ) )
	{
		m_iIdleSequence = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "decoreaction" ) )
	{
		m_iActionSequence = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CGunmanCycler::KeyValue( pkvd );
}

void CDecoreExplodable::Precache( void )
{
	if ( pev->model )
		PRECACHE_MODEL( (char*)STRING( pev->model ) );

	const char *szGib = m_iszGibModel ? STRING( m_iszGibModel ) : "models/decoregibs2.mdl";
	PRECACHE_MODEL( (char*)szGib );

	PRECACHE_MODEL( "sprites/fexplo.spr" );
}

void CDecoreExplodable::Spawn( void )
{
	Precache();

	CGunmanCycler::Spawn();

	// Set idle sequence if specified
	if ( m_iIdleSequence > 0 )
	{
		pev->sequence  = m_iIdleSequence;
		pev->animtime  = gpGlobals->time;
		pev->framerate = 1.0f;
	}

	pev->takedamage = DAMAGE_YES;
	if ( pev->health <= 0.0f )
		pev->health = 50.0f;
}

void CDecoreExplodable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_iActionSequence != pev->sequence )
	{
		pev->sequence  = m_iActionSequence;
		pev->animtime  = gpGlobals->time;
		pev->framerate = 1.0f;
		pev->frame     = 0;
	}
}

int CDecoreExplodable::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->health -= flDamage;

	if ( pev->health <= 0.0f )
	{
		CBaseEntity *pAttacker = CBaseEntity::Instance( pevAttacker );
		ExplodeAndRemove( pAttacker );
		return 0;
	}

	return 1;
}

void CDecoreExplodable::ExplodeAndRemove( CBaseEntity *pAttacker )
{
	const char *szGib = m_iszGibModel ? STRING( m_iszGibModel ) : "models/decoregibs2.mdl";

	// Spawn a visible gib at the entity's origin that flies out and fades
	CGib *pGib = GetClassPtr( (CGib *)NULL );
	if ( pGib )
	{
		pGib->Spawn( szGib );
		pGib->pev->origin   = pev->origin;
		pGib->pev->angles   = pev->angles;
		pGib->pev->velocity = Vector(
			RANDOM_FLOAT( -120.0f, 120.0f ),
			RANDOM_FLOAT( -120.0f, 120.0f ),
			RANDOM_FLOAT(   80.0f, 220.0f ) );
		pGib->m_lifeTime    = 8;
		pGib->m_bloodColor  = DONT_BLEED;
	}

	// Small explosion effect at origin
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE ( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( MODEL_INDEX( "sprites/fexplo.spr" ) );
		WRITE_BYTE ( 15 );   // scale / 10
		WRITE_BYTE ( 15 );   // framerate
		WRITE_BYTE ( TE_EXPLFLAG_NOADDITIVE );
	MESSAGE_END();

	// Fire any targets
	SUB_UseTargets( pAttacker, USE_TOGGLE, 0.0f );

	UTIL_Remove( this );
}


//=========================================================
// decore_sittingtubemortar
//
// Sitting (closed/inactive) tube-mortar prop.  Solid, drops
// to the floor.  Starts at animation frame 1 (the closed pose).
// Default model: models/tubemortar.mdl
//=========================================================
class CDecoreSittingTubeMortar : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/tubemortar.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
	BOOL        ShouldBeSolid( void )     { return TRUE; }
	int         StartFrame( void )        { return 1; }
};

LINK_ENTITY_TO_CLASS( decore_sittingtubemortar, CDecoreSittingTubeMortar );


//=========================================================
// decore_eagle
//
// Eagle decoration prop — non-solid, non-interactive.
// Default model: models/eagle.mdl
//=========================================================
class CDecoreEagle : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/eagle.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_eagle, CDecoreEagle );


//=========================================================
// decore_nest
//
// Bird's nest decoration — drops to the floor on spawn.
// Default model: models/ornest.mdl
//=========================================================
class CDecoreNest : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/ornest.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_nest, CDecoreNest );


//=========================================================
// decore_baboon
//
// Baboon decoration prop.
// Default model: models/Baboon.mdl
//=========================================================
class CDecoreBaboon : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/Baboon.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_baboon, CDecoreBaboon );


//=========================================================
// decore_hatgib
//
// Hat gib decoration — drops to the floor on spawn.
// Default model: models/Hatgib.mdl
//=========================================================
class CDecoreHatgib : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/Hatgib.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_hatgib, CDecoreHatgib );


//=========================================================
// decore_bodygib
//
// Body gib decoration — drops to the floor on spawn.
// Default model: models/Bodygib.mdl
//=========================================================
class CDecoreBodygib : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return "models/Bodygib.mdl"; }
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_bodygib, CDecoreBodygib );


//=========================================================
// decore_mushroom
//
// Mushroom decoration — solid (collidable), symmetrical
// bounding box (floats or protrudes from floor).
// Default model: models/Mushroom.mdl
//=========================================================
class CDecoreMushroom : public CDecoreSimple
{
public:
	const char *DefaultModel( void )  { return "models/Mushroom.mdl"; }
	BOOL        ShouldBeSolid( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_mushroom, CDecoreMushroom );


//=========================================================
// decore_mushroom2
//
// Mushroom variant 2 — solid (collidable).
// Default model: models/mushroom2.mdl
//=========================================================
class CDecoreMushroom2 : public CDecoreSimple
{
public:
	const char *DefaultModel( void )  { return "models/mushroom2.mdl"; }
	BOOL        ShouldBeSolid( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_mushroom2, CDecoreMushroom2 );


//=========================================================
// decore_foot
//
// Creature foot decoration prop (rene-saur foot). Tall prop.
// Default model: models/renesaurfoot.mdl
//=========================================================
class CDecoreFoot : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/renesaurfoot.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_foot, CDecoreFoot );


//=========================================================
// decore_aicore
//
// Decorative spinning AI Core prop — uses the AI Core world
// model and applies a random angular velocity so it rotates
// freely in space (used in hub areas and cinematic scenes).
// Default model: models/W_aicore.mdl
//=========================================================
class CDecoreAicore : public CDecoreSimple
{
public:
	const char *DefaultModel( void )  { return "models/W_aicore.mdl"; }
	BOOL        ShouldRotate( void )  { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_aicore, CDecoreAicore );


//=========================================================
// decore_goldskull
//
// Golden skull trophy decoration.
// Default model: models/goldskull.mdl
//=========================================================
class CDecoreGoldskull : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/goldskull.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_goldskull, CDecoreGoldskull );


//=========================================================
// decore_sack
//
// Sack / bag decoration prop.
// Default model: models/sack.mdl
//=========================================================
class CDecoreSack : public CDecoreSimple
{
public:
	const char *DefaultModel( void ) { return "models/sack.mdl"; }
};

LINK_ENTITY_TO_CLASS( decore_sack, CDecoreSack );


//=========================================================
// decore_scripted_boulder
//
// Solid boulder prop used by scripted sequences (the model
// can be knocked or used in scripted scene transitions).
// Solid (blocks movement), does not drop to floor by default.
// Default model: models/Rock.mdl (nearest shipping equivalent to
// a scripted boulder — boulder.mdl is not present in the game directory)
//=========================================================
class CDecoreScriptedBoulder : public CDecoreSimple
{
public:
	const char *DefaultModel( void )  { return "models/Rock.mdl"; }
	BOOL        ShouldBeSolid( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_scripted_boulder, CDecoreScriptedBoulder );


//=========================================================
// decore_corpse
//
// Generic corpse decoration prop.  The model is selected by
// the mapper via the standard "model" key; no default model
// is imposed so the mapper can choose the appropriate corpse
// model for the scene.  Non-solid, drops to the floor on spawn
// so the corpse lies naturally on whatever surface it's above.
//=========================================================
class CDecoreCorpse : public CDecoreSimple
{
public:
	const char *DefaultModel( void )      { return NULL; }   // mapper-selected
	BOOL        ShouldDropToFloor( void ) { return TRUE; }
};

LINK_ENTITY_TO_CLASS( decore_corpse, CDecoreCorpse );


//=========================================================
// random_trigger
//
// Fires its target (and optionally its kill-target) at a
// random interval between random_min and random_max seconds.
// Can be toggled on / off via Use().
//
// Key/values:
//   target       <string> - entity to fire (standard key)
//   killtarget   <string> - entity to kill  (standard key)
//   random_min   <float>  - minimum seconds between firings (default 5)
//   random_max   <float>  - maximum seconds between firings (default 10)
//   start_state  <int>    - 0 = off on spawn (default), 1 = on immediately
//=========================================================
class CRandomTrigger : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	float	m_flRandMin;   // minimum wait between firings (seconds)
	float	m_flRandMax;   // maximum wait between firings (seconds)
	BOOL	m_bStartOn;    // 1 = start active, 0 = start inactive
};

TYPEDESCRIPTION CRandomTrigger::m_SaveData[] =
{
	DEFINE_FIELD( CRandomTrigger, m_flRandMin, FIELD_FLOAT   ),
	DEFINE_FIELD( CRandomTrigger, m_flRandMax, FIELD_FLOAT   ),
	DEFINE_FIELD( CRandomTrigger, m_bStartOn,  FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CRandomTrigger, CBaseEntity );
LINK_ENTITY_TO_CLASS( random_trigger, CRandomTrigger );

void CRandomTrigger::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "random_min" ) )
	{
		m_flRandMin = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "random_max" ) )
	{
		m_flRandMax = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "start_state" ) )
	{
		m_bStartOn = ( atoi( pkvd->szValue ) != 0 ) ? TRUE : FALSE;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CRandomTrigger::Spawn( void )
{
	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	// Apply sane defaults if the mapper left the range unset
	if ( m_flRandMin <= 0.0f ) m_flRandMin = 5.0f;
	if ( m_flRandMax <= 0.0f ) m_flRandMax = 10.0f;

	// Ensure min <= max
	if ( m_flRandMin > m_flRandMax )
	{
		float flSwap = m_flRandMin;
		m_flRandMin  = m_flRandMax;
		m_flRandMax  = flSwap;
	}

	if ( m_bStartOn )
	{
		SetThink( &CRandomTrigger::TriggerThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( m_flRandMin, m_flRandMax );
	}
}

void CRandomTrigger::TriggerThink( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0.0f );

	// Schedule next firing
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( m_flRandMin, m_flRandMax );
}

void CRandomTrigger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	BOOL bIsActive = ( pev->nextthink > 0.0f );

	switch ( useType )
	{
	case USE_ON:
		if ( !bIsActive )
		{
			SetThink( &CRandomTrigger::TriggerThink );
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT( m_flRandMin, m_flRandMax );
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
			SetThink( &CRandomTrigger::TriggerThink );
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT( m_flRandMin, m_flRandMax );
		}
		break;
	}
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

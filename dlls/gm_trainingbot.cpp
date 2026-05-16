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
// Gunman Chronicles Training Bot
// Floating drone that follows a path_corner chain.
// Translated from MisterCalvin/SvenCoop-GC monster_trainingbot.as
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "effects.h"
#include "customentity.h"
#include "weapons.h"
#include "decals.h"
#include "explode.h"

// Spawnflag: make non-solid so players walk through it (e.g. holograms)
#define SF_TRAININGBOT_NOTSOLID		4

// Number of decoration beams (one per leg attachment)
#define TRAININGBOT_MAX_BEAMS		3

// Attachment indices on batterybot.mdl
#define TRAININGBOT_ATTACH_BODYSPIKE	1
#define TRAININGBOT_ATTACH_LEG1		2
#define TRAININGBOT_ATTACH_LEG2		3
#define TRAININGBOT_ATTACH_LEG3		4

//=========================================================
// CTrainingBot
//=========================================================
class CTrainingBot : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void ) { return CLASS_MACHINE; }
	int  BloodColor( void ) { return DONT_BLEED; }

	void KeyValue( KeyValueData *pkvd );

	int  ObjectCaps( void ) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int  Save( CSave &save );
	virtual int  Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	// Sound
	void IdleSound( void );
	void PainSound( void );
	void DeathSound( void );

	// Combat
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );

	// Think functions
	void EXPORT StartThink( void );
	void EXPORT LinearMove( void );
	void EXPORT LinearMoveDone( void );
	void EXPORT FlyThink( void );
	void Flight( void );
	void UpdateGoal( void );
	void EXPORT DyingThink( void );
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void ExplodeDie( void );

	// Beams / sparkball
	void CreateBeams( void );
	void CreateBeam( int attachment );
	void ClearBeams( void );
	void CreateSparkBall( int attachment );
	void ClearSparkBall( void );

private:
	// Precached model indices
	int		m_iGibModelindex;
	int		m_iExplModelindex;
	int		m_iSmokeModelIndex;

	// Movement
	EHANDLE		m_hGoalEnt;
	Vector		m_vecFinalDest;

	// Spline interpolation state (osprey-style movement)
	Vector		m_vel1, m_vel2;
	Vector		m_pos1, m_pos2;
	Vector		m_ang1, m_ang2;
	float		m_startTime;
	float		m_dTime;
	Vector		m_velocity;

	// Decoration
	EHANDLE		m_hBeams[TRAININGBOT_MAX_BEAMS];
	int			m_iBeamCount;
	EHANDLE		m_hSparkBall;

	BOOL		m_bIsUsingOspreyMovement;
};

LINK_ENTITY_TO_CLASS( monster_trainingbot, CTrainingBot );

TYPEDESCRIPTION CTrainingBot::m_SaveData[] =
{
	DEFINE_FIELD( CTrainingBot, m_hGoalEnt,              FIELD_EHANDLE ),
	DEFINE_FIELD( CTrainingBot, m_vecFinalDest,          FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_vel1,                  FIELD_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_vel2,                  FIELD_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_pos1,                  FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_pos2,                  FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_ang1,                  FIELD_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_ang2,                  FIELD_VECTOR ),
	DEFINE_FIELD( CTrainingBot, m_startTime,             FIELD_TIME ),
	DEFINE_FIELD( CTrainingBot, m_dTime,                 FIELD_FLOAT ),
	DEFINE_FIELD( CTrainingBot, m_velocity,              FIELD_VECTOR ),
	DEFINE_ARRAY( CTrainingBot, m_hBeams,                FIELD_EHANDLE, TRAININGBOT_MAX_BEAMS ),
	DEFINE_FIELD( CTrainingBot, m_iBeamCount,            FIELD_INTEGER ),
	DEFINE_FIELD( CTrainingBot, m_hSparkBall,            FIELD_EHANDLE ),
	DEFINE_FIELD( CTrainingBot, m_bIsUsingOspreyMovement,FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CTrainingBot, CBaseMonster );

//=========================================================
// KeyValue
//=========================================================
void CTrainingBot::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "isUsingOspreyMovement" ) )
	{
		m_bIsUsingOspreyMovement = atoi( pkvd->szValue ) > 0;
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}

//=========================================================
// SetYawSpeed
//=========================================================
void CTrainingBot::SetYawSpeed( void )
{
	switch( m_Activity )
	{
	case ACT_IDLE:
	default:
		pev->yaw_speed = 45;
		break;
	}
}

//=========================================================
// Precache
//=========================================================
void CTrainingBot::Precache( void )
{
	if( !FStringNull( pev->model ) )
		PRECACHE_MODEL( (char *)STRING( pev->model ) );

	PRECACHE_MODEL( "models/batterybot.mdl" );
	m_iGibModelindex   = PRECACHE_MODEL( "models/battgib.mdl" );
	m_iExplModelindex  = PRECACHE_MODEL( "sprites/zerogxplode.spr" );
	m_iSmokeModelIndex = PRECACHE_MODEL( "sprites/steam1.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/gunmanchronicles/ballspark.spr" );

	PRECACHE_SOUND( "gunmanchronicles/drone/drone_idle.wav" );
	PRECACHE_SOUND( "buttons/spark5.wav" );
	PRECACHE_SOUND( "buttons/spark6.wav" );
	PRECACHE_SOUND( "gunmanchronicles/drone/drone_flinch1.wav" );
	PRECACHE_SOUND( "gunmanchronicles/drone/drone_flinch2.wav" );
}

//=========================================================
// Spawn
//=========================================================
void CTrainingBot::Spawn( void )
{
	Precache();

	if( FStringNull( pev->model ) )
		pev->model = MAKE_STRING( "models/batterybot.mdl" );

	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid		= SOLID_BBOX;
	pev->movetype	= MOVETYPE_NOCLIP;
	pev->flags		|= FL_MONSTER | FL_FLY;
	m_bloodColor	= DONT_BLEED;
	pev->health		= 200;
	pev->takedamage	= DAMAGE_AIM;
	pev->view_ofs	= VEC_VIEW;
	m_flFieldOfView	= 0.5f;
	m_MonsterState	= MONSTERSTATE_NONE;
	m_afCapability	= bits_CAP_DOORS_GROUP;

	m_pos2		= pev->origin;
	m_ang2		= pev->angles;
	m_vel2		= pev->velocity;
	m_startTime	= gpGlobals->time;

	MonsterInit();

	if( pev->spawnflags & SF_TRAININGBOT_NOTSOLID )
	{
		pev->solid		= SOLID_NOT;
		pev->takedamage	= DAMAGE_NO;
	}

	if( FStringNull( pev->netname ) )
		pev->netname = MAKE_STRING( "Training Bot" );

	if( pev->speed <= 0.0f )
		pev->speed = 150.0f;

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG( 0, 0xFF );

	InitBoneControllers();

	IdleSound();
	CreateBeams();
	CreateSparkBall( TRAININGBOT_ATTACH_BODYSPIKE );

	SetThink( &CTrainingBot::StartThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

//=========================================================
// StartThink — switch to the configured movement mode
//=========================================================
void CTrainingBot::StartThink( void )
{
	StudioFrameAdvance();

	if( m_bIsUsingOspreyMovement )
		SetThink( &CTrainingBot::FlyThink );
	else
		SetThink( &CTrainingBot::LinearMove );

	pev->nextthink = gpGlobals->time + 0.1f;
}

//=========================================================
// LinearMove — move toward the current goal entity, then
// advance along the path_corner chain.
//=========================================================
void CTrainingBot::LinearMove( void )
{
	StudioFrameAdvance();

	if( !m_hGoalEnt && !FStringNull( pev->target ) )
		m_hGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	CBaseEntity *pGoal = m_hGoalEnt;

	if( pGoal && pev->speed > 0.0f )
	{
		m_vecFinalDest = pGoal->pev->origin;

		if( m_vecFinalDest == pev->origin )
		{
			LinearMoveDone();
			return;
		}

		Vector vecDelta  = m_vecFinalDest - pev->origin;
		float  flTravel  = vecDelta.Length() / pev->speed;

		SetThink( &CTrainingBot::LinearMoveDone );
		pev->nextthink = gpGlobals->time + flTravel;
		pev->velocity  = vecDelta / flTravel;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

//=========================================================
// LinearMoveDone — arrived at goal; advance to next target
//=========================================================
void CTrainingBot::LinearMoveDone( void )
{
	SetThink( NULL );
	UTIL_SetOrigin( pev, m_vecFinalDest );
	pev->velocity  = g_vecZero;
	pev->nextthink = -1;

	CBaseEntity *pGoal = m_hGoalEnt;
	if( pGoal )
		m_hGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pGoal->pev->target ) );

	StartThink();
}

//=========================================================
// UpdateGoal — refresh spline interpolation waypoint data
//=========================================================
void CTrainingBot::UpdateGoal( void )
{
	CBaseEntity *pGoal = m_hGoalEnt;

	if( pGoal )
	{
		m_pos1 = m_pos2;
		m_ang1 = m_ang2;
		m_vel1 = m_vel2;
		m_pos2 = pGoal->pev->origin;
		m_ang2 = pGoal->pev->angles;

		UTIL_MakeAimVectors( Vector( 0, m_ang2.y, 0 ) );
		m_vel2 = gpGlobals->v_forward * pev->speed;

		m_startTime = m_startTime + m_dTime;
		m_dTime     = 2.0f * ( m_pos1 - m_pos2 ).Length() / ( m_vel1.Length() + pev->speed );

		if( m_ang1.y - m_ang2.y < -180.0f )
			m_ang1.y += 360.0f;
		else if( m_ang1.y - m_ang2.y > 180.0f )
			m_ang1.y -= 360.0f;
	}
	else
	{
		ALERT( at_console, "trainingbot missing target\n" );
	}
}

//=========================================================
// FlyThink — osprey-style spline flight
//=========================================================
void CTrainingBot::FlyThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( !m_hGoalEnt && !FStringNull( pev->target ) )
	{
		m_hGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		UpdateGoal();
	}

	CBaseEntity *pGoal = m_hGoalEnt;

	if( pGoal && gpGlobals->time > m_startTime + m_dTime )
	{
		m_hGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pGoal->pev->target ) );
		UpdateGoal();
	}

	Flight();
}

//=========================================================
// Flight — spline position / angle interpolation
//=========================================================
void CTrainingBot::Flight( void )
{
	float t     = gpGlobals->time - m_startTime;
	float scale = ( m_dTime > 0.0f ) ? ( 1.0f / m_dTime ) : 1.0f;
	float f     = UTIL_SplineFraction( t * scale, 1.0f );

	Vector pos = ( m_pos1 + m_vel1 * t ) * ( 1.0f - f ) + ( m_pos2 - m_vel2 * ( m_dTime - t ) ) * f;
	Vector ang = m_ang1 * ( 1.0f - f ) + m_ang2 * f;
	m_velocity = m_vel1 * ( 1.0f - f ) + m_vel2 * f;

	UTIL_SetOrigin( pev, pos );
	pev->angles = ang;
	UTIL_MakeAimVectors( pev->angles );
}

//=========================================================
// Sound
//=========================================================
void CTrainingBot::IdleSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "gunmanchronicles/drone/drone_idle.wav",
		1.0f, ATTN_NORM, 0, PITCH_NORM );
}

void CTrainingBot::PainSound( void )
{
	static const char *szSounds[] =
	{
		"buttons/spark5.wav",
		"buttons/spark6.wav"
	};
	float flVol = RANDOM_FLOAT( 0.7f, 1.0f );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE,
		szSounds[ RANDOM_LONG( 0, ARRAYSIZE( szSounds ) - 1 ) ],
		flVol, ATTN_NORM, 0, PITCH_NORM );
}

void CTrainingBot::DeathSound( void )
{
	static const char *szSounds[] =
	{
		"gunmanchronicles/drone/drone_flinch1.wav",
		"gunmanchronicles/drone/drone_flinch2.wav"
	};
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE,
		szSounds[ RANDOM_LONG( 0, ARRAYSIZE( szSounds ) - 1 ) ],
		1.0f, ATTN_NORM, 0, PITCH_NORM );
}

//=========================================================
// TraceAttack — emit sparks at hit point
//=========================================================
void CTrainingBot::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir,
	TraceResult *ptr, int bitsDamageType )
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
	UTIL_Sparks( ptr->vecEndPos );
}

//=========================================================
// TakeDamage — force-gib on explosive / energy damage;
//              force-no-gib on bullet / melee damage
//=========================================================
int CTrainingBot::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker,
	float flDamage, int bitsDamageType )
{
	if( ( bitsDamageType & DMG_CRUSH )     != 0 ||
	    ( bitsDamageType & DMG_FALL )      != 0 ||
	    ( bitsDamageType & DMG_BLAST )     != 0 ||
	    ( bitsDamageType & DMG_ENERGYBEAM )!= 0 ||
	    ( bitsDamageType & DMG_ACID )      != 0 ||
	    ( bitsDamageType & DMG_SLOWBURN )  != 0 ||
	    ( bitsDamageType & DMG_MORTAR )    != 0 )
	{
		bitsDamageType &= ~DMG_NEVERGIB;
		bitsDamageType |=  DMG_ALWAYSGIB;
	}
	else
	{
		bitsDamageType &= ~DMG_ALWAYSGIB;
		bitsDamageType |=  DMG_NEVERGIB;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Killed
//=========================================================
void CTrainingBot::Killed( entvars_t *pevAttacker, int iGib )
{
	SetThink( NULL );

	ClearBeams();
	ClearSparkBall();

	STOP_SOUND( ENT( pev ), CHAN_STATIC, "gunmanchronicles/drone/drone_idle.wav" );
	DeathSound();

	if( iGib != GIB_NEVER )
	{
		ExplodeDie();
	}
	else
	{
		pev->deadflag	= DEAD_DEAD;
		pev->framerate	= 0;
		pev->effects	= EF_NOINTERP;
		pev->movetype	= MOVETYPE_TOSS;
		pev->gravity	= 0.3f;
		pev->avelocity	= Vector( RANDOM_FLOAT( -20, 20 ), 0, RANDOM_FLOAT( -50, 50 ) );

		UTIL_SetSize( pev, Vector( -32, -32, -16 ), Vector( 32, 32, 0 ) );

		SetTouch( &CTrainingBot::CrashTouch );
		SetThink( &CTrainingBot::DyingThink );
		pev->nextthink = gpGlobals->time + 0.1f;

		m_startTime = gpGlobals->time + 6.0f;
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// DyingThink — smoke + sparks while falling; explode on land
//=========================================================
void CTrainingBot::DyingThink( void )
{
	StudioFrameAdvance();

	if( pev->angles.x < 180.0f )
		pev->angles.x += 6.0f;

	if( m_startTime > gpGlobals->time )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE ( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( m_iSmokeModelIndex );
			WRITE_BYTE ( 25 );	// scale * 10
			WRITE_BYTE ( 10 );	// framerate
		MESSAGE_END();

		UTIL_Sparks( pev->origin + Vector(
			RANDOM_FLOAT( -150, 150 ),
			RANDOM_FLOAT( -150, 150 ),
			RANDOM_FLOAT( -150, -50 ) ) );

		pev->flags    &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		SetTouch( NULL );
		m_startTime = gpGlobals->time;
		ExplodeDie();
	}
}

//=========================================================
// CrashTouch — hit solid geometry → explode immediately
//=========================================================
void CTrainingBot::CrashTouch( CBaseEntity *pOther )
{
	if( pOther->pev->solid == SOLID_BSP )
	{
		SetTouch( NULL );
		m_startTime = gpGlobals->time;
		ExplodeDie();
	}
}

//=========================================================
// ExplodeDie — explosion flash + gibs, then self-remove
//=========================================================
void CTrainingBot::ExplodeDie( void )
{
	// Fireball
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE ( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iExplModelindex );
		WRITE_BYTE ( 10 );	// scale * 10
		WRITE_BYTE ( 15 );	// framerate
		WRITE_BYTE ( 0  );	// flags
	MESSAGE_END();

	// Gibs
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE ( TE_EXPLODEMODEL );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 16.0f );
		WRITE_COORD( 300.0f );			// impulse speed
		WRITE_SHORT( m_iGibModelindex );
		WRITE_SHORT( 4 );				// count
		WRITE_BYTE ( 200 );				// life
	MESSAGE_END();

	UTIL_Remove( this );
}

//=========================================================
// CreateBeams — connect each leg attachment to the body spike
//=========================================================
void CTrainingBot::CreateBeams( void )
{
	CreateBeam( TRAININGBOT_ATTACH_LEG1 );
	CreateBeam( TRAININGBOT_ATTACH_LEG2 );
	CreateBeam( TRAININGBOT_ATTACH_LEG3 );
}

void CTrainingBot::CreateBeam( int attachment )
{
	if( m_iBeamCount >= TRAININGBOT_MAX_BEAMS )
		return;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if( !pBeam )
		return;

	pBeam->EntsInit( entindex(), entindex() );
	pBeam->SetStartAttachment( attachment );
	pBeam->SetEndAttachment( TRAININGBOT_ATTACH_BODYSPIKE );
	pBeam->SetColor( 127, 255, 212 );
	pBeam->SetBrightness( 255 );
	pBeam->SetNoise( 80 );

	m_hBeams[ m_iBeamCount ] = pBeam;
	m_iBeamCount++;
}

void CTrainingBot::ClearBeams( void )
{
	for( int i = 0; i < TRAININGBOT_MAX_BEAMS; i++ )
	{
		CBaseEntity *pBeam = m_hBeams[i];
		if( pBeam )
		{
			UTIL_Remove( pBeam );
			m_hBeams[i] = NULL;
		}
	}
	m_iBeamCount = 0;
}

//=========================================================
// CreateSparkBall — sprite attached to the body spike
//=========================================================
void CTrainingBot::CreateSparkBall( int attachment )
{
	if( m_hSparkBall )
		return;

	CSprite *pSprite = CSprite::SpriteCreate(
		"sprites/gunmanchronicles/ballspark.spr", pev->origin, TRUE );

	if( !pSprite )
		return;

	pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
	pSprite->SetAttachment( edict(), attachment );

	m_hSparkBall = pSprite;
}

void CTrainingBot::ClearSparkBall( void )
{
	CBaseEntity *pSprite = m_hSparkBall;
	if( pSprite )
	{
		UTIL_Remove( pSprite );
		m_hSparkBall = NULL;
	}
}

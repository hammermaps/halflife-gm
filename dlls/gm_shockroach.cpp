/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Gunman Chronicles Shockroach - small electric alien bug
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	SHOCKROACH_AE_SHOCK		0x01

#define SHOCKROACH_FLINCH_DELAY	2

class CShockroach : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int  IgnoreConditions( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void DeathSound( void );

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackSounds[];

	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_shockroach, CShockroach );

const char *CShockroach::pIdleSounds[] =
{
	"headcrab/hc_idle1.wav",		// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_idle2.wav",
	"headcrab/hc_idle3.wav",
};

const char *CShockroach::pAlertSounds[] =
{
	"headcrab/hc_alert1.wav",		// TODO: replace with Gunman Chronicles sounds
};

const char *CShockroach::pPainSounds[] =
{
	"headcrab/hc_pain1.wav",		// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_pain2.wav",
	"headcrab/hc_pain3.wav",
};

const char *CShockroach::pDeathSounds[] =
{
	"headcrab/hc_die1.wav",			// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_die2.wav",
};

const char *CShockroach::pAttackSounds[] =
{
	"headcrab/hc_attack1.wav",		// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_attack2.wav",
	"headcrab/hc_attack3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CShockroach :: Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CShockroach :: SetYawSpeed( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 30;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 20;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 30;
		break;
	default:
		ys = 30;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// TakeDamage
//=========================================================
int CShockroach :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Immune to electric shock damage
	if ( bitsDamageType & DMG_SHOCK )
		flDamage = 0;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Sound functions
//=========================================================
void CShockroach :: PainSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
}

void CShockroach :: AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
}

void CShockroach :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 + RANDOM_LONG(-5,5) );
}

void CShockroach :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
}

//=========================================================
// CheckRangeAttack1 - shock attack within 256 units
//=========================================================
BOOL CShockroach :: CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist <= 256 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CShockroach :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SHOCKROACH_AE_SHOCK:
		{
			// Electric shock attack - 10 damage, DMG_SHOCK type
			CBaseEntity *pHurt = CheckTraceHullAttack( 48, 10, DMG_SHOCK );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -5;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CShockroach :: Spawn()
{
	Precache( );

	SET_MODEL( ENT(pev), "models/w_squeak.mdl" );	// TODO: replace with Gunman Chronicles model
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 16 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= 30;
	pev->view_ofs		= Vector( 0, 0, 12 );
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShockroach :: Precache()
{
	int i;

	PRECACHE_MODEL( "models/w_squeak.mdl" );	// TODO: replace with Gunman Chronicles model

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char *)pIdleSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char *)pAlertSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char *)pPainSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND( (char *)pDeathSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackSounds[i] );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CShockroach::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_RANGE_ATTACK1))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + SHOCKROACH_FLINCH_DELAY;
	}

	return iIgnore;
}

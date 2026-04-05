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
// Gunman Chronicles Massasaur - large predatory alien
// (similar to bullsquid)
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
#define	MASSASAUR_AE_BITE		0x01
#define	MASSASAUR_AE_TAILWHIP	0x02

#define MASSASAUR_FLINCH_DELAY	2

class CMassasaur : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int  IgnoreConditions( void );

	void KeyValue( KeyValueData *pkvd );

	float m_flNextFlinch;
	int m_iBodyType;		// 0 = Normal, 1 = Alpha (+200 health)

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void DeathSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_massasaur, CMassasaur );

const char *CMassasaur::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CMassasaur::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/claw_miss2.wav",
};

const char *CMassasaur::pAttackSounds[] =
{
	"bullchicken/bc_attack1.wav",	// TODO: replace with Gunman Chronicles sounds
	"bullchicken/bc_attack2.wav",
	"bullchicken/bc_attack3.wav",
};

const char *CMassasaur::pIdleSounds[] =
{
	"bullchicken/bc_idle1.wav",		// TODO: replace with Gunman Chronicles sounds
	"bullchicken/bc_idle2.wav",
	"bullchicken/bc_idle3.wav",
	"bullchicken/bc_idle4.wav",
	"bullchicken/bc_idle5.wav",
};

const char *CMassasaur::pAlertSounds[] =
{
	"bullchicken/bc_idle1.wav",		// TODO: replace with Gunman Chronicles sounds
	"bullchicken/bc_idle2.wav",
};

const char *CMassasaur::pPainSounds[] =
{
	"bullchicken/bc_pain1.wav",		// TODO: replace with Gunman Chronicles sounds
	"bullchicken/bc_pain2.wav",
	"bullchicken/bc_pain3.wav",
	"bullchicken/bc_pain4.wav",
};

const char *CMassasaur::pDeathSounds[] =
{
	"bullchicken/bc_die1.wav",		// TODO: replace with Gunman Chronicles sounds
	"bullchicken/bc_die2.wav",
	"bullchicken/bc_die3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CMassasaur :: Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// KeyValue
//=========================================================
void CMassasaur :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "body" ) )
	{
		m_iBodyType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMassasaur :: SetYawSpeed( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_WALK:
	case ACT_RUN:
		ys = 90;
		break;
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// TakeDamage
//=========================================================
int CMassasaur :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Thick hide - reduced bullet damage
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.5;
	}

	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Sound functions
//=========================================================
void CMassasaur :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CMassasaur :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CMassasaur :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CMassasaur :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0,9) );
}

void CMassasaur :: AttackSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

//=========================================================
// CheckRangeAttack1 - ranged spit attack within 512 units
//=========================================================
BOOL CMassasaur :: CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist <= 512 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassasaur :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case MASSASAUR_AE_BITE:
		{
			// Bite attack - 40 damage
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, 40, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -15;
					pHurt->pev->punchangle.x = 8;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				}
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case MASSASAUR_AE_TAILWHIP:
		{
			// Tail whip attack - 30 damage, knocks back
			CBaseEntity *pHurt = CheckTraceHullAttack( 85, 30, DMG_CLUB );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 20;
					pHurt->pev->punchangle.x = 8;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 150;
				}
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			AttackSound();
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
void CMassasaur :: Spawn()
{
	Precache( );

	SET_MODEL( ENT(pev), "models/bullsquid.mdl" );	// TODO: replace with Gunman Chronicles model
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= 300;
	pev->view_ofs		= Vector( 0, 0, 32 );
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	// Apply alpha variant bonus
	if ( m_iBodyType == 1 )
	{
		pev->body = 1;
		pev->health += 200;
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassasaur :: Precache()
{
	int i;

	PRECACHE_MODEL( "models/bullsquid.mdl" );	// TODO: replace with Gunman Chronicles model

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackHitSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackMissSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char *)pIdleSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char *)pAlertSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char *)pPainSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND( (char *)pDeathSounds[i] );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CMassasaur::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK1))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + MASSASAUR_FLINCH_DELAY;
	}

	return iIgnore;
}

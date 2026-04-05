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
// Gunman Chronicles Geneworm - large alien boss creature
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
#define	GENEWORM_AE_MELEE		0x01
#define	GENEWORM_AE_RANGED		0x02

#define GENEWORM_FLINCH_DELAY	4

class CGeneworm : public CBaseMonster
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
	int m_iCustomHealth;	// configurable via KeyValue

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

LINK_ENTITY_TO_CLASS( monster_geneworm, CGeneworm );

const char *CGeneworm::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGeneworm::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/claw_miss2.wav",
};

const char *CGeneworm::pAttackSounds[] =
{
	"gonarch/gon_attack1.wav",		// TODO: replace with Gunman Chronicles sounds
	"gonarch/gon_attack2.wav",
	"gonarch/gon_attack3.wav",
};

const char *CGeneworm::pIdleSounds[] =
{
	"gonarch/gon_sack1.wav",		// TODO: replace with Gunman Chronicles sounds
	"gonarch/gon_sack2.wav",
	"gonarch/gon_sack3.wav",
};

const char *CGeneworm::pAlertSounds[] =
{
	"gonarch/gon_attack1.wav",		// TODO: replace with Gunman Chronicles sounds
	"gonarch/gon_attack2.wav",
};

const char *CGeneworm::pPainSounds[] =
{
	"gonarch/gon_pain2.wav",		// TODO: replace with Gunman Chronicles sounds
	"gonarch/gon_pain4.wav",
	"gonarch/gon_pain5.wav",
};

const char *CGeneworm::pDeathSounds[] =
{
	"gonarch/gon_die1.wav",			// TODO: replace with Gunman Chronicles sounds
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGeneworm :: Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// KeyValue
//=========================================================
void CGeneworm :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "health" ) )
	{
		m_iCustomHealth = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGeneworm :: SetYawSpeed( void )
{
	int ys;

	ys = 60;

	pev->yaw_speed = ys;
}

//=========================================================
// TakeDamage
//=========================================================
int CGeneworm :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Boss takes reduced bullet damage
	if ( bitsDamageType == DMG_BULLET )
	{
		flDamage *= 0.5;
	}

	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Sound functions
//=========================================================
void CGeneworm :: PainSound( void )
{
	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0,9) );
}

void CGeneworm :: AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0,9) );
}

void CGeneworm :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CGeneworm :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0,9) );
}

void CGeneworm :: AttackSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

//=========================================================
// CheckRangeAttack1 - ranged attack within 1024 units
//=========================================================
BOOL CGeneworm :: CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGeneworm :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case GENEWORM_AE_MELEE:
		{
			// Heavy melee attack - 50 damage
			CBaseEntity *pHurt = CheckTraceHullAttack( 120, 50, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 15;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -200;
				}
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			AttackSound();
		}
		break;

		case GENEWORM_AE_RANGED:
		{
			// TODO: Implement ranged projectile attack
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
void CGeneworm :: Spawn()
{
	Precache( );

	SET_MODEL( ENT(pev), "models/big_mom.mdl" );	// TODO: replace with Gunman Chronicles model
	UTIL_SetSize( pev, Vector( -128, -128, 0 ), Vector( 128, 128, 256 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= (m_iCustomHealth > 0) ? m_iCustomHealth : 500;
	pev->view_ofs		= Vector( 0, 0, 128 );
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGeneworm :: Precache()
{
	int i;

	PRECACHE_MODEL( "models/big_mom.mdl" );		// TODO: replace with Gunman Chronicles model

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

int CGeneworm::IgnoreConditions( void )
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
			m_flNextFlinch = gpGlobals->time + GENEWORM_FLINCH_DELAY;
	}

	return iIgnore;
}

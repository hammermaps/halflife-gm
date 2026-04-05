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
// Gunman Chronicles Dinnerjacket - heavy melee alien
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	DINNERJACKET_AE_ATTACK		0x01

#define DINNERJACKET_FLINCH_DELAY	2

class CDinnerjacket : public CBaseMonster
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
	int m_iBodyType;		// 0 = Normal, 1 = Heavy (+100 health)

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

	// Melee only - no range attacks
	BOOL CheckRangeAttack1( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_dinnerjacket, CDinnerjacket );

const char *CDinnerjacket::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CDinnerjacket::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/claw_miss2.wav",
};

const char *CDinnerjacket::pAttackSounds[] =
{
	"zombie/zo_attack1.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/zo_attack2.wav",
};

const char *CDinnerjacket::pIdleSounds[] =
{
	"zombie/zo_idle1.wav",			// TODO: replace with Gunman Chronicles sounds
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CDinnerjacket::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",		// TODO: replace with Gunman Chronicles sounds
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CDinnerjacket::pPainSounds[] =
{
	"zombie/zo_pain1.wav",			// TODO: replace with Gunman Chronicles sounds
	"zombie/zo_pain2.wav",
};

const char *CDinnerjacket::pDeathSounds[] =
{
	"zombie/zo_pain1.wav",			// TODO: replace with Gunman Chronicles sounds
	"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CDinnerjacket :: Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// KeyValue
//=========================================================
void CDinnerjacket :: KeyValue( KeyValueData *pkvd )
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
void CDinnerjacket :: SetYawSpeed( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

//=========================================================
// TakeDamage
//=========================================================
int CDinnerjacket :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets (tough hide)
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Sound functions
//=========================================================
void CDinnerjacket :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CDinnerjacket :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CDinnerjacket :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CDinnerjacket :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0,9) );
}

void CDinnerjacket :: AttackSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDinnerjacket :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case DINNERJACKET_AE_ATTACK:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgBothSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
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
void CDinnerjacket :: Spawn()
{
	Precache( );

	SET_MODEL( ENT(pev), "models/zombie.mdl" );		// TODO: replace with Gunman Chronicles model
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= 200;
	pev->view_ofs		= VEC_VIEW;
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	// Apply heavy variant bonus
	if ( m_iBodyType == 1 )
	{
		pev->body = 1;
		pev->health += 100;
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDinnerjacket :: Precache()
{
	int i;

	PRECACHE_MODEL( "models/zombie.mdl" );		// TODO: replace with Gunman Chronicles model

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

int CDinnerjacket::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + DINNERJACKET_FLINCH_DELAY;
	}

	return iIgnore;
}

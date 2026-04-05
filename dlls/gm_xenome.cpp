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
// Gunman Chronicles Xenome - small leaping alien parasite
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
#define	XENOME_AE_ATTACK		0x01
#define	XENOME_AE_LEAP			0x02

#define XENOME_FLINCH_DELAY		2

class CXenome : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int  IgnoreConditions( void );

	void EXPORT LeapTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

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
	static const char *pBiteSounds[];

	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_xenome, CXenome );

TYPEDESCRIPTION CXenome::m_SaveData[] = 
{
	DEFINE_FIELD( CXenome, m_flNextFlinch, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CXenome, CBaseMonster );

const char *CXenome::pIdleSounds[] =
{
	"headcrab/hc_idle1.wav",		// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_idle2.wav",
	"headcrab/hc_idle3.wav",
};

const char *CXenome::pAlertSounds[] =
{
	"headcrab/hc_alert1.wav",		// TODO: replace with Gunman Chronicles sounds
};

const char *CXenome::pPainSounds[] =
{
	"headcrab/hc_pain1.wav",		// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_pain2.wav",
	"headcrab/hc_pain3.wav",
};

const char *CXenome::pDeathSounds[] =
{
	"headcrab/hc_die1.wav",			// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_die2.wav",
};

const char *CXenome::pAttackSounds[] =
{
	"headcrab/hc_attack1.wav",		// TODO: replace with Gunman Chronicles sounds
	"headcrab/hc_attack2.wav",
	"headcrab/hc_attack3.wav",
};

const char *CXenome::pBiteSounds[] =
{
	"headcrab/hc_headbite.wav",		// TODO: replace with Gunman Chronicles sounds
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CXenome :: Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CXenome :: SetYawSpeed( void )
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
int CXenome :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Immune to acid damage
	if ( bitsDamageType & DMG_ACID )
		flDamage = 0;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Sound functions
//=========================================================
void CXenome :: PainSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
}

void CXenome :: AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
}

void CXenome :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 + RANDOM_LONG(-5,5) );
}

void CXenome :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
}

//=========================================================
// CheckRangeAttack1 - leap attack within 256 units
//=========================================================
BOOL CXenome :: CheckRangeAttack1( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// LeapTouch - this is the xenome's touch function when it
// is in the air
//=========================================================
void CXenome :: LeapTouch( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage )
	{
		return;
	}

	if ( pOther->Classify() == Classify() )
	{
		return;
	}

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pBiteSounds[ RANDOM_LONG(0,ARRAYSIZE(pBiteSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );

		pOther->TakeDamage( pev, pev, gSkillData.headcrabDmgBite, DMG_SLASH );
	}

	SetTouch( NULL );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CXenome :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case XENOME_AE_ATTACK:
		{
			// Standard melee bite attack
			CBaseEntity *pHurt = CheckTraceHullAttack( 48, gSkillData.headcrabDmgBite, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -10;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, pBiteSounds[ RANDOM_LONG(0,ARRAYSIZE(pBiteSounds)-1) ], 1.0, ATTN_IDLE, 0, 100 );
			}
		}
		break;

		case XENOME_AE_LEAP:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );
			UTIL_MakeVectors( pev->angles );

			Vector vecJumpDir;
			if ( m_hEnemy != NULL )
			{
				float gravity = g_psv_gravity->value;
				if ( gravity <= 1 )
					gravity = 1;

				float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if ( height < 16 )
					height = 16;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				vecJumpDir.z = speed;

				float distance = vecJumpDir.Length();
				if ( distance > 650 )
				{
					vecJumpDir = vecJumpDir * ( 650.0 / distance );
				}
			}
			else
			{
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			int iSound = RANDOM_LONG(0,2);
			if ( iSound != 0 )
				EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[iSound], 1.0, ATTN_IDLE, 0, 100 );

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
			SetTouch( &CXenome::LeapTouch );
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
void CXenome :: Spawn()
{
	Precache( );

	SET_MODEL( ENT(pev), "models/headcrab.mdl" );	// TODO: replace with Gunman Chronicles model
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= 80;
	pev->view_ofs		= Vector( 0, 0, 20 );
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CXenome :: Precache()
{
	int i;

	PRECACHE_MODEL( "models/headcrab.mdl" );	// TODO: replace with Gunman Chronicles model

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

	for ( i = 0; i < ARRAYSIZE( pBiteSounds ); i++ )
		PRECACHE_SOUND( (char *)pBiteSounds[i] );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CXenome::IgnoreConditions( void )
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
			m_flNextFlinch = gpGlobals->time + XENOME_FLINCH_DELAY;
	}

	return iIgnore;
}

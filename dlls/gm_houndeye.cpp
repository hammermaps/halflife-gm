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
// Gunman Chronicles Houndeye - armored sonic dog variant
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"game.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HOUNDEYEGM_AE_WARN			1
#define		HOUNDEYEGM_AE_STARTATTACK	2
#define		HOUNDEYEGM_AE_THUMP		3

#define		HOUNDEYEGM_MAX_ATTACK_RADIUS	384

#define HOUNDEYEGM_FLINCH_DELAY		2

class CHoundeyeGM : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void DeathSound( void );
	void WarnSound( void );

	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int m_iArmorBody;		// 0 = Normal, 1 = Armored (+50 health)
	float m_flNextFlinch;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pWarnSounds[];
};

LINK_ENTITY_TO_CLASS( monster_houndeye_gm, CHoundeyeGM );

TYPEDESCRIPTION CHoundeyeGM::m_SaveData[] = 
{
	DEFINE_FIELD( CHoundeyeGM, m_iArmorBody, FIELD_INTEGER ),
	DEFINE_FIELD( CHoundeyeGM, m_flNextFlinch, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHoundeyeGM, CSquadMonster );

const char *CHoundeyeGM::pIdleSounds[] =
{
	"houndeye/he_idle1.wav",
	"houndeye/he_idle2.wav",
	"houndeye/he_idle3.wav",
};

const char *CHoundeyeGM::pAlertSounds[] =
{
	"houndeye/he_alert1.wav",
	"houndeye/he_alert2.wav",
	"houndeye/he_alert3.wav",
};

const char *CHoundeyeGM::pPainSounds[] =
{
	"houndeye/he_pain1.wav",
	"houndeye/he_pain3.wav",
	"houndeye/he_pain4.wav",
	"houndeye/he_pain5.wav",
};

const char *CHoundeyeGM::pDeathSounds[] =
{
	"houndeye/he_die1.wav",
	"houndeye/he_die2.wav",
	"houndeye/he_die3.wav",
};

const char *CHoundeyeGM::pWarnSounds[] =
{
	"houndeye/he_hunt1.wav",
	"houndeye/he_hunt2.wav",
	"houndeye/he_hunt3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHoundeyeGM :: Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// KeyValue
//=========================================================
void CHoundeyeGM :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "body" ) )
	{
		m_iArmorBody = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CSquadMonster::KeyValue( pkvd );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHoundeyeGM :: SetYawSpeed( void )
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
int CHoundeyeGM :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Armored variant takes reduced bullet damage
	if ( m_iArmorBody == 1 && bitsDamageType == DMG_BULLET )
	{
		flDamage *= 0.5;
	}

	if ( IsAlive() )
		PainSound();

	return CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Sound functions
//=========================================================
void CHoundeyeGM :: PainSound( void )
{
	if ( RANDOM_LONG( 0, 5 ) < 2 )
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG( 0, ARRAYSIZE(pPainSounds) - 1 ) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 9 ) );
}

void CHoundeyeGM :: AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG( 0, ARRAYSIZE(pAlertSounds) - 1 ) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 9 ) );
}

void CHoundeyeGM :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG( 0, ARRAYSIZE(pIdleSounds) - 1 ) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
}

void CHoundeyeGM :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG( 0, ARRAYSIZE(pDeathSounds) - 1 ) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 9 ) );
}

void CHoundeyeGM :: WarnSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pWarnSounds[ RANDOM_LONG( 0, ARRAYSIZE(pWarnSounds) - 1 ) ], 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 9 ) );
}

//=========================================================
// CheckRangeAttack1 - sonic attack within 384 units
//=========================================================
BOOL CHoundeyeGM :: CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist <= HOUNDEYEGM_MAX_ATTACK_RADIUS && flDot > 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHoundeyeGM :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HOUNDEYEGM_AE_WARN:
		{
			WarnSound();
		}
		break;

		case HOUNDEYEGM_AE_STARTATTACK:
		{
			WarnSound();
		}
		break;

		case HOUNDEYEGM_AE_THUMP:
		{
			// Sonic attack - damage entities in radius
			// TODO: Implement full sonic blast with shockwave effect
			CBaseEntity *pEntity = NULL;
			float flDist;

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "houndeye/he_blast1.wav", 1.0, ATTN_NORM, 0, PITCH_LOW + RANDOM_LONG( 0, 9 ) );

			// Damage entities in attack radius
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, HOUNDEYEGM_MAX_ATTACK_RADIUS ) ) != NULL )
			{
				if ( pEntity->pev->takedamage != DAMAGE_NO )
				{
					if ( !FClassnameIs( pEntity->pev, "monster_houndeye_gm" ) )
					{
						flDist = ( pEntity->Center() - pev->origin ).Length();
						// 20 base damage, falls off with distance
						float flDamage = 20 - ( 20.0 * ( flDist / HOUNDEYEGM_MAX_ATTACK_RADIUS ) );
						if ( flDamage > 0 )
							pEntity->TakeDamage( pev, pev, flDamage, DMG_SONIC );
					}
				}
			}
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHoundeyeGM :: Spawn()
{
	Precache( );

	SET_MODEL( ENT(pev), "models/houndeye.mdl" );	// TODO: replace with Gunman Chronicles model
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 36 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= 120;
	pev->view_ofs		= Vector( 0, 0, 20 );
	m_flFieldOfView		= 0.5;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_SQUAD;

	// Apply armor variant bonus
	if ( m_iArmorBody == 1 )
	{
		pev->body = 1;
		pev->health += 50;
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHoundeyeGM :: Precache()
{
	int i;

	PRECACHE_MODEL( "models/houndeye.mdl" );	// TODO: replace with Gunman Chronicles model

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char *)pIdleSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char *)pAlertSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char *)pPainSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND( (char *)pDeathSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pWarnSounds ); i++ )
		PRECACHE_SOUND( (char *)pWarnSounds[i] );

	PRECACHE_SOUND( "houndeye/he_blast1.wav" );
	PRECACHE_SOUND( "houndeye/he_blast2.wav" );
	PRECACHE_SOUND( "houndeye/he_blast3.wav" );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CHoundeyeGM::IgnoreConditions( void )
{
	int iIgnore = CSquadMonster::IgnoreConditions();

	if ( (m_Activity == ACT_RANGE_ATTACK1) )
	{
		if ( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ( (m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH) )
	{
		if ( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + HOUNDEYEGM_FLINCH_DELAY;
	}

	return iIgnore;
}

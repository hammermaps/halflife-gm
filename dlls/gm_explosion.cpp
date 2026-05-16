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
// env_explosion_gm - Gunman Chronicles custom explosion entity
// with radius override and custom sprite support
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "weapons.h"
#include "gunman_damage.h"
#include "gunman_sounds.h"

// Spawnflags
#define SF_ENVEXPLOSION_GM_NODAMAGE		( 1 << 0 )
#define SF_ENVEXPLOSION_GM_REPEATABLE	( 1 << 1 )
#define SF_ENVEXPLOSION_GM_NOFIREBALL	( 1 << 2 )
#define SF_ENVEXPLOSION_GM_NOSMOKE		( 1 << 3 )
#define SF_ENVEXPLOSION_GM_NODECAL		( 1 << 4 )
#define SF_ENVEXPLOSION_GM_NOSPARKS		( 1 << 5 )

class CEnvExplosionGM : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT Smoke( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude;
	int m_spriteScale;
	int m_iRadiusOverride;
	int m_iSpriteIndex;
};

TYPEDESCRIPTION CEnvExplosionGM::m_SaveData[] =
{
	DEFINE_FIELD( CEnvExplosionGM, m_iMagnitude, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosionGM, m_spriteScale, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosionGM, m_iRadiusOverride, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosionGM, m_iSpriteIndex, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvExplosionGM, CBaseMonster );
LINK_ENTITY_TO_CLASS( env_explosion_gm, CEnvExplosionGM );

void CEnvExplosionGM::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "iMagnitude" ) )
	{
		m_iMagnitude = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "iRadiusOverride" ) )
	{
		m_iRadiusOverride = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "sprite" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvExplosionGM::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;

	float flSpriteScale;
	flSpriteScale = ( m_iMagnitude - 50 ) * 0.6;

	if ( flSpriteScale < 10 )
	{
		flSpriteScale = 10;
	}

	m_spriteScale = (int)flSpriteScale;
}

void CEnvExplosionGM::Precache( void )
{
	if ( !FStringNull( pev->message ) )
		m_iSpriteIndex = PRECACHE_MODEL( (char *)STRING( pev->message ) );
	else
		m_iSpriteIndex = 0; // will use g_sModelIndexFireball
}

void CEnvExplosionGM::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;

	pev->model = iStringNull;
	pev->solid = SOLID_NOT;

	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );

	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT( pev ), &tr );

	if ( tr.flFraction != 1.0 )
	{
		pev->origin = tr.vecEndPos + ( tr.vecPlaneNormal * ( m_iMagnitude - 24 ) * 0.6 );
	}

	// Draw decal
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_GM_NODECAL ) )
	{
		if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
		}
		else
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
		}
	}

	// Draw fireball
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_GM_NOFIREBALL ) )
	{
		short spriteIndex = m_iSpriteIndex ? m_iSpriteIndex : g_sModelIndexFireball;

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( spriteIndex );
			WRITE_BYTE( (BYTE)m_spriteScale );
			WRITE_BYTE( 15 );
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 );
			WRITE_BYTE( 15 );
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}

	// Do damage with radius override support
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_GM_NODAMAGE ) )
	{
		if ( m_iRadiusOverride > 0 )
			::RadiusDamage( pev->origin, pev, pev, m_iMagnitude, (float)m_iRadiusOverride, CLASS_NONE, DMG_BLAST );
		else
			RadiusDamage( pev, pev, m_iMagnitude, CLASS_NONE, DMG_BLAST );
	}

	SetThink( &CEnvExplosionGM::Smoke );
	pev->nextthink = gpGlobals->time + 0.3;

	// Draw sparks
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_GM_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );

		for ( int i = 0; i < sparkCount; i++ )
		{
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
		}
	}
}

void CEnvExplosionGM::Smoke( void )
{
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_GM_NOSMOKE ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (BYTE)m_spriteScale );
			WRITE_BYTE( 12 );
		MESSAGE_END();
	}

	if ( !( pev->spawnflags & SF_ENVEXPLOSION_GM_REPEATABLE ) )
	{
		UTIL_Remove( this );
	}
}

// =========================================================
// gunman_explosion_small
//
// Programmatic small explosion entity (60 dmg / 256 u radius).
// Spawned by grenade cluster sub-bombs.  Immediately detonates
// on Spawn() and removes itself.
//
// Use CGunmanExplosionSmall::Create() to spawn it.
// =========================================================
class CGunmanExplosionSmall : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	static CGunmanExplosionSmall *Create( const Vector &origin, CBaseEntity *pOwner );
};

LINK_ENTITY_TO_CLASS( gunman_explosion_small, CGunmanExplosionSmall );

void CGunmanExplosionSmall::Precache( void )
{
	PRECACHE_SOUND( GMSND_KABOOM1 );
	PRECACHE_SOUND( GMSND_KABOOM2 );
	PRECACHE_SOUND( GMSND_KABOOM3 );
}

void CGunmanExplosionSmall::Spawn( void )
{
	Precache();

	pev->solid    = SOLID_NOT;
	pev->effects  = EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;

	// Pick a random kaboom sound
	const char *pszSounds[3] = { GMSND_KABOOM1, GMSND_KABOOM2, GMSND_KABOOM3 };
	EMIT_SOUND( ENT(pev), CHAN_AUTO,
		pszSounds[ RANDOM_LONG( 0, 2 ) ], 1.0f, ATTN_NORM );

	// Trace down for a decal
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT(pev), &tr );
	if ( tr.flFraction != 1.0f )
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );

	// Fireball visual
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 15 );  // scale
		WRITE_BYTE( 15 );  // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	// Damage
	::RadiusDamage( pev->origin, pev, pev,
		(float)GunmanEntityDamage::Explosions::Small.damage,
		(float)GunmanEntityDamage::Explosions::Small.radius,
		CLASS_NONE, DMG_BLAST );

	UTIL_Remove( this );
}

/*static*/ CGunmanExplosionSmall *CGunmanExplosionSmall::Create(
	const Vector &origin, CBaseEntity *pOwner )
{
	CGunmanExplosionSmall *p = GetClassPtr( (CGunmanExplosionSmall *)NULL );
	UTIL_SetOrigin( p->pev, origin );
	p->pev->owner = pOwner ? pOwner->edict() : NULL;
	p->Spawn();
	return p;
}

// =========================================================
// gunman_explosion_chem
//
// Chemical explosion entity (60 dmg / 256 u radius, scaled
// by pev->scale for larger chem-bomb bursts).
// Spawned by chemical bomb detonate when explode-flag is set.
//
// Use CGunmanExplosionChem::Create() to spawn it.
// The optional fScale parameter multiplies the blast radius.
// =========================================================
class CGunmanExplosionChem : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	static CGunmanExplosionChem *Create( const Vector &origin, CBaseEntity *pOwner,
		float fScale = 1.0f );
};

LINK_ENTITY_TO_CLASS( gunman_explosion_chem, CGunmanExplosionChem );

void CGunmanExplosionChem::Precache( void )
{
	PRECACHE_SOUND( GMSND_EXPLODE3 );
	PRECACHE_SOUND( GMSND_EXPLODE4 );
	PRECACHE_SOUND( GMSND_EXPLODE5 );
}

void CGunmanExplosionChem::Spawn( void )
{
	Precache();

	pev->solid    = SOLID_NOT;
	pev->effects  = EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;

	float fScale = ( pev->scale > 0.0f ) ? pev->scale : 1.0f;

	// Pick a random chemical explosion sound
	const char *pszSounds[3] = { GMSND_EXPLODE3, GMSND_EXPLODE4, GMSND_EXPLODE5 };
	EMIT_SOUND( ENT(pev), CHAN_AUTO,
		pszSounds[ RANDOM_LONG( 0, 2 ) ], 1.0f, ATTN_NORM );

	// Trace down for a decal
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT(pev), &tr );
	if ( tr.flFraction != 1.0f )
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );

	// Fireball visual (scaled)
	int iSprScale = (int)( 15.0f * fScale );
	if ( iSprScale < 5 ) iSprScale = 5;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( (BYTE)iSprScale );
		WRITE_BYTE( 15 );
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	// Damage (radius scaled)
	float flRadius = (float)GunmanEntityDamage::Explosions::Chem.radius * fScale;
	::RadiusDamage( pev->origin, pev, pev,
		(float)GunmanEntityDamage::Explosions::Chem.damage,
		flRadius,
		CLASS_NONE, DMG_BLAST );

	UTIL_Remove( this );
}

/*static*/ CGunmanExplosionChem *CGunmanExplosionChem::Create(
	const Vector &origin, CBaseEntity *pOwner, float fScale )
{
	CGunmanExplosionChem *p = GetClassPtr( (CGunmanExplosionChem *)NULL );
	UTIL_SetOrigin( p->pev, origin );
	p->pev->owner = pOwner ? pOwner->edict() : NULL;
	p->pev->scale = fScale;
	p->Spawn();
	return p;
}

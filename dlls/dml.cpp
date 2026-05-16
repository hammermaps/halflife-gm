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
// weapon_dml / CDML  -- Gunman Chronicles Dual Missile Launcher
//
// 4-axis customisation menu ported from:
//   game/lua/weapons/gunman_weapon_mule/shared.lua
//   game/lua/gunman_data.lua  (damage / ammo-usage tables)
//
//   LaunchType    (1-2) : 1=WhenFired, 2=WhenTargeted  (default 1)
//   FlightPathType(1-3) : 1=Guided, 2=Homing, 3=Spiral (default 1)
//   DetonationType(1-3) : 1=OnImpact, 2=Timed, 3=TripMine (default 1)
//   PayloadType   (1-2) : 1=Explosive, 2=Cluster        (default 1)
//
// SecondaryAttack cycles LaunchType -> FlightPath -> Detonation -> Payload
// advancing the current axis and playing the Customize anim.
//
// PrimaryAttack:
//   LaunchType=2 (WhenTargeted): requires a homing target in the forward
//   cone before firing; plays DryFire if none found.
//   FlightPathType=3 (Spiral): consumes 2 ammo, launches two missiles.
//   All other modes: consumes 1 ammo, launches one missile.
//
// Reload: dual (DML_RELOADBOTH, 2 ammo) when clip==0 and ammo>=2;
//         single (DML_RELOADLEFT / DML_RELOADRIGHT alternating, 1 ammo)
//         when clip < DML_MAX_CLIP and ammo >= 1.
//
// CDMLMissile (dml_missile) -- projectile
//   Flight modes: Guided (tracks player aim), Homing (tracks nearest
//   monster in forward cone), Spiral (sinusoidal offset around forward).
//   Detonation modes: OnImpact (explode on touch), Timed (detonate after
//   DML_TIMED_FUSE seconds), TripMine (land, arm, detonate on proximity).
//   Payload modes: Explosive (single RadiusDamage), Cluster (spawn
//   DML_CLUSTER_COUNT sub-bombs that scatter and explode).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

// -----------------------------------------------------------------------
// Damage / ammo constants  (game/lua/gunman_data.lua)
// -----------------------------------------------------------------------
#define DML_DMG_DIRECT      8       // WeaponDamage.Mule (direct touch dmg)
#define DML_DMG_BLAST       100     // radius damage at detonation
#define DML_DMG_RADIUS      150.0f  // blast radius

// Ammo-usage table (WeaponAmmoUsage.Mule)
#define DML_AMMO_STANDARD   1       // LaunchStandard: 1 ammo per shot
#define DML_AMMO_SPIRAL     2       // LaunchSpiral: 2 ammo (fires 2 rockets)
#define DML_AMMO_RELOAD_SINGLE 1    // ReloadSingle
#define DML_AMMO_RELOAD_DOUBLE 2    // ReloadDouble

// Default axis settings  (DefaultWeaponSettings.Mule)
#define DML_DEFAULT_LAUNCHTYPE    1
#define DML_DEFAULT_FLIGHTPATH    1
#define DML_DEFAULT_DETONATION    1
#define DML_DEFAULT_PAYLOAD       1

// Missile flight
#define DML_MISSILE_SPEED   800.0f  // u/s forward speed
#define DML_THINK_RATE      0.1f    // guidance update interval

// Timed detonation fuse
#define DML_TIMED_FUSE      20.0f   // seconds until auto-detonate (Timed mode)

// TripMine: arm delay and proximity radius
#define DML_TRIPMINE_ARM_DELAY  0.8f
#define DML_TRIPMINE_RADIUS     64.0f

// Cluster sub-bomb count and properties
#define DML_CLUSTER_COUNT   5
#define DML_CLUSTER_SPEED   400.0f
#define DML_CLUSTER_DMG     40
#define DML_CLUSTER_RADIUS  100.0f
#define DML_CLUSTER_FUSE    2.0f

// Homing: cone half-angle for target search (dot product threshold)
#define DML_HOMING_DOT      0.92f   // ~23° half-angle
#define DML_HOMING_RANGE    4096.0f

// Fire delays
#define DML_FIRE_DELAY      0.75f
#define DML_RELOAD_BOTH_TIME 2.5f   // approx reloadboth anim length + delay
#define DML_RELOAD_SINGLE_TIME 1.5f // approx reloadleft/right anim length + delay

// Punch angle on fire
#define DML_PUNCH_X         -5.0f

// -----------------------------------------------------------------------
// Animation indices  (model: models/v_dml.mdl)
// -----------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( weapon_dml, CDML );

// enum is already in the file — keep the same indices
enum dml_anim_e
{
	DML_IDLE		= 0,
	DML_IDLEFIDGET,
	DML_RELOADBOTH,
	DML_RELOADLEFT,
	DML_RELOADRIGHT,
	DML_FIRE,
	DML_CUSTOMIZE,
	DML_DRAW
};

#define DML_HOLSTER DML_DRAW

// -----------------------------------------------------------------------
// CDMLClusterBomb -- sub-projectile spawned by Cluster payload
//
// Bounces for DML_CLUSTER_FUSE seconds then detonates with a small blast.
// -----------------------------------------------------------------------
class CDMLClusterBomb : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT BombTouch( CBaseEntity *pOther );
	void EXPORT BombThink( void );

	static CDMLClusterBomb *Create( const Vector &vecOrigin,
		const Vector &vecVelocity, CBaseEntity *pOwner );

	float m_flDetonateTime;
	BOOL  m_bDetonated;
};

LINK_ENTITY_TO_CLASS( dml_clusterbomb, CDMLClusterBomb );

void CDMLClusterBomb::Spawn( void )
{
	pev->movetype  = MOVETYPE_BOUNCE;
	pev->solid     = SOLID_BBOX;
	pev->classname = MAKE_STRING( "dml_clusterbomb" );
	pev->gravity   = 1.0f;
	pev->friction  = 0.6f;

	SET_MODEL( ENT(pev), "models/dmlcluster.mdl" );
	UTIL_SetSize( pev, Vector(-4,-4,-4), Vector(4,4,4) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CDMLClusterBomb::BombTouch );
	SetThink( &CDMLClusterBomb::BombThink );
	pev->nextthink = gpGlobals->time + 0.05f;

	m_bDetonated = FALSE;
}

CDMLClusterBomb *CDMLClusterBomb::Create( const Vector &vecOrigin,
	const Vector &vecVelocity, CBaseEntity *pOwner )
{
	CDMLClusterBomb *p = GetClassPtr( (CDMLClusterBomb *)NULL );
	UTIL_SetOrigin( p->pev, vecOrigin );
	p->pev->velocity = vecVelocity;
	p->pev->owner    = pOwner->edict();
	p->m_flDetonateTime = gpGlobals->time + DML_CLUSTER_FUSE;
	p->Spawn();
	return p;
}

void CDMLClusterBomb::BombTouch( CBaseEntity *pOther )
{
	if ( m_bDetonated ) return;
	if ( pOther && pOther->edict() == pev->owner ) return;

	// Detonate immediately on entity contact (not world)
	if ( pOther && !FClassnameIs( pOther->pev, "worldspawn" )
		&& pOther->pev->solid != SOLID_BSP )
	{
		BombThink();
	}
}

void CDMLClusterBomb::BombThink( void )
{
	if ( m_bDetonated ) return;

	if ( gpGlobals->time >= m_flDetonateTime )
	{
		m_bDetonated = TRUE;
		entvars_t *pevAttacker = pev->owner ? VARS(pev->owner) : pev;
		::RadiusDamage( pev->origin, pev, pevAttacker,
			(float)DML_CLUSTER_DMG, DML_CLUSTER_RADIUS, CLASS_NONE, DMG_BLAST );

		// visual effect
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 10 );    // scale * 10
			WRITE_BYTE( 15 );    // framerate
			WRITE_BYTE( TE_EXPLFLAG_NOPARTICLES );
		MESSAGE_END();

		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.05f;
}

// -----------------------------------------------------------------------
// CDMLMissile -- DML rocket projectile
//
// Created by CDML::LaunchMissile(); pev->owner is the firing player.
// Flight-path, detonation and payload behaviour are set before Spawn().
// -----------------------------------------------------------------------
class CDMLMissile : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT MissileTouch( CBaseEntity *pOther );
	void EXPORT MissileThink( void );
	void Detonate( void );

	static CDMLMissile *Create( const Vector &vecOrigin,
		const Vector &vecAngles, CBaseEntity *pOwner,
		int iFlightPath, int iDetonation, int iPayload,
		BOOL bSpiralInvert, CBaseEntity *pHomingTarget );

	// Behaviour flags (set before Spawn)
	int   m_iFlightPathType;   // 1=Guided, 2=Homing, 3=Spiral
	int   m_iDetonationType;   // 1=OnImpact, 2=Timed, 3=TripMine
	int   m_iPayloadType;      // 1=Explosive, 2=Cluster

	BOOL  m_bSpiralInvert;     // reverse spiral rotation
	EHANDLE m_hHomingTarget;   // entity to track (Homing mode)

	float m_flLifeTime;        // gpGlobals->time when spawned
	float m_flSpiralPhase;     // random phase offset for spiral

	BOOL  m_bCollided;         // TRUE once the missile has hit something
	BOOL  m_bArmed;            // TRUE when TripMine is armed
	BOOL  m_bDetonated;        // guard against re-entrant Detonate()
};

LINK_ENTITY_TO_CLASS( dml_missile, CDMLMissile );

CDMLMissile *CDMLMissile::Create( const Vector &vecOrigin,
	const Vector &vecAngles, CBaseEntity *pOwner,
	int iFlightPath, int iDetonation, int iPayload,
	BOOL bSpiralInvert, CBaseEntity *pHomingTarget )
{
	CDMLMissile *p = GetClassPtr( (CDMLMissile *)NULL );
	UTIL_SetOrigin( p->pev, vecOrigin );
	p->pev->angles  = vecAngles;
	p->pev->owner   = pOwner->edict();

	p->m_iFlightPathType = iFlightPath;
	p->m_iDetonationType = iDetonation;
	p->m_iPayloadType    = iPayload;
	p->m_bSpiralInvert   = bSpiralInvert;
	p->m_hHomingTarget   = pHomingTarget;
	p->m_flSpiralPhase   = RANDOM_FLOAT( 0.0f, 6.28f );

	p->Spawn();
	return p;
}

void CDMLMissile::Spawn( void )
{
	pev->movetype  = MOVETYPE_FLY;
	pev->solid     = SOLID_BBOX;
	pev->classname = MAKE_STRING( "dml_missile" );
	pev->effects   |= EF_LIGHT;

	SET_MODEL( ENT(pev), "models/dmlrocket.mdl" );
	UTIL_SetSize( pev, Vector(0,0,0), Vector(0,0,0) );
	UTIL_SetOrigin( pev, pev->origin );

	// Align angles then fire forward
	UTIL_MakeVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * DML_MISSILE_SPEED;

	SetTouch( &CDMLMissile::MissileTouch );
	SetThink( &CDMLMissile::MissileThink );
	pev->nextthink = gpGlobals->time + DML_THINK_RATE;

	m_flLifeTime  = gpGlobals->time;
	m_bCollided   = FALSE;
	m_bArmed      = FALSE;
	m_bDetonated  = FALSE;

	// smoke + flame trail
	static int iSmokeSprite = 0;
	if ( !iSmokeSprite )
		iSmokeSprite = PRECACHE_MODEL( "sprites/smoke.spr" );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( iSmokeSprite );
		WRITE_BYTE( 30 );   // life * 0.1
		WRITE_BYTE( 4 );    // width
		WRITE_BYTE( 224 );  // r
		WRITE_BYTE( 200 );  // g
		WRITE_BYTE( 100 );  // b
		WRITE_BYTE( 200 );  // brightness
	MESSAGE_END();

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1.0f, 0.5f );
}

void CDMLMissile::MissileTouch( CBaseEntity *pOther )
{
	if ( m_bDetonated ) return;
	if ( pOther && pOther->edict() == pev->owner ) return;

	STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
	pev->effects &= ~EF_LIGHT;

	if ( m_iDetonationType == 3 )
	{
		// TripMine: land on surfaces only, stick and arm
		BOOL bSurface = !pOther
			|| FClassnameIs( pOther->pev, "worldspawn" )
			|| ( pOther->pev->solid == SOLID_BSP );

		if ( bSurface )
		{
			m_bCollided   = TRUE;
			pev->movetype = MOVETYPE_NONE;
			pev->velocity = g_vecZero;
			pev->effects  = 0;
			pev->nextthink = gpGlobals->time + DML_TRIPMINE_ARM_DELAY;
			SetThink( &CDMLMissile::MissileThink );
			return;
		}
		// Hit an entity directly — detonate
		Detonate();
	}
	else if ( m_iDetonationType == 1 )
	{
		// OnImpact: detonate immediately
		Detonate();
	}
	else
	{
		// Timed: land and wait for fuse
		m_bCollided   = TRUE;
		pev->movetype = MOVETYPE_NONE;
		pev->velocity = g_vecZero;
		pev->effects  = 0;
	}
}

void CDMLMissile::MissileThink( void )
{
	if ( m_bDetonated ) return;

	// ----- TripMine arm state -----
	if ( m_iDetonationType == 3 && m_bCollided && !m_bArmed )
	{
		m_bArmed = TRUE;
		pev->nextthink = gpGlobals->time + DML_THINK_RATE;
		return;
	}

	if ( m_iDetonationType == 3 && m_bCollided && m_bArmed )
	{
		// Scan for nearby entities within trip radius
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, DML_TRIPMINE_RADIUS ) ) != NULL )
		{
			if ( pEnt == this ) continue;
			if ( pEnt->edict() == pev->owner ) continue;
			if ( !pEnt->IsAlive() ) continue;
			Detonate();
			return;
		}
		pev->nextthink = gpGlobals->time + DML_THINK_RATE;
		return;
	}

	// ----- Timed auto-fuse -----
	if ( m_iDetonationType == 2 )
	{
		if ( gpGlobals->time - m_flLifeTime >= DML_TIMED_FUSE )
		{
			Detonate();
			return;
		}
	}

	// ----- Flight path guidance (only while in flight) -----
	if ( !m_bCollided )
	{
		UTIL_MakeAimVectors( pev->angles );

		if ( m_iFlightPathType == 1 )
		{
			// Guided: steer toward player aim point
			CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
			if ( pOwner && pOwner->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer*)pOwner;
				UTIL_MakeVectors( pPlayer->pev->v_angle );
				Vector vecTarget = gpGlobals->v_forward;

				// Soft steering: blend current direction toward aim
				float flSpeed = pev->velocity.Length();
				pev->velocity = pev->velocity * 0.8f + vecTarget * ( flSpeed * 0.2f + 50.0f );
				if ( pev->velocity.Length() > DML_MISSILE_SPEED )
					pev->velocity = pev->velocity.Normalize() * DML_MISSILE_SPEED;
				pev->angles = UTIL_VecToAngles( pev->velocity );
			}
		}
		else if ( m_iFlightPathType == 2 )
		{
			// Homing: steer toward locked target
			CBaseEntity *pTarget = (CBaseEntity *)(CBaseEntity *)m_hHomingTarget;
			if ( pTarget && pTarget->IsAlive() )
			{
				Vector vecDir = ( pTarget->Center() - pev->origin ).Normalize();
				float flSpeed = pev->velocity.Length();
				pev->velocity = pev->velocity * 0.8f + vecDir * ( flSpeed * 0.2f + 50.0f );
				if ( pev->velocity.Length() > DML_MISSILE_SPEED )
					pev->velocity = pev->velocity.Normalize() * DML_MISSILE_SPEED;
				pev->angles = UTIL_VecToAngles( pev->velocity );
			}
			else
			{
				// No target — fly straight
				pev->velocity = gpGlobals->v_forward * DML_MISSILE_SPEED;
			}
		}
		else if ( m_iFlightPathType == 3 )
		{
			// Spiral: sinusoidal offset around the forward axis
			UTIL_MakeVectors( pev->angles );
			float t     = gpGlobals->time * 8.0f + m_flSpiralPhase;
			float dist  = ( pev->origin - pev->origin ).Length(); // travel dist approx
			float frac  = 1.0f; // full amplitude throughout (Lua uses fraction vs 4096)

			Vector vecUp    = gpGlobals->v_up;
			Vector vecRight = CrossProduct( gpGlobals->v_forward, vecUp );

			float sComp = (float)sin( t ) * 150.0f * frac;
			float cComp = (float)cos( t ) * 150.0f * frac;

			if ( m_bSpiralInvert )
				cComp = -cComp;

			Vector vecFwdTarget = gpGlobals->v_forward * DML_MISSILE_SPEED
				+ vecUp * sComp + vecRight * cComp;

			pev->velocity = vecFwdTarget.Normalize() * DML_MISSILE_SPEED;
			pev->angles   = UTIL_VecToAngles( pev->velocity );
		}

		// Auto-detonate if in sky
		if ( UTIL_PointContents( pev->origin ) == CONTENTS_SKY )
		{
			Detonate();
			return;
		}

		// Timed mode auto-detonation in-flight
		if ( m_iDetonationType == 2 && gpGlobals->time - m_flLifeTime >= DML_TIMED_FUSE )
		{
			Detonate();
			return;
		}
	}

	pev->nextthink = gpGlobals->time + DML_THINK_RATE;
}

void CDMLMissile::Detonate( void )
{
	if ( m_bDetonated ) return;
	m_bDetonated = TRUE;

	STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav" );
	pev->effects  = 0;
	pev->solid    = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	entvars_t *pevAttacker = pev->owner ? VARS(pev->owner) : pev;

	// Explosion visual
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 20 );    // scale * 10
		WRITE_BYTE( 15 );    // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	if ( m_iPayloadType == 2 )
	{
		// Cluster: scatter DML_CLUSTER_COUNT sub-bombs
		for ( int i = 0; i < DML_CLUSTER_COUNT; i++ )
		{
			Vector vecSpread(
				RANDOM_FLOAT( -1.0f, 1.0f ),
				RANDOM_FLOAT( -1.0f, 1.0f ),
				RANDOM_FLOAT(  0.3f, 1.0f ) );
			vecSpread = vecSpread.Normalize();

			CDMLClusterBomb::Create(
				pev->origin + vecSpread * 8.0f,
				vecSpread * DML_CLUSTER_SPEED,
				CBaseEntity::Instance( pev->owner ) );
		}
	}
	else
	{
		// Explosive: single large blast
		::RadiusDamage( pev->origin, pev, pevAttacker,
			(float)DML_DMG_BLAST, DML_DMG_RADIUS, CLASS_NONE, DMG_BLAST );
	}

	UTIL_Remove( this );
}

// -----------------------------------------------------------------------
// CDML weapon
// -----------------------------------------------------------------------

void CDML::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DML;
	SET_MODEL(ENT(pev), "models/w_dml.mdl");

	m_iDefaultAmmo = DML_DEFAULT_GIVE;

	// Initialise menu axes to Lua defaults
	m_iLaunchType    = DML_DEFAULT_LAUNCHTYPE;
	m_iFlightPathType= DML_DEFAULT_FLIGHTPATH;
	m_iDetonationType= DML_DEFAULT_DETONATION;
	m_iPayloadType   = DML_DEFAULT_PAYLOAD;
	m_iMenuAxis      = 1;
	m_iReloadFlipFlop= 0;
	m_iReloadState   = 0;
	m_flReloadCompleteTime = 0;

	FallInit();
}


void CDML::Precache( void )
{
	PRECACHE_MODEL("models/v_dml.mdl");
	PRECACHE_MODEL("models/w_dml.mdl");
	PRECACHE_MODEL("models/p_crossbow.mdl");
	PRECACHE_MODEL("models/dmlrocket.mdl");
	PRECACHE_MODEL("models/dmlcluster.mdl");
	PRECACHE_MODEL("sprites/smoke.spr");

	PRECACHE_SOUND("gunmanchronicles/weapons/dml_fire.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/dml_reload.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/dml_dualreload.wav");
	PRECACHE_SOUND("gunmanchronicles/weapons/DryFire.wav");
	PRECACHE_SOUND("weapons/rocket1.wav");

	m_usDML = PRECACHE_EVENT( 1, "events/rpg.sc" );
}

int CDML::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "dml_ammo";
	p->iMaxAmmo1 = DML_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DML_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DML;
	p->iWeight = DML_WEIGHT;

	return 1;
}

int CDML::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CDML::Deploy( )
{
	m_iReloadState = 0;
	m_flReloadCompleteTime = 0;
	return DefaultDeploy( "models/v_dml.mdl", "models/p_crossbow.mdl", DML_DRAW, "rpg" );
}

// -----------------------------------------------------------------------
// SecondaryAttack -- cycle the 4-axis customisation menu
//
// Each press advances the value of the current axis, then moves to the
// next axis when the axis rolls over.  Plays the DML_CUSTOMIZE anim and
// DryFire sound so the player gets feedback.
// -----------------------------------------------------------------------
void CDML::SecondaryAttack( void )
{
	// Advance the currently selected axis
	switch ( m_iMenuAxis )
	{
	case 1: // LaunchType 1-2
		m_iLaunchType++;
		if ( m_iLaunchType > 2 ) { m_iLaunchType = 1; m_iMenuAxis = 2; }
		break;
	case 2: // FlightPathType 1-3
		m_iFlightPathType++;
		if ( m_iFlightPathType > 3 ) { m_iFlightPathType = 1; m_iMenuAxis = 3; }
		break;
	case 3: // DetonationType 1-3
		m_iDetonationType++;
		if ( m_iDetonationType > 3 ) { m_iDetonationType = 1; m_iMenuAxis = 4; }
		break;
	case 4: // PayloadType 1-2
		m_iPayloadType++;
		if ( m_iPayloadType > 2 ) { m_iPayloadType = 1; m_iMenuAxis = 1; }
		break;
	default:
		m_iMenuAxis = 1;
		break;
	}

	SendWeaponAnim( DML_CUSTOMIZE );
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
		"gunmanchronicles/weapons/DryFire.wav", 0.8f, ATTN_NORM );

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
}

// -----------------------------------------------------------------------
// Helper: find the nearest live monster/player in the player's forward
// cone within DML_HOMING_RANGE units (used by Homing flight path and
// LaunchType=2 target-required check).
// -----------------------------------------------------------------------
static CBaseEntity *DML_FindHomingTarget( CBasePlayer *pPlayer )
{
	UTIL_MakeVectors( pPlayer->pev->v_angle );
	Vector vecFwd = gpGlobals->v_forward;

	CBaseEntity *pBest = NULL;
	float flBestDist = DML_HOMING_RANGE;

	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityByClassname( pEnt, "monster_*" ) ) != NULL )
	{
		if ( !pEnt->IsAlive() ) continue;

		Vector vecToEnt = ( pEnt->Center() - pPlayer->GetGunPosition() );
		float flDist = vecToEnt.Length();
		if ( flDist > DML_HOMING_RANGE ) continue;

		// Must be within cone
		float flDot = DotProduct( vecFwd, vecToEnt.Normalize() );
		if ( flDot < DML_HOMING_DOT ) continue;

		// Line-of-sight check
		TraceResult tr;
		UTIL_TraceLine( pPlayer->GetGunPosition(), pEnt->Center(),
			dont_ignore_monsters, ENT(pPlayer->pev), &tr );
		if ( tr.flFraction < 0.99f ) continue;

		if ( flDist < flBestDist )
		{
			flBestDist = flDist;
			pBest = pEnt;
		}
	}
	return pBest;
}

// -----------------------------------------------------------------------
// LaunchMissile -- spawn one CDMLMissile from the player's gun position
// -----------------------------------------------------------------------
static void DML_LaunchMissile( CBasePlayer *pPlayer, CDML *pWeapon,
	BOOL bSpiralInvert, CBaseEntity *pHomingTarget )
{
	UTIL_MakeVectors( pPlayer->pev->v_angle );
	Vector vecSrc = pPlayer->GetGunPosition()
		+ gpGlobals->v_forward * 16.0f
		+ gpGlobals->v_right   * ( bSpiralInvert ? -8.0f : 8.0f )
		+ gpGlobals->v_up      * -8.0f;

	CDMLMissile::Create( vecSrc, pPlayer->pev->v_angle, pPlayer,
		pWeapon->m_iFlightPathType, pWeapon->m_iDetonationType,
		pWeapon->m_iPayloadType, bSpiralInvert, pHomingTarget );
}

// -----------------------------------------------------------------------
// PrimaryAttack
// -----------------------------------------------------------------------
void CDML::PrimaryAttack( void )
{
	// Block during reload
	if ( m_iReloadState == 1 )
		return;

	// Dry-fire check
	if ( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	// Spiral needs 2 loaded
	if ( m_iFlightPathType == 3 && m_iClip < DML_AMMO_SPIRAL )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/DryFire.wav", 0.8f, ATTN_NORM );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}

	// LaunchType=2 (WhenTargeted): need a homing target
	CBaseEntity *pHomingTarget = NULL;
	if ( m_iFlightPathType == 2 )
	{
		pHomingTarget = DML_FindHomingTarget( m_pPlayer );
	}

	if ( m_iLaunchType == 2 && pHomingTarget == NULL )
	{
		// No target yet — indicate with DryFire
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/DryFire.wav", 0.8f, ATTN_NORM );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash  = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->pev->punchangle.x = DML_PUNCH_X;

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON,
		"gunmanchronicles/weapons/dml_fire.wav", 1.0f, ATTN_NORM );
	SendWeaponAnim( DML_FIRE );

	if ( m_iFlightPathType == 3 )
	{
		// Spiral: fire two rockets simultaneously
		DML_LaunchMissile( m_pPlayer, this, FALSE, pHomingTarget );
		DML_LaunchMissile( m_pPlayer, this, TRUE,  pHomingTarget );
		m_iClip -= DML_AMMO_SPIRAL;
	}
	else
	{
		DML_LaunchMissile( m_pPlayer, this, FALSE, pHomingTarget );
		m_iClip -= DML_AMMO_STANDARD;
	}

	// Begin reload immediately after fire
	m_iReloadState = 1;

	float flReloadTime = ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= DML_AMMO_RELOAD_DOUBLE )
		? DML_RELOAD_BOTH_TIME : DML_RELOAD_SINGLE_TIME;

	m_flReloadCompleteTime = UTIL_WeaponTimeBase() + flReloadTime;
	m_flNextPrimaryAttack  = UTIL_WeaponTimeBase() + flReloadTime;
	m_flNextSecondaryAttack= UTIL_WeaponTimeBase() + flReloadTime;
	m_flTimeWeaponIdle     = UTIL_WeaponTimeBase() + flReloadTime + 0.5f;

	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= DML_AMMO_RELOAD_DOUBLE )
	{
		SendWeaponAnim( DML_RELOADBOTH );
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/dml_dualreload.wav", 1.0f, ATTN_NORM );
	}
	else if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= DML_AMMO_RELOAD_SINGLE )
	{
		int iReloadAnim = m_iReloadFlipFlop ? DML_RELOADRIGHT : DML_RELOADLEFT;
		m_iReloadFlipFlop = !m_iReloadFlipFlop;
		SendWeaponAnim( iReloadAnim );
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/dml_reload.wav", 1.0f, ATTN_NORM );
	}

	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
}


void CDML::Reload( void )
{
	// The DML auto-reloads immediately after each shot (handled in
	// PrimaryAttack).  Reload() is still provided so that the HUD /
	// engine can call it safely, but it is a no-op while reloading.
	if ( m_iReloadState != 0 )
		return;

	if ( m_iClip >= DML_MAX_CLIP )
		return;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	// Kick off a manual reload (e.g. player pressed R)
	m_iReloadState = 1;
	float flReloadTime;

	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= DML_AMMO_RELOAD_DOUBLE )
	{
		SendWeaponAnim( DML_RELOADBOTH );
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/dml_dualreload.wav", 1.0f, ATTN_NORM );
		flReloadTime = DML_RELOAD_BOTH_TIME;
	}
	else
	{
		int iReloadAnim = m_iReloadFlipFlop ? DML_RELOADRIGHT : DML_RELOADLEFT;
		m_iReloadFlipFlop = !m_iReloadFlipFlop;
		SendWeaponAnim( iReloadAnim );
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"gunmanchronicles/weapons/dml_reload.wav", 1.0f, ATTN_NORM );
		flReloadTime = DML_RELOAD_SINGLE_TIME;
	}

	m_flReloadCompleteTime  = UTIL_WeaponTimeBase() + flReloadTime;
	m_flNextPrimaryAttack   = UTIL_WeaponTimeBase() + flReloadTime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flReloadTime;
	m_flTimeWeaponIdle      = UTIL_WeaponTimeBase() + flReloadTime + 0.5f;
}

// -----------------------------------------------------------------------
// WeaponIdle -- complete pending reload and play idle animations
// -----------------------------------------------------------------------
void CDML::WeaponIdle( void )
{
	ResetEmptySound();

	// ----- Complete reload when timer expires -----
	if ( m_iReloadState == 1 && UTIL_WeaponTimeBase() >= m_flReloadCompleteTime )
	{
		m_iReloadState = 0;

		int iAmmo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];

		if ( m_iClip == 0 && iAmmo >= DML_AMMO_RELOAD_DOUBLE )
		{
			// Fill both barrels
			m_iClip = DML_MAX_CLIP;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= DML_AMMO_RELOAD_DOUBLE;
		}
		else if ( m_iClip < DML_MAX_CLIP && iAmmo >= DML_AMMO_RELOAD_SINGLE )
		{
			// Single barrel
			m_iClip++;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= DML_AMMO_RELOAD_SINGLE;
		}
	}

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// Idle animation selection
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	int iAnim;

	if ( flRand <= 0.6f )
	{
		iAnim = DML_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
	}
	else
	{
		iAnim = DML_IDLEFIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
	}
	SendWeaponAnim( iAnim, 1 );
}


void CDML::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( DML_HOLSTER );
}


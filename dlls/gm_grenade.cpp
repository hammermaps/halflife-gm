/***
*
*Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*This product contains software technology licensed from Id
*Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// weapon_gcgrenade / CGCGrenade  -- Gunman Chronicles Grenade Core
//
// Ported from:
//   game/lua/weapons/gunman_weapon_grenade/shared.lua
//   game/lua/entities/gunman_weapon_grenade_base.lua
//   game/lua/entities/gunman_weapon_grenade_armed.lua
//   game/lua/entities/gunman_weapon_grenade_cluster.lua
//   game/lua/entities/gunman_weapon_grenade_tripmine.lua
//   game/lua/gunman_data.lua  (damage / ammo-usage tables)
//
// 2-axis customisation menu:
//   Axis 1 — DetonationType : 1=Normal(3 s fuse), 2=OnImpact, 3=TripMine
//   Axis 2 — PayloadType    : 1=Standard, 2=Cluster
//
// SecondaryAttack cycles DetonationType axis, then PayloadType axis.
// PrimaryAttack throws (DetonationType 1 or 2) or places (DetonationType 3).
// One dml_ammo is consumed per throw/placement.
//
// Projectile entities (registered classnames):
//   gunman_weapon_grenade_armed    — thrown grenade
//   gunman_weapon_grenade_cluster  — sub-bomb spawned by Cluster payload
//   gunman_weapon_grenade_tripmine — surface-placed tripmine
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
#include "decals.h"
#include "explode.h"
#include "gunman_damage.h"
#include "gunman_sounds.h"

// -----------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------

#define GCG_DEFAULT_DETONATION  2    // OnImpact
#define GCG_DEFAULT_PAYLOAD     1    // Standard

#define GCG_FUSE_TIME           3.0f // armed grenade timed fuse (seconds)

#define GCG_CLUSTER_COUNT       6
#define GCG_CLUSTER_FUSE_MIN    1.0f
#define GCG_CLUSTER_FUSE_MAX    2.5f
#define GCG_CLUSTER_LAUNCH_MIN  250.0f
#define GCG_CLUSTER_LAUNCH_MAX  400.0f

#define GCG_TRIPMINE_ARM_DELAY  3.0f
#define GCG_TRIPMINE_TRACE_LEN  1024.0f

// Explosion blast (WeaponDamage.Grenade / EntityDamage.Explosions.Standard)
#define GCG_BLAST_DMG           ( (float)GunmanDamage::Grenade )
#define GCG_BLAST_RADIUS        128.0f

// Animation indices (v_grenadecore.mdl)
enum gcgrenade_e
{
GCGRENADE_IDLE = 0,
GCGRENADE_FIDGET,
GCGRENADE_PINPULL,
GCGRENADE_THROW1,
GCGRENADE_THROW2,
GCGRENADE_THROW3,
GCGRENADE_HOLSTER,
GCGRENADE_DRAW,
GCGRENADE_TRIPMINE,
GCGRENADE_CUSTOMIZE
};

// -----------------------------------------------------------------------
// CGCGrenadeCluster  (gunman_weapon_grenade_cluster)
//
// Sub-bomb spawned by the Cluster payload.  Bounces for a short random
// fuse, then triggers a small explosion (EntityDamage.Explosions.Small).
// Defined before CGCGrenadeArmed so that Cluster() can reference it.
// -----------------------------------------------------------------------
class CGCGrenadeCluster : public CBaseEntity
{
public:
void Spawn( void );
void Precache( void );
void EXPORT ClusterTouch( CBaseEntity *pOther );
void EXPORT ClusterThink( void );

float m_flDetonateTime;

virtual int Save( CSave &save );
virtual int Restore( CRestore &restore );
static TYPEDESCRIPTION m_SaveData[];
};

TYPEDESCRIPTION CGCGrenadeCluster::m_SaveData[] =
{
DEFINE_FIELD( CGCGrenadeCluster, m_flDetonateTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CGCGrenadeCluster, CBaseEntity );
LINK_ENTITY_TO_CLASS( gunman_weapon_grenade_cluster, CGCGrenadeCluster );

void CGCGrenadeCluster::Precache( void )
{
PRECACHE_MODEL( "models/gunman/dmlcluster.mdl" );
PRECACHE_SOUND( GMSND_GRENADE_HIT1 );
PRECACHE_SOUND( GMSND_GRENADE_HIT2 );
PRECACHE_SOUND( GMSND_GRENADE_HIT3 );
PRECACHE_SOUND( GMSND_KABOOM1 );
PRECACHE_SOUND( GMSND_KABOOM2 );
PRECACHE_SOUND( GMSND_KABOOM3 );
}

void CGCGrenadeCluster::Spawn( void )
{
Precache();

pev->movetype = MOVETYPE_BOUNCE;
pev->solid    = SOLID_BBOX;
pev->gravity  = 1.0f;
pev->friction = 0.6f;

SET_MODEL( ENT(pev), "models/gunman/dmlcluster.mdl" );
UTIL_SetSize( pev, Vector(-4,-4,-4), Vector(4,4,4) );
UTIL_SetOrigin( pev, pev->origin );

SetTouch( &CGCGrenadeCluster::ClusterTouch );
SetThink( &CGCGrenadeCluster::ClusterThink );
pev->nextthink = gpGlobals->time + 0.1f;
}

void CGCGrenadeCluster::ClusterTouch( CBaseEntity *pOther )
{
if ( pev->velocity.Length() > 100.0f )
{
const char *pszSounds[3] = { GMSND_GRENADE_HIT1, GMSND_GRENADE_HIT2, GMSND_GRENADE_HIT3 };
EMIT_SOUND( ENT(pev), CHAN_ITEM,
pszSounds[ RANDOM_LONG(0,2) ], 0.5f, ATTN_NORM );
}
}

void CGCGrenadeCluster::ClusterThink( void )
{
if ( gpGlobals->time < m_flDetonateTime )
{
pev->nextthink = gpGlobals->time + 0.1f;
return;
}

SetTouch( NULL );
SetThink( NULL );

// Explosion visual
MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
WRITE_BYTE( TE_EXPLOSION );
WRITE_COORD( pev->origin.x );
WRITE_COORD( pev->origin.y );
WRITE_COORD( pev->origin.z );
WRITE_SHORT( g_sModelIndexFireball );
WRITE_BYTE( 10 );
WRITE_BYTE( 15 );
WRITE_BYTE( TE_EXPLFLAG_NONE );
MESSAGE_END();

const char *pszSounds[3] = { GMSND_KABOOM1, GMSND_KABOOM2, GMSND_KABOOM3 };
EMIT_SOUND( ENT(pev), CHAN_AUTO,
pszSounds[ RANDOM_LONG(0,2) ], 0.9f, ATTN_NORM );

::RadiusDamage( pev->origin, pev, pev,
(float)GunmanEntityDamage::Explosions::Small.damage,
(float)GunmanEntityDamage::Explosions::Small.radius,
CLASS_NONE, DMG_BLAST );

UTIL_Remove( this );
}

// -----------------------------------------------------------------------
// Helper: spawn GCG_CLUSTER_COUNT cluster sub-bombs from origin with owner
// -----------------------------------------------------------------------
static void GCG_SpawnClusters( Vector vecOrigin, EOFFSET hOwner )
{
EMIT_SOUND( INDEXENT(hOwner), CHAN_ITEM, GMSND_DML_FRAGMENT, 1.0f, ATTN_NORM );

for ( int i = 0; i < GCG_CLUSTER_COUNT; i++ )
{
float angle = ( (float)i / (float)GCG_CLUSTER_COUNT ) * M_PI * 2.0f;
float x = (float)cos( angle );
float y = (float)sin( angle );

Vector vecDir( x * RANDOM_FLOAT(1.0f, 2.0f),
               y * RANDOM_FLOAT(1.0f, 2.0f),
               1.0f );
vecDir = vecDir.Normalize();

float fSpeed = RANDOM_FLOAT( GCG_CLUSTER_LAUNCH_MIN, GCG_CLUSTER_LAUNCH_MAX );

CGCGrenadeCluster *pCluster = GetClassPtr( (CGCGrenadeCluster *)NULL );
UTIL_SetOrigin( pCluster->pev, vecOrigin );
pCluster->pev->velocity        = vecDir * fSpeed;
pCluster->pev->owner           = INDEXENT(hOwner);
pCluster->m_flDetonateTime     = gpGlobals->time +
RANDOM_FLOAT( GCG_CLUSTER_FUSE_MIN, GCG_CLUSTER_FUSE_MAX );
pCluster->Spawn();
}
}

// -----------------------------------------------------------------------
// CGCGrenadeArmed  (gunman_weapon_grenade_armed)
//
// Thrown grenade projectile.  Bounces until its fuse expires, then
// detonates.  m_bDetonateOnImpact causes immediate detonation on any
// world/entity collision.  m_bDoCluster spawns sub-bombs.
// -----------------------------------------------------------------------
class CGCGrenadeArmed : public CBaseEntity
{
public:
void Spawn( void );
void Precache( void );
void EXPORT GrenadeTouch( CBaseEntity *pOther );
void EXPORT GrenadeThink( void );

void Detonate( void );
void ExplodeNow( void );

static CGCGrenadeArmed *Create( const Vector &vecOrigin,
const Vector &vecVelocity, CBaseEntity *pOwner,
BOOL bDetonateOnImpact, BOOL bDoCluster,
float fFuse = GCG_FUSE_TIME );

BOOL  m_bDetonateOnImpact;
BOOL  m_bDoCluster;
float m_flDetonateTime;

virtual int Save( CSave &save );
virtual int Restore( CRestore &restore );
static TYPEDESCRIPTION m_SaveData[];
};

TYPEDESCRIPTION CGCGrenadeArmed::m_SaveData[] =
{
DEFINE_FIELD( CGCGrenadeArmed, m_bDetonateOnImpact, FIELD_BOOLEAN ),
DEFINE_FIELD( CGCGrenadeArmed, m_bDoCluster,        FIELD_BOOLEAN ),
DEFINE_FIELD( CGCGrenadeArmed, m_flDetonateTime,    FIELD_TIME    ),
};
IMPLEMENT_SAVERESTORE( CGCGrenadeArmed, CBaseEntity );
LINK_ENTITY_TO_CLASS( gunman_weapon_grenade_armed, CGCGrenadeArmed );

void CGCGrenadeArmed::Precache( void )
{
PRECACHE_MODEL( "models/gunman/w_grenadecore_prop.mdl" );
PRECACHE_MODEL( "models/gunman/dmlcluster.mdl" );
PRECACHE_SOUND( GMSND_GRENADE_HIT1 );
PRECACHE_SOUND( GMSND_GRENADE_HIT2 );
PRECACHE_SOUND( GMSND_GRENADE_HIT3 );
PRECACHE_SOUND( GMSND_DML_FRAGMENT );
PRECACHE_SOUND( GMSND_KABAM1   );
PRECACHE_SOUND( GMSND_KABAM2   );
PRECACHE_SOUND( GMSND_KABAM3   );
PRECACHE_SOUND( GMSND_KABOOM1  );
PRECACHE_SOUND( GMSND_KABOOM2  );
PRECACHE_SOUND( GMSND_KABOOM3  );
PRECACHE_SOUND( GMSND_MINE_DEPLOY   );
PRECACHE_SOUND( GMSND_MINE_CHARGE   );
PRECACHE_SOUND( GMSND_MINE_ACTIVATE );
}

void CGCGrenadeArmed::Spawn( void )
{
Precache();

pev->movetype = MOVETYPE_BOUNCE;
pev->solid    = SOLID_BBOX;
pev->gravity  = 1.0f;
pev->friction = 0.6f;

SET_MODEL( ENT(pev), "models/gunman/w_grenadecore_prop.mdl" );
UTIL_SetSize( pev, Vector(-4,-4,-4), Vector(4,4,4) );
UTIL_SetOrigin( pev, pev->origin );

SetTouch( &CGCGrenadeArmed::GrenadeTouch );
SetThink( &CGCGrenadeArmed::GrenadeThink );
pev->nextthink = gpGlobals->time + 0.1f;
}

void CGCGrenadeArmed::GrenadeTouch( CBaseEntity *pOther )
{
if ( pev->velocity.Length() > 100.0f )
{
const char *pszSounds[3] = { GMSND_GRENADE_HIT1, GMSND_GRENADE_HIT2, GMSND_GRENADE_HIT3 };
EMIT_SOUND( ENT(pev), CHAN_ITEM,
pszSounds[ RANDOM_LONG(0,2) ], 0.7f, ATTN_NORM );
}

if ( m_bDetonateOnImpact )
{
// Ignore detonation during the first 0.1 s to clear the thrower
if ( gpGlobals->time > m_flDetonateTime - GCG_FUSE_TIME + 0.1f )
Detonate();
}
}

void CGCGrenadeArmed::GrenadeThink( void )
{
if ( gpGlobals->time >= m_flDetonateTime )
{
Detonate();
return;
}
pev->nextthink = gpGlobals->time + 0.1f;
}

void CGCGrenadeArmed::Detonate( void )
{
SetTouch( NULL );
SetThink( NULL );

if ( m_bDoCluster )
GCG_SpawnClusters( pev->origin, OFFSET(pev) );
else
ExplodeNow();
}

void CGCGrenadeArmed::ExplodeNow( void )
{
TraceResult tr;
Vector vecSpot = pev->origin + Vector(0,0,8);
UTIL_TraceLine( vecSpot, vecSpot + Vector(0,0,-40), ignore_monsters, ENT(pev), &tr );
if ( tr.flFraction != 1.0f )
{
if ( RANDOM_FLOAT(0.f,1.f) < 0.5f )
UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
else
UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
}

MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
WRITE_BYTE( TE_EXPLOSION );
WRITE_COORD( pev->origin.x );
WRITE_COORD( pev->origin.y );
WRITE_COORD( pev->origin.z );
WRITE_SHORT( g_sModelIndexFireball );
WRITE_BYTE( 20 );
WRITE_BYTE( 15 );
WRITE_BYTE( TE_EXPLFLAG_NONE );
MESSAGE_END();

const char *pszSounds[3] = { GMSND_KABAM1, GMSND_KABAM2, GMSND_KABAM3 };
EMIT_SOUND( ENT(pev), CHAN_AUTO,
pszSounds[ RANDOM_LONG(0,2) ], 1.0f, ATTN_NORM );

::RadiusDamage( pev->origin, pev, pev,
GCG_BLAST_DMG, GCG_BLAST_RADIUS, CLASS_NONE, DMG_BLAST );

UTIL_Remove( this );
}

/*static*/ CGCGrenadeArmed *CGCGrenadeArmed::Create(
const Vector &vecOrigin, const Vector &vecVelocity, CBaseEntity *pOwner,
BOOL bDetonateOnImpact, BOOL bDoCluster, float fFuse )
{
CGCGrenadeArmed *p = GetClassPtr( (CGCGrenadeArmed *)NULL );
UTIL_SetOrigin( p->pev, vecOrigin );
p->pev->velocity           = vecVelocity;
p->pev->owner              = pOwner ? pOwner->edict() : NULL;
p->m_bDetonateOnImpact     = bDetonateOnImpact;
p->m_bDoCluster            = bDoCluster;
p->m_flDetonateTime        = gpGlobals->time + fFuse;
p->Spawn();
return p;
}

// -----------------------------------------------------------------------
// CGCGrenadeTripmine  (gunman_weapon_grenade_tripmine)
//
// Grenade placed on a surface.  Arms after GCG_TRIPMINE_ARM_DELAY seconds,
// then detonates when any entity crosses the forward tripwire or when the
// mine itself takes damage.
// -----------------------------------------------------------------------
class CGCGrenadeTripmine : public CBaseEntity
{
public:
void Spawn( void );
void Precache( void );
void EXPORT TripThink( void );
int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker,
float flDamage, int bitsDamageType );

void Detonate( void );

static CGCGrenadeTripmine *Create( const Vector &vecOrigin,
const Vector &vecAngles, CBaseEntity *pOwner, BOOL bDoCluster );

BOOL  m_bArmed;
BOOL  m_bDoCluster;
float m_flArmTime;
float m_flLineLength;

virtual int Save( CSave &save );
virtual int Restore( CRestore &restore );
static TYPEDESCRIPTION m_SaveData[];
};

TYPEDESCRIPTION CGCGrenadeTripmine::m_SaveData[] =
{
DEFINE_FIELD( CGCGrenadeTripmine, m_bArmed,       FIELD_BOOLEAN ),
DEFINE_FIELD( CGCGrenadeTripmine, m_bDoCluster,   FIELD_BOOLEAN ),
DEFINE_FIELD( CGCGrenadeTripmine, m_flArmTime,    FIELD_TIME    ),
DEFINE_FIELD( CGCGrenadeTripmine, m_flLineLength, FIELD_FLOAT   ),
};
IMPLEMENT_SAVERESTORE( CGCGrenadeTripmine, CBaseEntity );
LINK_ENTITY_TO_CLASS( gunman_weapon_grenade_tripmine, CGCGrenadeTripmine );

void CGCGrenadeTripmine::Precache( void )
{
PRECACHE_MODEL( "models/gunman/w_grenadecore_prop.mdl" );
PRECACHE_SOUND( GMSND_MINE_DEPLOY   );
PRECACHE_SOUND( GMSND_MINE_CHARGE   );
PRECACHE_SOUND( GMSND_MINE_ACTIVATE );
PRECACHE_SOUND( GMSND_KABAM1 );
PRECACHE_SOUND( GMSND_KABAM2 );
PRECACHE_SOUND( GMSND_KABAM3 );
PRECACHE_SOUND( GMSND_DML_FRAGMENT );
}

void CGCGrenadeTripmine::Spawn( void )
{
Precache();

pev->movetype   = MOVETYPE_NONE;
pev->solid      = SOLID_BBOX;
pev->takedamage = DAMAGE_YES;

SET_MODEL( ENT(pev), "models/gunman/w_grenadecore_prop.mdl" );
UTIL_SetSize( pev, Vector(-4,-4,-4), Vector(4,4,4) );
UTIL_SetOrigin( pev, pev->origin );

m_bArmed      = FALSE;
m_flLineLength = 0.0f;

EMIT_SOUND( ENT(pev), CHAN_ITEM, GMSND_MINE_DEPLOY, 0.8f, ATTN_NORM );

SetThink( &CGCGrenadeTripmine::TripThink );
pev->nextthink = gpGlobals->time + GCG_TRIPMINE_ARM_DELAY;
}

void CGCGrenadeTripmine::TripThink( void )
{
if ( !m_bArmed )
{
m_bArmed = TRUE;
EMIT_SOUND( ENT(pev), CHAN_ITEM, GMSND_MINE_ACTIVATE, 0.8f, ATTN_NORM );

// Record initial tripwire fraction
TraceResult tr;
Vector vecForward;
UTIL_MakeVectorsPrivate( pev->angles, vecForward, NULL, NULL );
Vector vecEnd = pev->origin + vecForward * GCG_TRIPMINE_TRACE_LEN;
UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, ENT(pev), &tr );
m_flLineLength = tr.flFraction;

pev->nextthink = gpGlobals->time + 0.05f;
return;
}

// Check for tripwire break
TraceResult tr;
Vector vecForward;
UTIL_MakeVectorsPrivate( pev->angles, vecForward, NULL, NULL );
Vector vecEnd = pev->origin + vecForward * GCG_TRIPMINE_TRACE_LEN;
UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, ENT(pev), &tr );

if ( m_flLineLength > 0.0f && fabs( tr.flFraction - m_flLineLength ) > 0.01f )
{
Detonate();
return;
}

pev->nextthink = gpGlobals->time + 0.05f;
}

int CGCGrenadeTripmine::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker,
float flDamage, int bitsDamageType )
{
if ( m_bArmed )
Detonate();
return 0;
}

void CGCGrenadeTripmine::Detonate( void )
{
SetThink( NULL );
pev->takedamage = DAMAGE_NO;

if ( m_bDoCluster )
{
GCG_SpawnClusters( pev->origin, OFFSET(pev) );
}
else
{
TraceResult tr;
Vector vecSpot = pev->origin + Vector(0,0,8);
UTIL_TraceLine( vecSpot, vecSpot + Vector(0,0,-40), ignore_monsters, ENT(pev), &tr );
if ( tr.flFraction != 1.0f )
UTIL_DecalTrace( &tr, DECAL_SCORCH1 );

MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
WRITE_BYTE( TE_EXPLOSION );
WRITE_COORD( pev->origin.x );
WRITE_COORD( pev->origin.y );
WRITE_COORD( pev->origin.z );
WRITE_SHORT( g_sModelIndexFireball );
WRITE_BYTE( 20 );
WRITE_BYTE( 15 );
WRITE_BYTE( TE_EXPLFLAG_NONE );
MESSAGE_END();

const char *pszSounds[3] = { GMSND_KABAM1, GMSND_KABAM2, GMSND_KABAM3 };
EMIT_SOUND( ENT(pev), CHAN_AUTO,
pszSounds[ RANDOM_LONG(0,2) ], 1.0f, ATTN_NORM );

::RadiusDamage( pev->origin, pev, pev,
GCG_BLAST_DMG, GCG_BLAST_RADIUS, CLASS_NONE, DMG_BLAST );
}

UTIL_Remove( this );
}

/*static*/ CGCGrenadeTripmine *CGCGrenadeTripmine::Create(
const Vector &vecOrigin, const Vector &vecAngles,
CBaseEntity *pOwner, BOOL bDoCluster )
{
CGCGrenadeTripmine *p = GetClassPtr( (CGCGrenadeTripmine *)NULL );
UTIL_SetOrigin( p->pev, vecOrigin );
p->pev->angles   = vecAngles;
p->pev->owner    = pOwner ? pOwner->edict() : NULL;
p->m_bDoCluster  = bDoCluster;
p->Spawn();
return p;
}

// =======================================================================
// weapon_gcgrenade / CGCGrenade
//
// Player weapon.  SecondaryAttack cycles the 2-axis config menu.
// PrimaryAttack throws or places the configured grenade variant.
// =======================================================================

LINK_ENTITY_TO_CLASS( weapon_gcgrenade, CGCGrenade );

#ifndef CLIENT_DLL

TYPEDESCRIPTION CGCGrenade::m_SaveData[] =
{
DEFINE_FIELD( CGCGrenade, m_iDetonationType, FIELD_INTEGER ),
DEFINE_FIELD( CGCGrenade, m_iPayloadType,    FIELD_INTEGER ),
DEFINE_FIELD( CGCGrenade, m_iMenuAxis,       FIELD_INTEGER ),
DEFINE_FIELD( CGCGrenade, m_flStartThrow,    FIELD_FLOAT   ),
DEFINE_FIELD( CGCGrenade, m_flReleaseThrow,  FIELD_FLOAT   ),
};
IMPLEMENT_SAVERESTORE( CGCGrenade, CBasePlayerWeapon );

#endif // CLIENT_DLL

void CGCGrenade::Spawn( void )
{
Precache();
m_iId = WEAPON_GCGRENADE;
SET_MODEL( ENT(pev), "models/gunman/w_grenadecore.mdl" );

m_iDefaultAmmo    = GCGRENADE_DEFAULT_GIVE;
m_iDetonationType = GCG_DEFAULT_DETONATION;
m_iPayloadType    = GCG_DEFAULT_PAYLOAD;
m_iMenuAxis       = 0;
m_flStartThrow    = 0;
m_flReleaseThrow  = -1;

FallInit();
}

void CGCGrenade::Precache( void )
{
PRECACHE_MODEL( "models/gunman/v_grenadecore.mdl" );
PRECACHE_MODEL( "models/gunman/w_grenadecore.mdl" );
PRECACHE_MODEL( "models/gunman/w_grenadecore_prop.mdl" );
PRECACHE_MODEL( "models/gunman/dmlcluster.mdl" );

PRECACHE_SOUND( GMSND_DRYFIRE      );
PRECACHE_SOUND( GMSND_GRENADE_HIT1 );
PRECACHE_SOUND( GMSND_GRENADE_HIT2 );
PRECACHE_SOUND( GMSND_GRENADE_HIT3 );
PRECACHE_SOUND( GMSND_DML_FRAGMENT );
PRECACHE_SOUND( GMSND_KABAM1   );
PRECACHE_SOUND( GMSND_KABAM2   );
PRECACHE_SOUND( GMSND_KABAM3   );
PRECACHE_SOUND( GMSND_KABOOM1  );
PRECACHE_SOUND( GMSND_KABOOM2  );
PRECACHE_SOUND( GMSND_KABOOM3  );
PRECACHE_SOUND( GMSND_MINE_DEPLOY   );
PRECACHE_SOUND( GMSND_MINE_CHARGE   );
PRECACHE_SOUND( GMSND_MINE_ACTIVATE );
}

int CGCGrenade::GetItemInfo( ItemInfo *p )
{
p->pszName    = STRING( pev->classname );
p->pszAmmo1   = "dml_ammo";
p->iMaxAmmo1  = DML_MAX_CARRY;
p->pszAmmo2   = NULL;
p->iMaxAmmo2  = -1;
p->iMaxClip   = GCGRENADE_MAX_CLIP;
p->iSlot      = 4;
p->iPosition  = 1;
p->iId        = m_iId = WEAPON_GCGRENADE;
p->iWeight    = GCGRENADE_WEIGHT;
p->iFlags     = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
return 1;
}

int CGCGrenade::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGCGrenade::Deploy( void )
{
m_flReleaseThrow = -1;
m_flStartThrow   = 0;
return DefaultDeploy(
"models/gunman/v_grenadecore.mdl",
"models/gunman/w_grenadecore.mdl",
GCGRENADE_DRAW, "grenade" );
}

void CGCGrenade::Holster( int skiplocal /* = 0 */ )
{
m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
{
SendWeaponAnim( GCGRENADE_HOLSTER );
}
else
{
m_pPlayer->pev->weapons &= ~( 1 << WEAPON_GCGRENADE );
SetThink( &CGCGrenade::DestroyItem );
pev->nextthink = gpGlobals->time + 0.1f;
}

EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );
}

void CGCGrenade::SecondaryAttack( void )
{
if ( m_iMenuAxis == 0 )
{
m_iDetonationType++;
if ( m_iDetonationType > 3 )
{
m_iDetonationType = 1;
m_iMenuAxis = 1;
}
}
else
{
m_iPayloadType++;
if ( m_iPayloadType > 2 )
{
m_iPayloadType = 1;
m_iMenuAxis    = 0;
}
}

SendWeaponAnim( GCGRENADE_CUSTOMIZE );
EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, GMSND_DRYFIRE, 0.9f, ATTN_NORM );

m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
m_flTimeWeaponIdle      = UTIL_WeaponTimeBase() + 1.0f;
}

void CGCGrenade::PrimaryAttack( void )
{
if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 )
return;

if ( m_iDetonationType == 3 )
{
// TripMine: place on surface within 128 u
UTIL_MakeVectors( m_pPlayer->pev->v_angle );
Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
Vector vecEnd = vecSrc + gpGlobals->v_forward * 128.0f;

TraceResult tr;
UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters,
m_pPlayer->edict(), &tr );

if ( tr.flFraction < 1.0f )
{
Vector vecPos  = tr.vecEndPos;
Vector vecAngs = UTIL_VecToAngles( -tr.vecPlaneNormal );

BOOL bDoCluster = ( m_iPayloadType == 2 );
CGCGrenadeTripmine::Create( vecPos, vecAngs, m_pPlayer, bDoCluster );

SendWeaponAnim( GCGRENADE_TRIPMINE );
m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

m_flNextPrimaryAttack = GetNextAttackDelay( 1.5f );
m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 1.5f;
}
}
else
{
if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0 )
{
m_flStartThrow   = gpGlobals->time;
m_flReleaseThrow = 0;
SendWeaponAnim( GCGRENADE_PINPULL );
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
}
}
}

void CGCGrenade::WeaponIdle( void )
{
if ( m_flReleaseThrow == 0 && m_flStartThrow )
m_flReleaseThrow = gpGlobals->time;

if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
return;

if ( m_flStartThrow )
{
Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
if ( angThrow.x < 0 )
angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0f );
else
angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0f );

float flVel = ( 90 - angThrow.x ) * 6.5f;
if ( flVel > 1000 ) flVel = 1000;

UTIL_MakeVectors( angThrow );
Vector vecSrc   = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs +
                  gpGlobals->v_forward * 16.0f;
Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

BOOL bOnImpact = ( m_iDetonationType == 2 );
BOOL bCluster  = ( m_iPayloadType    == 2 );

float fFuse = m_flStartThrow - gpGlobals->time + GCG_FUSE_TIME;
if ( fFuse < 0 ) fFuse = 0;

CGCGrenadeArmed::Create( vecSrc, vecThrow, m_pPlayer,
bOnImpact, bCluster, fFuse );

if      ( flVel < 500  ) SendWeaponAnim( GCGRENADE_THROW1 );
else if ( flVel < 1000 ) SendWeaponAnim( GCGRENADE_THROW2 );
else                     SendWeaponAnim( GCGRENADE_THROW3 );

m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

m_flReleaseThrow = 0;
m_flStartThrow   = 0;
m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 0.5f;

m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
{
m_flTimeWeaponIdle = m_flNextSecondaryAttack =
m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );
}
return;
}
else if ( m_flReleaseThrow > 0 )
{
m_flStartThrow = 0;

if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
SendWeaponAnim( GCGRENADE_DRAW );
else
{
RetireWeapon();
return;
}

m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
m_flReleaseThrow = -1;
return;
}

if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
{
int iAnim;
float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
if ( flRand <= 0.75f )
{
iAnim = GCGRENADE_IDLE;
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}
else
{
iAnim = GCGRENADE_FIDGET;
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0f / 30.0f;
}
SendWeaponAnim( iAnim );
}
}

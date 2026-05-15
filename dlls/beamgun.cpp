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
// weapon_beamgun — Gunman Chronicles Beam Gun
//
// Three fire-mode axes ported from:
//   game/lua/weapons/gunman_weapon_beamgun/shared.lua
//   game/lua/gunman_data.lua  (damage / ammo-usage tables)
//
//   Range (1-4)         : beam length = 128 * Range^2 units
//   Power/Accuracy (1-4): damage from Modes[]={7,10,12,15}, ammo per shot
//   Lightning (1-3)     :
//     1 = Beam / Tazer  — hitscan continuous beam; Range<=2 triggers tazer burst
//     2 = Chain         — beam + periodic CBeamGunChain explosion entity
//     3 = Ball          — charges 0.75 s then launches CBeamGunBall projectile
//
// SecondaryAttack cycles Range → Power/Accuracy → Lightning menu axes.
// Hold primary to windup (0.75 s), then fire; release to stop.
// Temperature ramps while firing; malfunction lockout at 140.
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
// Damage / ammo-usage tables  (game/lua/gunman_data.lua)
// -----------------------------------------------------------------------
#define BG_DMG_MODES_1			7		// PowerAndAccuracy=1 damage
#define BG_DMG_MODES_2			10		// PowerAndAccuracy=2 damage
#define BG_DMG_MODES_3			12		// PowerAndAccuracy=3 damage
#define BG_DMG_MODES_4			15		// PowerAndAccuracy=4 damage
#define BG_DMG_TAZER_BURST		160		// Tazer burst total (divided by P/A)

#define BG_BALL_TOUCH_DMG		100		// CBeamGunBall direct-hit damage
#define BG_BALL_BLAST_DMG		100		// CBeamGunBall blast damage
#define BG_BALL_RADIUS			128.0f	// CBeamGunBall blast radius
#define BG_BALL_THINK_BLAST		10		// CBeamGunBall periodic blast dmg
#define BG_BALL_THINK_RADIUS	128.0f	// CBeamGunBall periodic blast radius

#define BG_BSMALL_TOUCH_DMG		20		// CBeamGunBallSmall direct-hit damage
#define BG_BSMALL_BLAST_DMG		10		// CBeamGunBallSmall blast damage
#define BG_BSMALL_RADIUS		64.0f	// CBeamGunBallSmall blast radius

#define BG_CHAIN_DMG			5		// CBeamGunChain blast damage

#define BG_AMMO_STANDARD		1		// ammo per standard beam tick
#define BG_AMMO_POWERBALL		20		// ammo for PowerBall launch

// -----------------------------------------------------------------------
// Tuning constants
// -----------------------------------------------------------------------
#define BG_WINDUP_TIME			0.75f	// seconds to full charge
#define BG_FIRE_INTERVAL		0.1f	// seconds per continuous-fire tick
#define BG_CHAIN_SPAWN_INTERVAL	0.5f	// minimum seconds between chain spawns
#define BG_TEMP_RISE_RATE		0.2f	// temperature rise per fire tick
#define BG_TEMP_FALL_RATE		0.1f	// temperature fall per idle tick
#define BG_TEMP_MALFUNCTION		140.0f	// temperature triggering malfunction
#define BG_TEMP_CLEAR			130.0f	// temperature at which malfunction clears
#define BG_MALFUNCTION_DELAY	3.0f	// lockout duration on malfunction
#define BG_BALL_EXPIRE			10.0f	// auto-expire for BeamGunBall (seconds)
#define BG_BSMALL_EXPIRE		3.0f	// auto-expire for BallSmall (seconds)
#define BG_CHAIN_THINK_COUNT	10		// Think cycles before chain entity removes

// Default axis values (game/lua/gunman_data.lua DefaultWeaponSettings.Beamgun)
#define BG_DEFAULT_RANGE		3
#define BG_DEFAULT_POWER		2
#define BG_DEFAULT_LIGHTNING	1

// Beam-length formula: 128 * Range^2 (max Range=4 → 2048 u)
static inline float BG_BeamLength( int iRange )
{
	return 128.0f * (float)(iRange * iRange);
}

// Ammo cost per tick: ceil((5 - P/A) * 0.5) * BG_AMMO_STANDARD
// PA=1 → ceil(2.0)*1=2, PA=2 → ceil(1.5)*1=2, PA=3 → ceil(1.0)*1=1, PA=4 → ceil(0.5)*1=1
static inline int BG_AmmoCost( int iPowerAndAccuracy )
{
	int raw = (int)( (5 - iPowerAndAccuracy) * 0.5f );
	// ceiling without float overhead
	if ( ( (5 - iPowerAndAccuracy) & 1 ) != 0 )
		raw++;
	return raw < 1 ? 1 : raw;
}

// -----------------------------------------------------------------------
// Animation indices  (model: models/v_beam.mdl)
// -----------------------------------------------------------------------
enum beamgun_anim_e
{
	BEAMGUN_ARM			= 0,
	BEAMGUN_IDLESUBDUEL,
	BEAMGUN_IDLEBLADE,
	BEAMGUN_IDLEINSPECT,
	BEAMGUN_FIRE,
	BEAMGUN_SHOOTSINGLE,
	BEAMGUN_CONFIG,
	BEAMGUN_CHARGE
};

// -----------------------------------------------------------------------
// CBeamGunBallSmall — tiny sub-projectile spawned by the main ball
//   Touch: 20 hp, blast 10 hp, radius 64 u.  Expires after 3 s.
// -----------------------------------------------------------------------
class CBeamGunBallSmall : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT BallSmallTouch( CBaseEntity *pOther );
	void EXPORT BallSmallThink( void );

	static CBeamGunBallSmall *CreateBallSmall( Vector vecOrigin, Vector vecVelocity, CBaseEntity *pOwner );

	float m_flExpireTime;
	Vector m_vecLastPos;
};

LINK_ENTITY_TO_CLASS( beamgun_ball_small, CBeamGunBallSmall );

void CBeamGunBallSmall::Spawn( void )
{
	pev->movetype  = MOVETYPE_FLY;
	pev->solid     = SOLID_BBOX;
	pev->classname = MAKE_STRING( "beamgun_ball_small" );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt  = 160;

	SET_MODEL( ENT(pev), "models/dmlcluster.mdl" );
	UTIL_SetSize( pev, Vector(-8,-8,-8), Vector(8,8,8) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CBeamGunBallSmall::BallSmallTouch );
	SetThink( &CBeamGunBallSmall::BallSmallThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	m_vecLastPos = pev->origin;
}

CBeamGunBallSmall *CBeamGunBallSmall::CreateBallSmall( Vector vecOrigin, Vector vecVelocity, CBaseEntity *pOwner )
{
	CBeamGunBallSmall *pBall = GetClassPtr( (CBeamGunBallSmall *)NULL );
	UTIL_SetOrigin( pBall->pev, vecOrigin );
	pBall->pev->velocity = vecVelocity;
	pBall->pev->owner    = pOwner->edict();
	pBall->m_flExpireTime = gpGlobals->time + BG_BSMALL_EXPIRE;
	pBall->Spawn();
	return pBall;
}

void CBeamGunBallSmall::BallSmallTouch( CBaseEntity *pOther )
{
	if ( pOther && pOther->edict() == pev->owner )
		return;

	entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;

	if ( pOther )
		pOther->TakeDamage( pev, pevAttacker, (float)BG_BSMALL_TOUCH_DMG, DMG_ENERGYBEAM );

	::RadiusDamage( pev->origin, pev, pevAttacker,
		(float)BG_BSMALL_BLAST_DMG, BG_BSMALL_RADIUS, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_BLAST );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE ( TE_SPARKS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	UTIL_Remove( this );
}

void CBeamGunBallSmall::BallSmallThink( void )
{
	if ( gpGlobals->time >= m_flExpireTime )
	{
		UTIL_Remove( this );
		return;
	}

	// Periodic blast damage
	entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;
	::RadiusDamage( pev->origin, pev, pevAttacker,
		(float)BG_BSMALL_BLAST_DMG, BG_BSMALL_RADIUS, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_BLAST );

	m_vecLastPos  = pev->origin;
	pev->nextthink = gpGlobals->time + 0.1f;
}

// -----------------------------------------------------------------------
// CBeamGunChain — lightning arc entity spawned by chain/ball modes
//   Applies BG_CHAIN_DMG blast damage each Think tick for ~5 s then removes.
// -----------------------------------------------------------------------
class CBeamGunChain : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT ChainThink( void );

	static CBeamGunChain *CreateChain( Vector vecOrigin, CBaseEntity *pOwner );

	int   m_iThinkCount;
	float m_flDamageRadius;
};

LINK_ENTITY_TO_CLASS( beamgun_chain, CBeamGunChain );

void CBeamGunChain::Spawn( void )
{
	pev->movetype  = MOVETYPE_NONE;
	pev->solid     = SOLID_NOT;
	pev->classname = MAKE_STRING( "beamgun_chain" );

	SET_MODEL( ENT(pev), "sprites/xbeam1.spr" );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CBeamGunChain::ChainThink );
	pev->nextthink = gpGlobals->time + 0.5f;
	m_iThinkCount  = 0;

	// Randomise the arc start/end extent as in Lua:
	// beamStartPos / beamEndPos offsets ±64 u around spawn
	m_flDamageRadius = 64.0f + (float)RANDOM_LONG( 32, 96 );
}

CBeamGunChain *CBeamGunChain::CreateChain( Vector vecOrigin, CBaseEntity *pOwner )
{
	CBeamGunChain *pChain = GetClassPtr( (CBeamGunChain *)NULL );
	UTIL_SetOrigin( pChain->pev, vecOrigin );
	pChain->pev->owner = pOwner->edict();
	pChain->Spawn();
	return pChain;
}

void CBeamGunChain::ChainThink( void )
{
	m_iThinkCount++;
	if ( m_iThinkCount > BG_CHAIN_THINK_COUNT )
	{
		UTIL_Remove( this );
		return;
	}

	entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;
	::RadiusDamage( pev->origin, pev, pevAttacker,
		(float)BG_CHAIN_DMG, m_flDamageRadius, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_BLAST );

	// Electric sparks visual
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE ( TE_SPARKS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	pev->nextthink = gpGlobals->time + 0.5f;
}

// -----------------------------------------------------------------------
// CBeamGunBall — main power-ball projectile (Lightning=3 mode)
//   Touch: 100 hp, blast 100/128.  Think blast 10/128 every 0.25 s.
//   Spawns CBeamGunBallSmall sub-projectiles; also spawns chain arcs.
//   Expires after 10 s or on touch.
// -----------------------------------------------------------------------
class CBeamGunBall : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT BallTouch( CBaseEntity *pOther );
	void EXPORT BallThink( void );
	void Explode( void );

	static CBeamGunBall *CreateBall( Vector vecOrigin, Vector vecVelocity, CBaseEntity *pOwner );

	float m_flExpireTime;
	float m_flNextSubSpawn;
	float m_flNextChainSpawn;
	int   m_iSubCount;
	int   m_iChainCount;
};

LINK_ENTITY_TO_CLASS( beamgun_ball, CBeamGunBall );

void CBeamGunBall::Spawn( void )
{
	pev->movetype  = MOVETYPE_FLY;
	pev->solid     = SOLID_BBOX;
	pev->classname = MAKE_STRING( "beamgun_ball" );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt  = 200;
	pev->gravity    = 0.0f;

	SET_MODEL( ENT(pev), "models/dmlcluster.mdl" );
	UTIL_SetSize( pev, Vector(-16,-16,-16), Vector(16,16,16) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CBeamGunBall::BallTouch );
	SetThink( &CBeamGunBall::BallThink );
	pev->nextthink = gpGlobals->time + 0.05f;

	m_flNextSubSpawn   = gpGlobals->time + 0.25f;
	m_flNextChainSpawn = gpGlobals->time + 0.25f;
	m_iSubCount        = 0;
	m_iChainCount      = 0;

	EMIT_SOUND( ENT(pev), CHAN_AUTO, "weapons/ball-fly.wav", 0.8f, ATTN_NORM );
}

CBeamGunBall *CBeamGunBall::CreateBall( Vector vecOrigin, Vector vecVelocity, CBaseEntity *pOwner )
{
	CBeamGunBall *pBall = GetClassPtr( (CBeamGunBall *)NULL );
	UTIL_SetOrigin( pBall->pev, vecOrigin );
	pBall->pev->velocity = vecVelocity;
	pBall->pev->owner    = pOwner->edict();
	pBall->m_flExpireTime = gpGlobals->time + BG_BALL_EXPIRE;
	pBall->Spawn();
	return pBall;
}

void CBeamGunBall::BallTouch( CBaseEntity *pOther )
{
	if ( pOther && pOther->edict() == pev->owner )
		return;
	if ( pev->dmgtime == 1 )		// already exploding
		return;
	pev->dmgtime = 1;

	entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;

	if ( pOther )
		pOther->TakeDamage( pev, pevAttacker, (float)BG_BALL_TOUCH_DMG, DMG_ENERGYBEAM );

	Explode();
}

void CBeamGunBall::Explode( void )
{
	entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;

	::RadiusDamage( pev->origin, pev, pevAttacker,
		(float)BG_BALL_BLAST_DMG, BG_BALL_RADIUS, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_BLAST );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE ( TE_SPARKS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	EMIT_SOUND( ENT(pev), CHAN_AUTO, "weapons/ball-die.wav", 1.0f, ATTN_NORM );

	UTIL_Remove( this );
}

void CBeamGunBall::BallThink( void )
{
	if ( gpGlobals->time >= m_flExpireTime )
	{
		Explode();
		return;
	}

	entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;

	// Periodic blast (Lua: ThinkBlast=10, ThinkRadius=128)
	::RadiusDamage( pev->origin, pev, pevAttacker,
		(float)BG_BALL_THINK_BLAST, BG_BALL_THINK_RADIUS, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_BLAST );

	// Spawn sub-projectiles (Lua: up to 18 BallSmall entities)
	if ( gpGlobals->time >= m_flNextSubSpawn && m_iSubCount < 18 )
	{
		m_iSubCount++;
		Vector vecRandom = Vector(
			RANDOM_FLOAT(-1.0f, 1.0f),
			RANDOM_FLOAT(-1.0f, 1.0f),
			RANDOM_FLOAT(-1.0f, 1.0f) ).Normalize();
		CBeamGunBallSmall::CreateBallSmall(
			pev->origin,
			vecRandom * 300.0f,
			CBaseEntity::Instance( pev->owner ) ? CBaseEntity::Instance( pev->owner ) : this );
		m_flNextSubSpawn = gpGlobals->time + 0.25f;
	}

	// Spawn chain arcs (Lua: up to 3 chain entities)
	if ( gpGlobals->time >= m_flNextChainSpawn && m_iChainCount < 3 )
	{
		m_iChainCount++;
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		CBeamGunChain::CreateChain( pev->origin, pOwner ? pOwner : this );
		m_flNextChainSpawn = gpGlobals->time + 0.25f;
	}

	pev->nextthink = gpGlobals->time + 0.05f;
}


// -----------------------------------------------------------------------
// CBeamGun implementation
// -----------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( weapon_beamgun, CBeamGun );

void CBeamGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BEAMGUN;
	SET_MODEL(ENT(pev), "models/W_beam.mdl");

	m_iRange            = BG_DEFAULT_RANGE;
	m_iPowerAndAccuracy = BG_DEFAULT_POWER;
	m_iLightning        = BG_DEFAULT_LIGHTNING;
	m_iFireState        = BGFIRE_OFF;
	m_flChargeStartTime = 0;
	m_bBallWasLaunched  = FALSE;
	m_flBeamTemp        = 0.0f;
	m_bMalfunction      = FALSE;
	m_flMalfunctionReset = 0;
	m_flNextChainTime   = 0;

	m_iDefaultAmmo = BEAMGUN_DEFAULT_GIVE;
	FallInit();
}


void CBeamGun::Precache( void )
{
	PRECACHE_MODEL("models/v_beam.mdl");
	PRECACHE_MODEL("models/W_beam.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");
	PRECACHE_MODEL("models/dmlcluster.mdl");		// ball projectile placeholder model
	PRECACHE_MODEL("sprites/xbeam1.spr");		// chain arc sprite

	// Windup / firing / off sounds
	PRECACHE_SOUND("weapons/egon_windup2.wav");		// windup (P/A >= 2)
	PRECACHE_SOUND("weapons/egon_windup2_new.wav");	// windup (P/A < 2)
	PRECACHE_SOUND("weapons/egon_run3.wav");			// continuous beam loop
	PRECACHE_SOUND("weapons/egon_off1.wav");			// weapon shut-off

	// Residual sounds (on release)
	PRECACHE_SOUND("weapons/residual1.wav");
	PRECACHE_SOUND("weapons/residual2.wav");
	PRECACHE_SOUND("weapons/residual3.wav");
	PRECACHE_SOUND("weapons/residual4.wav");

	// Malfunction / tazer
	PRECACHE_SOUND("weapons/overheat.wav");
	PRECACHE_SOUND("weapons/electro4.wav");

	// Fire-mode change (config)
	PRECACHE_SOUND("weapons/DryFire.wav");

	// Ball projectile sounds
	PRECACHE_SOUND("weapons/ball-fly.wav");
	PRECACHE_SOUND("weapons/ball-die.wav");

	m_usBeamGun = PRECACHE_EVENT( 1, "events/egon.sc" );
}

int CBeamGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "beamgun_ammo";
	p->iMaxAmmo1 = BEAMGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BEAMGUN;
	p->iWeight = BEAMGUN_WEIGHT;

	return 1;
}

int CBeamGun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CBeamGun::Deploy( )
{
	m_iFireState        = BGFIRE_OFF;
	m_flChargeStartTime = 0;
	m_bBallWasLaunched  = FALSE;
	m_flBeamTemp        = 0.0f;
	m_bMalfunction      = FALSE;
	m_flMalfunctionReset = 0;

	return DefaultDeploy( "models/v_beam.mdl", "models/p_egon.mdl", BEAMGUN_ARM, "egon" );
}

// -----------------------------------------------------------------------
// SecondaryAttack — cycles through the three menu axes:
//   press 1: Range  1→2→3→4→1
//   press 2: Power/Accuracy 1→2→3→4→1
//   press 3: Lightning 1→2→3→1
// Each press advances current axis and plays the "config" animation.
// -----------------------------------------------------------------------
void CBeamGun::SecondaryAttack( void )
{
	if ( m_iFireState != BGFIRE_OFF )
		return;

	// Cycle axes in order: Range → Power/Accuracy → Lightning → Range
	// Determine which axis to advance based on a small tap counter encoded
	// in a single integer: we advance Range first, then P/A, then Lightning,
	// cycling the value and looping back.
	static int s_iAxis = 0;

	switch ( s_iAxis % 3 )
	{
	case 0:
		m_iRange = ( m_iRange % 4 ) + 1;
		break;
	case 1:
		m_iPowerAndAccuracy = ( m_iPowerAndAccuracy % 4 ) + 1;
		break;
	case 2:
		m_iLightning = ( m_iLightning % 3 ) + 1;
		break;
	}
	s_iAxis++;

	SendWeaponAnim( BEAMGUN_CONFIG );
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
		"weapons/DryFire.wav", 0.8f, ATTN_NORM );

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flNextPrimaryAttack   = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle      = UTIL_WeaponTimeBase() + 1.5f;
}

// -----------------------------------------------------------------------
// PrimaryAttack — drive the windup → firing state machine
// -----------------------------------------------------------------------
void CBeamGun::PrimaryAttack( void )
{
	if ( m_bMalfunction )
	{
		if ( gpGlobals->time >= m_flMalfunctionReset )
		{
			m_bMalfunction = FALSE;
		}
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
			return;
		}
	}

	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
		return;
	}

	switch ( m_iFireState )
	{
	case BGFIRE_OFF:
		BeginAttack();
		break;

	case BGFIRE_WINDUP:
		if ( ( gpGlobals->time - m_flChargeStartTime ) >= BG_WINDUP_TIME )
		{
			m_iFireState = BGFIRE_FIRING;
			SendWeaponAnim( BEAMGUN_FIRE );
			// Fall through to fire this tick
			ContinueAttack();
		}
		else
		{
			// Still winding up — re-arm quickly to keep polling
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + BG_FIRE_INTERVAL;
			m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + BG_FIRE_INTERVAL;
		}
		break;

	case BGFIRE_FIRING:
		ContinueAttack();
		break;
	}
}

//=========================================================
void CBeamGun::BeginAttack( void )
{
	m_iFireState        = BGFIRE_WINDUP;
	m_flChargeStartTime = gpGlobals->time;

	// Choose windup sound by power level (Lua: PA >= 2 uses egon_windup2)
	const char *pszWindup = ( m_iPowerAndAccuracy >= 2 )
		? "weapons/egon_windup2.wav"
		: "weapons/egon_windup2_new.wav";

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, pszWindup, 0.9f, ATTN_NORM );

	SendWeaponAnim( BEAMGUN_CHARGE );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Poll quickly to detect when charge is complete
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + BG_FIRE_INTERVAL;
	m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + BG_FIRE_INTERVAL;
}

//=========================================================
void CBeamGun::ContinueAttack( void )
{
	if ( m_bBallWasLaunched )
	{
		// Ball mode: ball is in flight — wait for WeaponIdle (button release)
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 0.05f;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		EndAttack();
		return;
	}

	FireWeapon();
}

//=========================================================
void CBeamGun::FireWeapon( void )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc    = m_pPlayer->GetGunPosition();
	float  flLength  = BG_BeamLength( m_iRange );

	// Accuracy spread (decreases with higher P/A)
	float flCone = 0.02f / (float)m_iPowerAndAccuracy;
	Vector vecDir = gpGlobals->v_forward
		+ gpGlobals->v_right * RANDOM_FLOAT( -flCone, flCone )
		+ gpGlobals->v_up    * RANDOM_FLOAT( -flCone, flCone );
	vecDir = vecDir.Normalize();

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecDir * flLength,
		dont_ignore_monsters, m_pPlayer->edict(), &tr );

	int iDmgTable[4] = { BG_DMG_MODES_1, BG_DMG_MODES_2,
	                     BG_DMG_MODES_3, BG_DMG_MODES_4 };
	int iDmg = iDmgTable[ m_iPowerAndAccuracy - 1 ];

	if ( m_iLightning == 3 )
	{
		// Ball mode: launch the ball once then enter wait state
		LaunchPowerBall();
		return;
	}

	// Lightning 1 or 2: apply beam / tazer damage

	if ( m_iRange <= 2 )
	{
		// Tazer burst when hitting an entity at close range
		if ( tr.flFraction < 1.0f )
		{
			CBaseEntity *pEnt = CBaseEntity::Instance( tr.pHit );
			if ( pEnt && !FClassnameIs( pEnt->pev, "worldspawn" )
				&& !FClassnameIs( pEnt->pev, "beamgun_chain" ) )
			{
				float flTazerDmg = (float)BG_DMG_TAZER_BURST / (float)m_iPowerAndAccuracy;
				pEnt->TakeDamage( pev, m_pPlayer->pev, flTazerDmg, DMG_ENERGYBEAM | DMG_BLAST );

				// Tazer zap sound
				EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC,
					"weapons/overheat.wav", 0.6f, ATTN_NORM );

				// Long cooldown after tazer
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.0f;
				m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 2.0f;

				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= BG_AMMO_STANDARD;
				EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON,
					"weapons/egon_run3.wav", 0.6f, ATTN_NORM );
				return;
			}
		}
	}

	// Standard beam damage at trace endpoint
	if ( tr.flFraction < 1.0f )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( tr.pHit );
		if ( pEnt )
		{
			ClearMultiDamage();
			pEnt->TraceAttack( m_pPlayer->pev, (float)iDmg, vecDir, &tr, DMG_ENERGYBEAM );
			ApplyMultiDamage( pev, m_pPlayer->pev );
		}

		// Area blast at impact point
		::RadiusDamage( tr.vecEndPos, pev, m_pPlayer->pev,
			(float)iDmg, 16.0f, CLASS_NONE,
			DMG_ENERGYBEAM | DMG_BLAST );

		// Impact spark
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE ( TE_SPARKS );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
		MESSAGE_END();
	}

	// Chain mode: spawn an arc entity at hit position occasionally
	if ( m_iLightning == 2 && gpGlobals->time >= m_flNextChainTime )
	{
		Vector vecChainPos = ( tr.flFraction < 1.0f ) ? tr.vecEndPos : vecSrc + vecDir * flLength;
		CBeamGunChain::CreateChain( vecChainPos, m_pPlayer );

		// Adjust spawn rate by power (Lua: adjustSpawnRate = (P/A^0.5)*0.25)
		float flAdjust = sqrtf( (float)m_iPowerAndAccuracy ) * 0.25f;
		m_flNextChainTime = gpGlobals->time + flAdjust;

		// Chain costs an extra ammo tick
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= BG_AMMO_STANDARD;
	}

	// Continuous beam running sound
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON,
		"weapons/egon_run3.wav", 0.6f, ATTN_NORM );

	// Consume ammo
	int iAmmoTick = BG_AmmoCost( m_iPowerAndAccuracy );
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= iAmmoTick )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= iAmmoTick;
	else
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
		EndAttack();
		return;
	}

	// Temperature rise
	m_flBeamTemp += BG_TEMP_RISE_RATE;
	if ( m_flBeamTemp > BG_TEMP_MALFUNCTION )
		m_flBeamTemp = BG_TEMP_MALFUNCTION;

	if ( !m_bMalfunction && m_flBeamTemp >= BG_TEMP_MALFUNCTION )
	{
		m_bMalfunction       = TRUE;
		m_flMalfunctionReset = gpGlobals->time + BG_MALFUNCTION_DELAY;
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
			"weapons/electro4.wav", 0.9f, ATTN_NORM );
		EndAttack();
		return;
	}

	m_pPlayer->pev->punchangle.x = -1.0f;
	m_pPlayer->m_iWeaponVolume   = NORMAL_GUN_VOLUME;

	// Keep polling
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + BG_FIRE_INTERVAL;
	m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + BG_FIRE_INTERVAL;
}

//=========================================================
void CBeamGun::LaunchPowerBall( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < BG_AMMO_POWERBALL )
	{
		PlayEmptySound();
		EndAttack();
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = gpGlobals->v_forward.Normalize();

	CBeamGunBall::CreateBall( vecSrc, vecDir * 500.0f, m_pPlayer );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= BG_AMMO_POWERBALL;
	m_bBallWasLaunched = TRUE;

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON,
		"weapons/egon_off1.wav", 0.8f, ATTN_NORM );

	m_pPlayer->pev->punchangle.x = -2.0f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 0.05f;
}

//=========================================================
void CBeamGun::EndAttack( void )
{
	m_iFireState = BGFIRE_OFF;

	if ( m_pPlayer )
	{
		STOP_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/egon_run3.wav" );
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON,
			"weapons/egon_off1.wav", 0.8f, ATTN_NORM );

		// Random residual sound
		char szResidual[32];
		int iRes = RANDOM_LONG(1, 4);
		snprintf( szResidual, sizeof(szResidual), "weapons/residual%d.wav", iRes );
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, szResidual, 0.6f, ATTN_NORM );
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle    = UTIL_WeaponTimeBase() + 1.0f;
}

// -----------------------------------------------------------------------
// Reload — no physical clip
// -----------------------------------------------------------------------
void CBeamGun::Reload( void )
{
	// WEAPON_NOCLIP — nothing to reload
}

// -----------------------------------------------------------------------
// WeaponIdle — called when button is released (or on quiet intervals)
// -----------------------------------------------------------------------
void CBeamGun::WeaponIdle( void )
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// If firing when player releases attack button → stop
	if ( m_iFireState != BGFIRE_OFF )
	{
		// Ball in flight: just reset so next press can launch again
		m_bBallWasLaunched = FALSE;
		EndAttack();
		return;
	}

	// Temperature cool-down while idle
	if ( m_flBeamTemp > 0.0f )
	{
		m_flBeamTemp -= BG_TEMP_FALL_RATE;
		if ( m_flBeamTemp < 0.0f )
			m_flBeamTemp = 0.0f;
	}

	// Check malfunction clear
	if ( m_bMalfunction && gpGlobals->time >= m_flMalfunctionReset
		&& m_flBeamTemp <= BG_TEMP_CLEAR )
	{
		m_bMalfunction = FALSE;
	}

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );
	int iAnim;

	if ( flRand <= 0.4f )
	{
		iAnim = BEAMGUN_IDLESUBDUEL;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
	}
	else if ( flRand <= 0.7f )
	{
		iAnim = BEAMGUN_IDLEBLADE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
	}
	else
	{
		iAnim = BEAMGUN_IDLEINSPECT;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
	}
	SendWeaponAnim( iAnim, 1 );
}

// -----------------------------------------------------------------------
// Holster
// -----------------------------------------------------------------------
void CBeamGun::Holster( int skiplocal /* = 0 */ )
{
	if ( m_iFireState != BGFIRE_OFF )
	{
		m_bBallWasLaunched = FALSE;
		EndAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( BEAMGUN_ARM );
}

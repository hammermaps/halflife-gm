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
// weapon_chemicalgun -- Gunman Chronicles Chemical Launcher
//
// 4-axis chemistry menu ported from:
//   game/lua/weapons/gunman_weapon_chemgun/shared.lua
//   game/lua/gunman_data.lua  (damage / ammo-usage tables)
//
//   Acid     (0-4) : acidity of the mixture (default 4)
//   Neutral  (0-4) : neutral stabiliser level (default 2)
//   Base     (0-4) : base component level (default 3)
//   Pressure (1-5) : launch force modifier (default 3)
//
// SecondaryAttack cycles Acid -> Neutral -> Base -> Pressure -> Acid,
// advancing the current axis value and playing the Changemixture anim.
//
// PrimaryAttack launches a CChemBomb projectile whose behaviour is
// derived from the chemistry settings via ChemConfig():
//   explode    : acid>0 && base>0        -> large blast on detonation
//   bounce     : explode && neutral>acid or neutral>base -> deflects
//   stick      : neutral>2 && (acid>0||base>0) && !bounce -> adheres
//   smokeBurn  : neutral>2 && (acid>2 || base>0&&!explode) -> periodic aoe
//   airExpireTime: explode -> max(1,5-neutral) else 4 s
//   damageArea : smokeBurn->64; explode->clamp(16*(a+b),32,128);
//                else->clamp(32*max(a,b,n),32,128)
//   skin       : a>0&&b>0->2; b>0&&n>b&&a==0->3; n>b&&n>a->1; else->0
//   ammoTake   : clamp(floor(1+(a+n+b)/6), 1, 3)
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
#define CG_DMG_DIRECT       8       // direct hit on touched entity (WeaponDamage.Chemgun)
#define CG_DMG_BLAST        60      // large chem explosion (EntityDamage.Explosions.Chem.Damage)
#define CG_AMMO_MIN         1       // WeaponAmmoUsage.Chemgun.Min
#define CG_AMMO_MAX         3       // WeaponAmmoUsage.Chemgun.Max

// Launch velocity: base + scale * pressure (u/s)
#define CG_LAUNCH_BASE      200.0f
#define CG_LAUNCH_SCALE     150.0f

// Fire interval
#define CG_FIRE_INTERVAL    0.8f    // seconds between shots

// smokeBurn damage tick interval
#define CG_SMOKEBURN_RATE   0.1f

// Default axis values  (game/lua/gunman_data.lua DefaultWeaponSettings.Chemgun)
#define CG_DEFAULT_ACID     4
#define CG_DEFAULT_NEUTRAL  2
#define CG_DEFAULT_BASE     3
#define CG_DEFAULT_PRESSURE 3

// -----------------------------------------------------------------------
// Animation indices  (model: models/v_chemgun.mdl)
//   draw=0, holster=1, Idle=2, Idlefidget=3, Changemixture=4, Shoot=5
// -----------------------------------------------------------------------
enum chemicalgun_e
{
    CHEMICALGUN_DRAW      = 0,  // draw
    CHEMICALGUN_HOLSTER   = 1,  // holster
    CHEMICALGUN_IDLE      = 2,  // Idle
    CHEMICALGUN_FIDGET    = 3,  // Idlefidget
    CHEMICALGUN_CUSTOMIZE = 4,  // Changemixture
    CHEMICALGUN_SHOOT     = 5,  // Shoot
};

// -----------------------------------------------------------------------
// Chemistry configuration helper
//   Derives all CChemBomb properties from the three chemical-level axes.
//   Pressure is handled separately (launch velocity + ammo cost).
// -----------------------------------------------------------------------
struct ChemConfig_t
{
int   iSkin;
BOOL  bExplode;
BOOL  bBounce;
BOOL  bStick;
BOOL  bSmokeBurn;
float flAirExpireTime;
float flDamageArea;
int   iAmmoTake;
};

static ChemConfig_t ChemConfig( int iAcid, int iNeutral, int iBase )
{
ChemConfig_t cfg;
cfg.bExplode        = FALSE;
cfg.bBounce         = FALSE;
cfg.bStick          = FALSE;
cfg.bSmokeBurn      = FALSE;
cfg.flAirExpireTime = 4.0f;
cfg.flDamageArea    = 32.0f;
cfg.iSkin           = 0;
cfg.iAmmoTake       = 1;

// explode: acid > 0 and base > 0
if ( iAcid > 0 && iBase > 0 )
cfg.bExplode = TRUE;

// bounce: explode && neutral dominates either component
if ( cfg.bExplode && ( iNeutral > iAcid || iNeutral > iBase ) )
cfg.bBounce = TRUE;

// stick: high neutral + components present, not already bouncing
if ( iNeutral > 2 && ( iAcid > 0 || iBase > 0 ) && !cfg.bBounce )
cfg.bStick = TRUE;

// smokeBurn: high neutral + strong acid or standalone base
if ( iNeutral > 2 )
{
if ( iAcid > 2 || ( iBase > 0 && !cfg.bExplode ) )
cfg.bSmokeBurn = TRUE;
}

// airExpireTime
if ( cfg.bExplode )
{
int iExpire = 5 - iNeutral;
cfg.flAirExpireTime = (float)( iExpire < 1 ? 1 : iExpire );
}
else
{
cfg.flAirExpireTime = 4.0f;
}

// damageArea
if ( cfg.bSmokeBurn )
{
cfg.flDamageArea = 64.0f;
}
else if ( cfg.bExplode )
{
int iArea = 16 * ( iAcid + iBase );
if ( iArea < 32 )  iArea = 32;
if ( iArea > 128 ) iArea = 128;
cfg.flDamageArea = (float)iArea;
}
else
{
int iMax = iAcid > iBase ? iAcid : iBase;
if ( iNeutral > iMax ) iMax = iNeutral;
int iArea = 32 * iMax;
if ( iArea < 32 )  iArea = 32;
if ( iArea > 128 ) iArea = 128;
cfg.flDamageArea = (float)iArea;
}

// skin (projectile colour)
if ( iBase > 0 && iAcid > 0 )
cfg.iSkin = 2;
else if ( iBase > 0 && iNeutral > iBase && iAcid == 0 )
cfg.iSkin = 3;
else if ( iNeutral > iBase && iNeutral > iAcid )
cfg.iSkin = 1;
else
cfg.iSkin = 0;

// ammoTake: clamp(floor(1 + (a+n+b)/6), 1, 3)
int iSum  = iAcid + iNeutral + iBase;
int iTake = 1 + ( iSum / 6 );// integer division floors automatically
if ( iTake < CG_AMMO_MIN ) iTake = CG_AMMO_MIN;
if ( iTake > CG_AMMO_MAX ) iTake = CG_AMMO_MAX;
cfg.iAmmoTake = iTake;

return cfg;
}

// -----------------------------------------------------------------------
// CChemBomb -- chemical projectile spawned by CChemicalGun
//
// Behaviour flags set at spawn time from ChemConfig():
//   m_bExplode   -- detonation uses large chem blast (CG_DMG_BLAST / 2*damageArea radius)
//   m_bBounce    -- projectile deflects off surfaces (MOVETYPE_BOUNCE)
//   m_bStick     -- stops on first surface contact; periodic damage if smokeBurn
//   m_bSmokeBurn -- continuous area damage while stuck
//
// Auto-detonates after m_flAirExpireTime seconds.
// -----------------------------------------------------------------------
class CChemBomb : public CBaseEntity
{
public:
    int     Save( CSave &save );
    int     Restore( CRestore &restore );
    static  TYPEDESCRIPTION m_SaveData[];

    void Spawn( void );
    void EXPORT BombTouch( CBaseEntity *pOther );
    void EXPORT BombThink( void );
    void Detonate( void );

    static CChemBomb *CreateBomb( const Vector &vecOrigin, const Vector &vecVelocity,
        CBaseEntity *pOwner, const ChemConfig_t &cfg );

    // Chemistry-derived properties (set by CreateBomb before Spawn)
    BOOL  m_bExplode;
    BOOL  m_bBounce;
    BOOL  m_bStick;
    BOOL  m_bSmokeBurn;
    float m_flDamageArea;

    float m_flExpireTime;
    float m_flNextSmokeBurn;
    BOOL  m_bStuck;
    BOOL  m_bDetonated;	// guard against re-entrant Detonate()
};

LINK_ENTITY_TO_CLASS( chembomb, CChemBomb );
// Lua entity name alias so any code or map that references
// "gunman_weapon_chembomb" resolves to the same class.
LINK_ENTITY_TO_CLASS( gunman_weapon_chembomb, CChemBomb );

TYPEDESCRIPTION CChemBomb::m_SaveData[] =
{
    DEFINE_FIELD( CChemBomb, m_bExplode,       FIELD_INTEGER ),
    DEFINE_FIELD( CChemBomb, m_bBounce,        FIELD_INTEGER ),
    DEFINE_FIELD( CChemBomb, m_bStick,         FIELD_INTEGER ),
    DEFINE_FIELD( CChemBomb, m_bSmokeBurn,     FIELD_INTEGER ),
    DEFINE_FIELD( CChemBomb, m_flDamageArea,   FIELD_FLOAT   ),
    DEFINE_FIELD( CChemBomb, m_flExpireTime,   FIELD_TIME    ),
    DEFINE_FIELD( CChemBomb, m_flNextSmokeBurn,FIELD_TIME    ),
    DEFINE_FIELD( CChemBomb, m_bStuck,         FIELD_INTEGER ),
    DEFINE_FIELD( CChemBomb, m_bDetonated,     FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CChemBomb, CBaseEntity );

void CChemBomb::Spawn( void )
{
pev->movetype  = m_bBounce ? MOVETYPE_BOUNCE : MOVETYPE_TOSS;
pev->solid     = SOLID_BBOX;
pev->classname = MAKE_STRING( "chembomb" );
pev->gravity   = 1.0f;
pev->friction  = 0.8f;

SET_MODEL( ENT(pev), "models/Tubeball.mdl" );
UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
UTIL_SetOrigin( pev, pev->origin );

SetTouch( &CChemBomb::BombTouch );
SetThink( &CChemBomb::BombThink );
pev->nextthink = gpGlobals->time + 0.1f;

m_bStuck          = FALSE;
m_bDetonated      = FALSE;
m_flNextSmokeBurn = gpGlobals->time + CG_SMOKEBURN_RATE;
}

CChemBomb *CChemBomb::CreateBomb( const Vector &vecOrigin, const Vector &vecVelocity,
CBaseEntity *pOwner, const ChemConfig_t &cfg )
{
CChemBomb *pBomb = GetClassPtr( (CChemBomb *)NULL );
UTIL_SetOrigin( pBomb->pev, vecOrigin );
pBomb->pev->velocity  = vecVelocity;
pBomb->pev->owner     = pOwner->edict();
pBomb->pev->skin      = cfg.iSkin;

pBomb->m_bExplode       = cfg.bExplode;
pBomb->m_bBounce        = cfg.bBounce;
pBomb->m_bStick         = cfg.bStick;
pBomb->m_bSmokeBurn     = cfg.bSmokeBurn;
pBomb->m_flDamageArea   = cfg.flDamageArea;
pBomb->m_flExpireTime   = gpGlobals->time + cfg.flAirExpireTime;

pBomb->Spawn();
return pBomb;
}

void CChemBomb::BombTouch( CBaseEntity *pOther )
{
if ( m_bDetonated )
return;
if ( pOther && pOther->edict() == pev->owner )
return;

// Classify: worldspawn / solid brush = surface; everything else = entity
BOOL bIsSurface = !pOther
|| FClassnameIs( pOther->pev, "worldspawn" )
|| ( pOther->pev->solid == SOLID_BSP );

if ( bIsSurface )
{
EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON,
"weapons/cgbounce.wav", 0.7f, ATTN_NORM, 0,
RANDOM_LONG( 90, 110 ) );

if ( m_bStick && !m_bStuck )
{
m_bStuck      = TRUE;
pev->movetype = MOVETYPE_NONE;
pev->velocity = g_vecZero;
}
else if ( !m_bBounce && !m_bStick )
{
// Detonate on surface impact when neither bounce nor stick
Detonate();
}
// m_bBounce: MOVETYPE_BOUNCE handles the deflection automatically
}
else
{
    // Direct entity hit
    entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;
    pOther->TakeDamage( pev, pevAttacker, (float)CG_DMG_DIRECT, DMG_ACID );

    if ( m_bStick && !m_bStuck )
    {
        // Parent the bomb to the entity so it follows its movement
        m_bStuck      = TRUE;
        pev->movetype = MOVETYPE_FOLLOW;
        pev->aiment   = pOther->edict();
        pev->velocity = g_vecZero;
    }
    else
    {
        Detonate();
    }
}
}

void CChemBomb::BombThink( void )
{
if ( m_bDetonated )
return;

// Auto-detonate when air timer expires
if ( gpGlobals->time >= m_flExpireTime )
{
Detonate();
return;
}

// Periodic smoke-burn area damage while stuck
if ( m_bStuck && m_bSmokeBurn && gpGlobals->time >= m_flNextSmokeBurn )
{
entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;
float flDmg = m_flDamageArea * 0.2f;
::RadiusDamage( pev->origin, pev, pevAttacker,
flDmg, m_flDamageArea, CLASS_NONE,
DMG_ACID | DMG_BLAST );

m_flNextSmokeBurn = gpGlobals->time + CG_SMOKEBURN_RATE;
}

pev->nextthink = gpGlobals->time + 0.1f;
}

void CChemBomb::Detonate( void )
{
if ( m_bDetonated )
return;
m_bDetonated = TRUE;

entvars_t *pevAttacker = pev->owner ? VARS( pev->owner ) : pev;

if ( m_bExplode )
{
// Large chem explosion: CG_DMG_BLAST dmg / (2 * damageArea) radius
float flRadius = 2.0f * m_flDamageArea;
::RadiusDamage( pev->origin, pev, pevAttacker,
(float)CG_DMG_BLAST, flRadius, CLASS_NONE,
DMG_ACID | DMG_BLAST );

MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
WRITE_BYTE ( TE_EXPLOSION );
WRITE_COORD( pev->origin.x );
WRITE_COORD( pev->origin.y );
WRITE_COORD( pev->origin.z );
WRITE_SHORT( g_sModelIndexFireball );
WRITE_BYTE ( 15 );// scale * 10
WRITE_BYTE ( 15 );// framerate
WRITE_BYTE ( TE_EXPLFLAG_NOSOUND );
MESSAGE_END();
}
else
{
// Normal release: damageArea * 0.2 dmg, damageArea radius
float flDmg = m_flDamageArea * 0.2f;
::RadiusDamage( pev->origin, pev, pevAttacker,
flDmg, m_flDamageArea, CLASS_NONE,
DMG_ACID | DMG_BLAST );

MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
WRITE_BYTE ( TE_SPARKS );
WRITE_COORD( pev->origin.x );
WRITE_COORD( pev->origin.y );
WRITE_COORD( pev->origin.z );
MESSAGE_END();
}

EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON,
"weapons/fizzy1.wav", 0.8f, ATTN_NORM, 0,
RANDOM_LONG( 90, 110 ) );

UTIL_Remove( this );
}

// -----------------------------------------------------------------------
// CChemicalGun weapon
// -----------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( weapon_chemicalgun, CChemicalGun );

void CChemicalGun::Spawn( )
{
    Precache( );
    m_iId = WEAPON_CHEMICALGUN;
    SET_MODEL( ENT(pev), "models/w_chemgun.mdl" );

    m_iDefaultAmmo = CHEMICALGUN_DEFAULT_GIVE;

    // Initialise chemistry defaults; save/restore will overwrite these on load.
    m_iAcid     = CG_DEFAULT_ACID;
    m_iNeutral  = CG_DEFAULT_NEUTRAL;
    m_iBase     = CG_DEFAULT_BASE;
    m_iPressure = CG_DEFAULT_PRESSURE;
    m_iMenuAxis = 0;

    FallInit();
}

void CChemicalGun::Precache( void )
{
PRECACHE_MODEL( "models/v_chemgun.mdl" );
PRECACHE_MODEL( "models/w_chemgun.mdl" );
PRECACHE_MODEL( "models/p_crossbow.mdl" );
PRECACHE_MODEL( "models/Tubeball.mdl" );

PRECACHE_SOUND( "weapons/cg-fire.wav"  );
PRECACHE_SOUND( "weapons/cgbounce.wav" );
PRECACHE_SOUND( "weapons/fizzy1.wav"   );
PRECACHE_SOUND( "weapons/sprog1.wav"   );
PRECACHE_SOUND( "weapons/sprog2.wav"   );
PRECACHE_SOUND( "weapons/DryFire.wav"  );

m_usChemicalGun = PRECACHE_EVENT( 1, "events/mp51.sc" );
}

int CChemicalGun::GetItemInfo( ItemInfo *p )
{
p->pszName   = STRING( pev->classname );
p->pszAmmo1  = "chemical_ammo";
p->iMaxAmmo1 = CHEMICAL_MAX_CARRY;
p->pszAmmo2  = NULL;
p->iMaxAmmo2 = -1;
p->iMaxClip  = WEAPON_NOCLIP;
p->iSlot     = 3;
p->iPosition = 1;
p->iFlags    = 0;
p->iId       = m_iId = WEAPON_CHEMICALGUN;
p->iWeight   = CHEMICALGUN_WEIGHT;

return 1;
}

int CChemicalGun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CChemicalGun::Deploy( )
{
    return DefaultDeploy( "models/v_chemgun.mdl", "models/p_crossbow.mdl",
        CHEMICALGUN_DRAW, "mp5" );
}

// -----------------------------------------------------------------------
// SecondaryAttack -- cycles through the four menu axes:
//   axis 0 (Acid)    : 0 -> 1 -> 2 -> 3 -> 4 -> 0
//   axis 1 (Neutral) : 0 -> 1 -> 2 -> 3 -> 4 -> 0
//   axis 2 (Base)    : 0 -> 1 -> 2 -> 3 -> 4 -> 0
//   axis 3 (Pressure): 1 -> 2 -> 3 -> 4 -> 5 -> 1
// Each press advances the current axis value and moves to the next axis.
// Plays the Changemixture animation + DryFire sound.
// -----------------------------------------------------------------------
void CChemicalGun::SecondaryAttack( void )
{
switch ( m_iMenuAxis % 4 )
{
case 0:
	m_iAcid = ( m_iAcid + 1 ) % 5;		// 0-4 wraps back to 0
	break;
case 1:
	m_iNeutral = ( m_iNeutral + 1 ) % 5;
	break;
case 2:
m_iBase = ( m_iBase + 1 ) % 5;
break;
case 3:
m_iPressure = ( m_iPressure % 5 ) + 1;// 1-5 wraps back to 1
break;
}
m_iMenuAxis++;

SendWeaponAnim( CHEMICALGUN_CUSTOMIZE );
EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM,
"weapons/DryFire.wav", 0.8f, ATTN_NORM );

m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
m_flNextPrimaryAttack   = UTIL_WeaponTimeBase() + 0.5f;
m_flTimeWeaponIdle      = UTIL_WeaponTimeBase() + 1.5f;
}

// -----------------------------------------------------------------------
// PrimaryAttack
// -----------------------------------------------------------------------
void CChemicalGun::PrimaryAttack( void )
{
// No firing underwater
if ( m_pPlayer->pev->waterlevel == 3 )
{
PlayEmptySound();
m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
return;
}

// All-zero chemistry -- nothing to fire
if ( m_iAcid == 0 && m_iNeutral == 0 && m_iBase == 0 )
{
PlayEmptySound();
m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + CG_FIRE_INTERVAL;
return;
}

// Derive ammo cost from chemistry settings
ChemConfig_t cfg = ChemConfig( m_iAcid, m_iNeutral, m_iBase );

if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < cfg.iAmmoTake )
{
PlayEmptySound();
m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
return;
}

m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;

m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
SendWeaponAnim( CHEMICALGUN_SHOOT );

LaunchChemBomb();

m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= cfg.iAmmoTake;

if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

m_flNextPrimaryAttack = m_flNextSecondaryAttack
= UTIL_WeaponTimeBase() + CG_FIRE_INTERVAL;
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + CG_FIRE_INTERVAL + 1.0f;
}

// -----------------------------------------------------------------------
// LaunchChemBomb -- builds chemistry config and spawns the projectile
// -----------------------------------------------------------------------
void CChemicalGun::LaunchChemBomb( void )
{
UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

Vector vecSrc = m_pPlayer->GetGunPosition()
+ gpGlobals->v_forward * 16.0f
+ gpGlobals->v_right   * 8.0f;

// Launch speed scales with Pressure axis
float flSpeed = CG_LAUNCH_BASE + CG_LAUNCH_SCALE * (float)m_iPressure;
Vector vecVel = gpGlobals->v_forward * flSpeed + m_pPlayer->pev->velocity;

ChemConfig_t cfg = ChemConfig( m_iAcid, m_iNeutral, m_iBase );
CChemBomb::CreateBomb( vecSrc, vecVel, m_pPlayer, cfg );

// Fire sound with pitch tinted by pressure (higher pressure = higher pitch)
EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON,
"weapons/cg-fire.wav", 1.0f, ATTN_NORM, 0,
90 + m_iPressure * 4 );

// View punch proportional to pressure
m_pPlayer->pev->punchangle.x = (float)( -m_iPressure );
}

void CChemicalGun::Reload( void )
{
// WEAPON_NOCLIP -- no physical clip to reload
}

void CChemicalGun::WeaponIdle( void )
{
ResetEmptySound();
m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
return;

float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
int   iAnim;

if ( flRand <= 0.5f )
{
iAnim = CHEMICALGUN_IDLE;
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
}
else
{
iAnim = CHEMICALGUN_FIDGET;
m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
}
SendWeaponAnim( iAnim, 1 );
}

void CChemicalGun::Holster( int skiplocal /* = 0 */ )
{
m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
SendWeaponAnim( CHEMICALGUN_HOLSTER );
}

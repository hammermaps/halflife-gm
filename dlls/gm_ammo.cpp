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
// Gunman Chronicles ammo pickup entities
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

// Ammo give amounts
#define AMMO_GAUSSCLIP_GIVE		20
#define AMMO_SHOTCYCLER_GIVE	8
#define AMMO_CHEMICAL_GIVE		30
#define AMMO_MINIGUN_GIVE		100
#define AMMO_DML_GIVE			4
#define AMMO_BEAMGUN_GIVE		20

//=========================================================
// Gauss Pistol ammo
//=========================================================
class CGaussClipAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/guassammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/guassammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_GAUSSCLIP_GIVE, "gausspistol_ammo", GAUSS_PISTOL_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussClipAmmo );

//=========================================================
// Shotcycler ammo
//=========================================================
class CShotCyclerAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/shotgunammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/shotgunammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_SHOTCYCLER_GIVE, "shotcycler_ammo", SHOTCYCLER_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_shotcycler, CShotCyclerAmmo );

//=========================================================
// Chemical ammo
//=========================================================
class CChemicalAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/chem_ammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/chem_ammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_CHEMICAL_GIVE, "chemical_ammo", CHEMICAL_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_chemical, CChemicalAmmo );

//=========================================================
// Minigun ammo
//=========================================================
class CMinigunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/mechammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/mechammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_MINIGUN_GIVE, "minigun_ammo", MINIGUN_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_minigun, CMinigunAmmo );

//=========================================================
// DML (rocket) ammo
//=========================================================
class CDMLAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/dmlammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/dmlammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_DML_GIVE, "dml_ammo", DML_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_dml, CDMLAmmo );

//=========================================================
// ammo_dmlclip  — 8-round Mule magazine pickup.
// Stock maps use this classname for the higher-capacity clip
// (distinct from the 4-round ammo_dml loose-round pickup).
//=========================================================
#define AMMO_DML_CLIP_GIVE  8

class CDMLClipAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/dmlammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/dmlammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_DML_CLIP_GIVE, "dml_ammo", DML_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_dmlclip, CDMLClipAmmo );

//=========================================================
// Beam gun ammo
//=========================================================
class CBeamGunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/beamgunammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/beamgunammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_BEAMGUN_GIVE, "beamgun_ammo", BEAMGUN_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_beamgun, CBeamGunAmmo );

//=========================================================
// GC Buckshot ammo (for weapon_gcshotgun)
//=========================================================
class CGCBuckshotAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/shotgunammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/shotgunammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_GCBUCKSHOT_GIVE, "buckshot", GC_BUCKSHOT_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gcbuckshot, CGCBuckshotAmmo );

// =========================================================
// Aliased ammo entity names
//
// The AngelScript scripts used different entity names for
// several ammo pickups.  Registering them here ensures maps
// that reference the original GC entity names also work.
// =========================================================

//=========================================================
// ammo_gcgaussclip  (alias for gauss pistol ammo)
// Uses the alternate model path from the Sven Co-op scripts.
//=========================================================
class CGCGaussClipAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/guassammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/guassammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_GAUSSCLIP_GIVE, "gausspistol_ammo", GAUSS_PISTOL_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gcgaussclip, CGCGaussClipAmmo );

//=========================================================
// ammo_gcminigunclip  (alias for minigun ammo)
// Uses the Gunman mechammo model.
//=========================================================
class CGCMinigunClipAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/mechammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/mechammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_MINIGUN_GIVE, "minigun_ammo", MINIGUN_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gcminigunclip, CGCMinigunClipAmmo );
// Stock GC maps spell the classname with a capital 'C' ("ammo_minigunClip").
// GoldSrc classname lookup is case-sensitive on Linux dedicated servers,
// so register an explicit alias so those map entities resolve.
LINK_ENTITY_TO_CLASS( ammo_minigunClip, CGCMinigunClipAmmo );
// Some maps also use the all-lowercase variant "ammo_minigunclip".
LINK_ENTITY_TO_CLASS( ammo_minigunclip, CGCMinigunClipAmmo );

//=========================================================
// ammo_dmlsingle  (alias for DML / rocket ammo)
// Uses the Gunman dmlrocket model; gives a single reload
// worth of DML rockets (2 rounds).
//=========================================================
#define AMMO_DMLSINGLE_GIVE		2

class CDMLSingleAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/dmlrocket.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/dmlrocket.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_DMLSINGLE_GIVE, "dml_ammo", DML_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_dmlsingle, CDMLSingleAmmo );

//=========================================================
// ammo_beamgunclip  (alias for beam gun ammo)
// Uses the Gunman beamgunammo model.
//=========================================================
class CBeamGunClipAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/beamgunammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/beamgunammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( AMMO_BEAMGUN_GIVE, "beamgun_ammo", BEAMGUN_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_beamgunclip, CBeamGunClipAmmo );

// =========================================================
// Gunman Chronicles item pickups
// =========================================================

//=========================================================
// item_gascan
//
// Gas canister pickup – replenishes Gauss Pistol ammunition.
// Model: models/gastank.mdl
//=========================================================
#define ITEM_GASCAN_GIVE	25

class CItemGasCan : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/gastank.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/gastank.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if ( pOther->GiveAmmo( ITEM_GASCAN_GIVE, "gausspistol_ammo", GAUSS_PISTOL_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_gascan, CItemGasCan );

//=========================================================
// item_armor
//
// Gunman Chronicles armor pickup.  Restores suit charge
// (armour value) to the player, capped at MAX_NORMAL_BATTERY.
// Model: models/W_armor.mdl
//=========================================================
#define ITEM_ARMOR_CHARGE	75

class CItemArmor : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT ArmorTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( item_armor, CItemArmor );

void CItemArmor::Precache( void )
{
	PRECACHE_MODEL( "models/W_armor.mdl" );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

void CItemArmor::Spawn( void )
{
	Precache();

	pev->solid    = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_TOSS;

	SET_MODEL( ENT( pev ), "models/W_armor.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CItemArmor::ArmorTouch );
}

void CItemArmor::ArmorTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() ) return;

	if ( pOther->pev->armorvalue >= MAX_NORMAL_BATTERY ) return;

	pOther->pev->armorvalue += (float)ITEM_ARMOR_CHARGE;
	if ( pOther->pev->armorvalue > MAX_NORMAL_BATTERY )
		pOther->pev->armorvalue = MAX_NORMAL_BATTERY;

	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

	SetTouch( NULL );
	UTIL_Remove( this );
}

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
// gm_monsters_stub.cpp
//
// Dummy entity registrations for all Gunman Chronicles monster
// classnames that appear in BSP maps but have no full C++
// implementation yet.  Each stub calls UTIL_Remove() immediately
// so the engine entity slot is freed; the entity is not drawn
// or simulated.
//
// All stubs are subclasses of CBaseMonster so the engine's
// monster-specific field handling (health, etc.) still works if
// a mapper needs to place them for future use.
//
// Entities listed here (from GUNMAN_BSP_ANALYSIS.md):
//   Human / NPC variants:
//     monster_human_bandit, monster_human_gunman,
//     monster_human_demoman, monster_human_scientist,
//     monster_human_unarmed, monster_human_chopper
//   Alien creatures:
//     monster_beak, monster_raptor, monster_microraptor,
//     monster_rustbit, monster_rustbot, monster_rustflier,
//     monster_rustgunr
//   Arthropods:
//     monster_scorpion, monster_largescorpion
//   Mechanical / special:
//     monster_sentry_mini
//   Worm/tube variants:
//     monster_tube, monster_tubequeen
//   Bosses and unique NPCs:
//     monster_aigirl, monster_ourano, monster_renesaur,
//     monster_endboss, monster_gator
//   Environmental / vehicles:
//     monster_targetrocket,
//     monster_trainingbot
//   Small critters:
//     monster_critter, monster_cricket, monster_dragonfly,
//     monster_flashlight, monster_hatchetfish, monster_maggot,
//     monster_darttrap
//   Embryo / spawner:
//     monster_xenome_embryo, monster_tube_embryo,
//     monster_beakbirther
//   Friendly variants:
//     monster_gunner_friendly, monster_rustbit_friendly,
//     monster_rustbot_friendly
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

//=========================================================
// Base stub class shared by all unimplemented monsters.
// Immediately removes itself on Spawn() so it does not
// occupy an entity slot or affect gameplay.
//=========================================================
class CGMMonsterStub : public CBaseMonster
{
public:
	void Spawn( void )
	{
		pev->effects = EF_NODRAW;
		pev->solid   = SOLID_NOT;
		UTIL_Remove( this );
	}

	int Classify( void ) { return CLASS_NONE; }
};

//=========================================================
// Human / NPC variants
//=========================================================
LINK_ENTITY_TO_CLASS( monster_human_bandit,    CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_human_gunman,    CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_human_demoman,   CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_human_scientist, CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_human_unarmed,   CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_human_chopper,   CGMMonsterStub );

//=========================================================
// Alien creatures
//=========================================================
LINK_ENTITY_TO_CLASS( monster_beak,       CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_raptor,     CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_microraptor,CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_rustbit,    CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_rustbot,    CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_rustflier,  CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_rustgunr,   CGMMonsterStub );

//=========================================================
// Arthropods
//=========================================================
LINK_ENTITY_TO_CLASS( monster_scorpion,      CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_largescorpion, CGMMonsterStub );

//=========================================================
// Mechanical / special
// Note: monster_sentry is already CSentry in turret.cpp
//=========================================================
LINK_ENTITY_TO_CLASS( monster_sentry_mini, CGMMonsterStub );

//=========================================================
// Worm / tube variants
//=========================================================
LINK_ENTITY_TO_CLASS( monster_tube,      CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_tubequeen, CGMMonsterStub );

//=========================================================
// Bosses and unique NPCs
//=========================================================
LINK_ENTITY_TO_CLASS( monster_aigirl,   CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_ourano,   CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_renesaur, CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_endboss,  CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_gator,    CGMMonsterStub );

//=========================================================
// Environmental / vehicles
// Note: monster_furniture is CFurniture in scripted.cpp (real implementation)
//=========================================================
LINK_ENTITY_TO_CLASS( monster_targetrocket,  CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_trainingbot,   CGMMonsterStub );

//=========================================================
// Small critters
//=========================================================
LINK_ENTITY_TO_CLASS( monster_critter,     CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_cricket,     CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_dragonfly,   CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_flashlight,  CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_hatchetfish, CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_maggot,      CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_darttrap,    CGMMonsterStub );

//=========================================================
// Embryo / spawner entities
//=========================================================
LINK_ENTITY_TO_CLASS( monster_xenome_embryo, CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_tube_embryo,   CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_beakbirther,   CGMMonsterStub );

//=========================================================
// Friendly variants
//=========================================================
LINK_ENTITY_TO_CLASS( monster_gunner_friendly,  CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_rustbit_friendly, CGMMonsterStub );
LINK_ENTITY_TO_CLASS( monster_rustbot_friendly, CGMMonsterStub );

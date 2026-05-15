AddCSLuaFile()

local decals = {
	{
		"gunman_gauss_pulse_hit", "gunman/decals/pulse"
	},
	{
		"gunman_gauss_rapid_hit", "gunman/decals/pulse"
	},	
	{
		"gunman_gauss_charge_hit", "gunman/decals/charge"
	},	
	{
		"gunman_beamgun_ball_hit", "gunman/decals/balburn"
	},	
	{
		"gunman_scorch1", "gunman/decals/scorch1"
	},	
	{
		"gunman_scorch2", "gunman/decals/scorch2"
	},	
	{
		"gunman_cg_red", "gunman/decals/cg_red"
	},	
	{
		"gunman_cg_lime", "gunman/decals/cg_lime"
	},	
	{
		"gunman_cg_green", "gunman/decals/cg_green"
	},	
	{
		"gunman_cg_brown", "gunman/decals/cg_brown"
	},	
}

local killicons = {
	{
		"gunman_weapon_gausspistol", "vgui/killicons/gausspistol_death",
	},
	{
		"gunman_weapon_gausspistol_rapid_projectile", "vgui/killicons/gauss_rapid_death",
	},
	{
		"gunman_weapon_gausspistol_charge_projectile", "vgui/killicons/gauss_charge_death",
	},
	{
		"gunman_weapon_beamgun", "vgui/killicons/beamgun_death",
	},
	{
		"gunman_weapon_beamgun_ball", "vgui/killicons/beamgun_ball_death",
	},
	{
		"gunman_weapon_beamgun_ball_small", "vgui/killicons/beamgun_ball_small_death",
	},
	{
		"gunman_weapon_beamgun_chain", "vgui/killicons/beamgun_chain_death",
	},
	{
		"gunman_weapon_chembomb", "vgui/killicons/chembomb_death",
	},
	{
		"gunman_weapon_shotgun", "vgui/killicons/shotgun_death",
	},
	{
		"gunman_weapon_mechagun", "vgui/killicons/mechagun_death",
	},
	{
		"gunman_weapon_knife", "vgui/killicons/knife_death",
	},	
	{
		"gunman_explosion", "vgui/killicons/explosion_death",
	},	
	{
		"gunman_explosion_small", "vgui/killicons/explosion_small_death",
	},	
	{
		"gunman_explosion_chem", "vgui/killicons/explosion_chem_death",
	},
	{
		"gunman_weapon_aicore", "vgui/killicons/aicore_death",
	},		
}

local defaultType = {
	name = "gunman_ammo_guassClip", 
	dmgtype = DMG_BULLET, 
	tracer = TRACER_LINE,
	plydmg = 0,
	npcdmg = 0, 
	force = 2000,
	maxcarry = 1, 
	minsplash = 10,
	maxsplash = 5,
}

local ammoTypes = {
	{
		name = "gunman_ammo_guassClip",
		maxcarry = 150, 
	},
	{
		name = "gunman_ammo_buckshot",
		maxcarry = 90, 
	},
	{
		name = "gunman_ammo_chemical",
		maxcarry = 50, 
	},
	{
		name = "gunman_ammo_beamgunClip",
		maxcarry = 100, 
	},
	{
		name = "gunman_ammo_minigunClip",
		maxcarry = 200, 
	},
	{
		name = "gunman_ammo_dmlclip",
		maxcarry = 8, 
	},
	{
		name = "gunman_ammo_dmlclip_primary",
		maxcarry = 2, 
	},
}

local WeaponSoundSettings = {
	channel = CHAN_WEAPON,
	pitch = { 100 },
	volume = VOL_NORM,
	soundlevel = SNDLVL_GUNFIRE,
}

local ItemSoundSettings = {
	channel = CHAN_ITEM,
	pitch = { 100 },
	volume = VOL_NORM,
	soundlevel = SNDLVL_GUNFIRE,
}

local AutoSoundSettings = {
	channel = CHAN_AUTO,
	pitch = { 100 },
	volume = VOL_NORM,
	soundlevel = SNDLVL_GUNFIRE,
}

local AutoChannel = {
	{
		name = "gunman_HudOn",
		sound = "gunman/weapons/wpn_hudon.wav",
	},
	{
		name = "gunman_HudOff",
		sound = "gunman/weapons/wpn_hudoff.wav",
	},
	{
		name = "gunman_debris",
		sound =  {"gunman/weapons/debris1.wav", "gunman/weapons/debris2.wav", "gunman/weapons/debris3.wav" },
		volume = {0.5, 0.7},
	},
	{
		name = "gunman_kabam",
		sound = {"gunman/weapons/kabam1.wav", "gunman/weapons/kabam2.wav", "gunman/weapons/kabam3.wav" },
	},
	{
		name = "gunman_kaboom",
		sound = {"gunman/weapons/kaboom1.wav", "gunman/weapons/kaboom2.wav", "gunman/weapons/kaboom3.wav" },
	},
	{
		name = "gunman_explode",
		sound = { "gunman/weapons/explode3.wav", "gunman/weapons/explode4.wav", "gunman/weapons/explode5.wav" },
	},
	{
		name = "gunman_dml_lock",
		sound = "gunman/weapons/dml_lock.wav",
	},
	{
		name = "gunman_mainframe_hurt",
		sound = "gunman/mainframe/mainframe_hurt02.wav",
	},
	{
		name = "gunman_mainframe_found_enemy",
		sound = "gunman/mainframe/mainframe007.wav",
	},
	{
		name = "gunman_mainframe_hit_enemy",
		sound = "gunman/mainframe/mainframe001.wav", 
	},
}

local WeaponChannel = {
	{
		name = "gunman_muleFire",
		sound = "gunman/weapons/dml_fire.wav",
		pitch = { 90, 110 },
	},	
	{
		name = "gunman_mineCharge",
		sound = "gunman/weapons/mine_charge.wav",
		volume = 0.5,
	},	
	{
		name = "gunman_gaussFirePulse",
		sound = "gunman/weapons/gauss_fire4.wav",
		pitch = { 96, 112 },
	},	
	{
		name = "gunman_gaussFireCharge",
		sound = "gunman/weapons/gauss_fire2.wav",
	},		
	{
		name = "gunman_gaussFireRapid",
		sound = "gunman/weapons/gauss_fire1.wav",
		pitch = { 100, 106 },
	},	
	{
		name = "gunman_gaussSniperZoom",
		sound = "gunman/weapons/gsnipe_zoom.wav",
	},	
	{
		name = "gunman_gaussSniperRunZoomFire",
		sound = "gunman/weapons/sniperunzoom.wav",
	},	
	{
		name = "gunman_gaussSniperZoomFire",
		sound = "gunman/weapons/sniperzoom.wav",
	},	
	{
		name = "gunman_shotgunFire",
		sound = "gunman/weapons/sbarrel1.wav",
	},	
	{
		name = "gunman_shotgunCock",
		sound = "gunman/weapons/shotgun_cock_heavy.wav",
	},	
	{
		name = "gunman_mechagunFire",
		sound = "gunman/weapons/hks3.wav",
	},	
	{
		name = "gunman_mechaSpinUp",
		sound = "gunman/weapons/MechaSpinUp.wav",
	},
	{
		name = "gunman_mechaSpinDown",
		sound = "gunman/weapons/MechaSpinDown.wav",
	},
	{
		name = "gunman_beamgun_taze",
		sound = "gunman/weapons/overheat.wav",
	},
	{
		name = "gunman_beamgun_windup",
		sound = "gunman/weapons/egon_windup2.wav",
		pitch = { 125 },
	},
	{
		name = "gunman_beamgun_windup2",
		sound = "gunman/weapons/egon_windup2_new.wav",
		pitch = { 125 },
	},
	{
		name = "gunman_beamgun_off",
		sound = "gunman/weapons/egon_off1.wav",
	},
	{
		name = "gunman_beamgun_electro",
		sound = {"gunman/weapons/electro4.wav", "gunman/weapons/electro5.wav", "gunman/weapons/electro6.wav"},
	},
	{
		name = "gunman_beamgun_config",
		sound = "gunman/weapons/beamgun_config.wav",
		pitch = { 125 },
	},
	{
		name = "gunman_DryFire",
		sound = "gunman/weapons/dryfire.wav",
	},
	{
		name = "gunman_dml_customize",
		sound = "gunman/weapons/dml_customize.wav",
	},
	{
		name = "gunman_dml_reload",
		sound = "gunman/weapons/dml_reload.wav",
		pitch = { 90, 110 },
		volume = 0.5,
	},
	{
		name = "gunman_dml_dualreload",
		sound = "gunman/weapons/dml_dualreload.wav",
		pitch = { 90, 110 },
		volume = 0.5,
	},
	{
		name = "gunman_rightpunch",
		sound = "gunman/weapons/rightpunch.wav",
		pitch = { 90, 110 },
	},
	{
		name = "gunman_hands_miss",
		sound = "gunman/weapons/cbar_miss1.wav",
		pitch = { 90, 110 },
	},
	{
		name = "gunman_hands_idlekickass",
		sound = "gunman/weapons/hands_idlekickass_f0.wav",
	},
	{
		name = "gunman_hands_knifeattack1",
		sound = "gunman/weapons/knifeattack1.wav",
		pitch = { 90, 110 },
	},
	{
		name = "gunman_hands_knifeattack2",
		sound = "gunman/weapons/knifeattack2.wav",
		pitch = { 90, 110 },
	},
	{
		name = "gunman_hands_knifedraw",
		sound = "gunman/weapons/knifedraw.wav",
	},
	{
		name = "gunman_hands_knifeholster",
		sound = "gunman/weapons/knifeholster.wav",
	},
	{
		name = "gunman_chemgun_fizzy",
		sound = "gunman/weapons/fizzy1.wav",
	},
	{
		name = "gunman_chemgun_sprog1",
		sound = "gunman/weapons/sprog1.wav",
	},
	{
		name = "gunman_chemgun_sprog2",
		sound = "gunman/weapons/sprog2.wav",
	},
	{
		name = "gunman_chemgun_cgbounce",
		sound = "gunman/weapons/cgbounce.wav",
	},
	{
		name = "gunman_chemgun_cg-fire",
		sound = "gunman/weapons/cg-fire.wav",
	},
	{
		name = "gunman_chemgun_empty",
		sound = "gunman/weapons/empty.wav",
	},
	{
		name = "gunman_chemgun_customize",
		sound = "gunman/weapons/chemgun_customize.wav",
	},
	{
		name = "gunman_mainframe_aiplug",
		sound = "gunman/mainframe/aiplug_deactivate_gs.wav",
	},
}

local ItemChannel = {
	{
		name = "gunman_muleRocketFly",
		sound = "gunman/weapons/rocket1.wav",
	},	
	{
		name = "gunman_mineActive",
		sound = "gunman/weapons/mine_activate.wav",
	},	
	{
		name = "gunman_mineDeploy",
		sound = "gunman/weapons/mine_deploy.wav",
	},	
	{
		name = "gunman_beamgun_residual",
		sound = {"gunman/weapons/residual1.wav", "gunman/weapons/residual2.wav", "gunman/weapons/residual3.wav", "gunman/weapons/residual4.wav"}
	},
	{
		name = "gunman_gunPickup",
		sound = "gunman/weapons/gunpickup2.wav",
	},
	{
		name = "gunman_ammoPickup",
		sound = "gunman/items/9mmclip1.wav",
	},
	{
		name = "gunman_dml_fragment",
		sound = "gunman/weapons/dml_fragment.wav",
	},
	{
		name = "gunman_GrenadeHit",
		sound = {"gunman/weapons/grenade_hit1.wav", "gunman/weapons/grenade_hit2.wav", "gunman/weapons/grenade_hit3.wav"},
	},
	{
		name = "gunman_sshell",
		sound = {"gunman/weapons/sshell1.wav", "gunman/weapons/sshell2.wav", "gunman/weapons/sshell3.wav"},
	},
	{
		name = "gunman_gaussPistol_spriteHit",
		sound = "gunman/weapons/gauss_spritesmall.wav",
	},
	{
		name = "gunman_gaussPistol_spriteBigHit",
		sound = "gunman/weapons/gauss_spritebig.wav",
	},
	{
		name = "gunman_beamgun_ballfly",
		sound = "gunman/weapons/ball-fly.wav",
	},
	{
		name = "gunman_beamgun_balldie",
		sound = "gunman/weapons/ball-die.wav",
	},
	{
		name = "gunman_aicore_deactivate",
		sound = "gunman/mainframe/aiplug_deactivate_gs.wav",
	},
	{
		name = "gunman_aicore_activate",
		sound = "gunman/mainframe/aiplug_activate_gs.wav",
	},	
	{
		name = "gunman_aicore_activated",
		sound = "gunman/mainframe/rebar_psychomotorstartup.wav",
	},	
}

local DefaultWeaponSettings = {
    GaussPistol = {
        FireMode = 1,
    },

    Mule = {
        LaunchType 	   = 1,
        FlightPathType = 1,
        DetonationType = 1,
        PayloadType    = 1,
    },

    Chemgun = {
        Acid 	 = 4,
        Neutral  = 2,
        Base 	 = 3,
        Pressure = 3,
    },

    Grenade = {
        DetonationType = 2,
        PayloadType    = 1,
    },
	
    Beamgun = {
        Range 			 = 3,
        PowerAndAccuracy = 2,
        Lightning 		 = 1,
    },
}

local WeaponAmmoUsage = {
    GaussPistol = {
        Pulse  = 1,
        Rapid  = 1,
        Charge = 10,
        Sniper = 10,
        SniperZoomed = 20,
    },
    Shotgun = {
        ShellLevels = { 1, 2, 3, 4 },
    },
    Mule = {
        LaunchStandard = 1,
        LaunchSpiral   = 2,
        ReloadSingle   = 1,
        ReloadDouble   = 2,
    },
    Grenade = {
        Standard = 1,
    },
    Mechagun = {
        Standard = 1,
    },
    Beamgun = {
        Standard = 1,
        PowerBall = 20,
    },
    Chemgun = {
        Min = 1,
        Max = 3,
    }
}

local WeaponDamage = {
    GaussPistol = {
        Pulse 		= 8,
        Rapid 		= 40,
        Charge 		= 100,
        Sniper 		= 100,
        SniperQuick = 40,
    },

	Beamgun = {
        Standard   = 8,
		TazerBurst = 160,
        Modes 	   = { 7, 10, 12, 15 },
    },

	Knife = {
        Hands = 10,
        Knife = 30,
    },

    AICore = {
        Bullet = 100,
    },

    Shotgun  = 7,
    Mechagun = 14,
    Mule 	 = 8,
    Grenade  = 8,
    Chemgun  = 8,
}

local EntityDamage = {
    GaussPistol = {
        Rapid  = { Touch = 13, Blast = 26,  Radius = 16 },
        Charge = { Touch = 50, Blast = 100, Radius = 96 },
    },

    Beamgun = {
        Ball = 		{ Touch = 100, Blast = 100, Radius = 128, ThinkBlast = 10, ThinkRadius = 128 },
        BallSmall = { Touch = 20,  Blast = 10,  Radius = 64 },
        Chain = 5,
    },
	
    Explosions = {
        Standard = { Damage = 256, Radius = 512 },
        Small =    { Damage = 60,  Radius = 256 },
        Chem = 	   { Damage = 60,  Radius = 256 },
    }
}

return {
	WeaponDamage = WeaponDamage,
	EntityDamage = EntityDamage,
	DefaultWeaponSettings = DefaultWeaponSettings,
	decals = decals,
	WeaponSoundSettings = WeaponSoundSettings,
	ItemSoundSettings = ItemSoundSettings,
	AutoSoundSettings = AutoSoundSettings,
	AutoChannel = AutoChannel,
	WeaponChannel = WeaponChannel,
	ItemChannel = ItemChannel,
	defaultType = defaultType,
	ammoTypes = ammoTypes,
	killicons = killicons,
	WeaponAmmoUsage = WeaponAmmoUsage
}
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "Chemical Launcher"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModel	= "models/gunman/v_chemgun.mdl"
SWEP.ViewModelFOV = 100
SWEP.WorldModel = "models/gunman/w_chemgun.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 3
SWEP.SlotPos = 1

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "pistol"
SWEP.DeploySequence = "draw"
SWEP.IdleSequences = { "idle", "idlefidget" }

SWEP.Primary.ClipSize = 50
SWEP.Primary.DefaultClip = 25
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = true
SWEP.Primary.Ammo = "gunman_ammo_chemical"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Chemgun
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "none"
SWEP.Secondary.Automatic = false

SWEP.isFiring = false
SWEP.isFiringDelay = 1

SWEP.FireModeMenuToggle = false
SWEP.FireModeMenuOption = 1

SWEP.CustomizeAnimation = "Changemixture"
SWEP.CustomizeSound = "gunman_chemgun_customize"
SWEP.CustomizeSoundDelay = 0.8

SWEP.MenuOptions = {
	[1] = {var = "CurrentAcid",     min = 0, max = 4},
	[2] = {var = "CurrentNeutral",  min = 0, max = 4},
	[3] = {var = "CurrentBase",     min = 0, max = 4},
	[4] = {var = "CurrentPressure", min = 1, max = 5},
}

SWEP.CurrentAcid = GunmanShared.DefaultWeaponSettings.Chemgun.Acid
SWEP.CurrentNeutral = GunmanShared.DefaultWeaponSettings.Chemgun.Neutral
SWEP.CurrentBase = GunmanShared.DefaultWeaponSettings.Chemgun.Base
SWEP.CurrentPressure = GunmanShared.DefaultWeaponSettings.Chemgun.Pressure

SWEP.ammoCost = 1

SWEP.chemDefault = {
	skin = 0,
	explode = false,
	bounce = false,
	stick = false,
	smokeBurn = false,
	airExpireTime = 8,
	damageArea = 32,
	ammoTake = 1,
}

function SWEP:ChemConfig(base, neutral, acid)
    local cfg = {
        skin = 0,
        damageArea = 32,
        ammoTake = 1,
        explode = false,
        bounce = false,
        stick = false,
        smokeBurn = false,
        airExpireTime = 4
    }

    if acid > 0 and base > 0 then
        cfg.explode = true
    end

    if cfg.explode and (neutral > acid or neutral > base) then
        cfg.bounce = true
    end

    if neutral > 2 and (acid > 0 or base > 0) and not cfg.bounce then
        cfg.stick = true
    end

    if neutral > 2 then
        if acid > 2 or (base > 0 and not cfg.explode) then
            cfg.smokeBurn = true
        end
    end

    if cfg.explode then
        cfg.airExpireTime = math.max(1, 5 - neutral)
    else
        cfg.airExpireTime = 4
    end

    if cfg.smokeBurn then
        cfg.damageArea = 64
    elseif cfg.explode then
        cfg.damageArea = math.Clamp(16 * (acid + base), 32, 128)
    else
        cfg.damageArea = math.Clamp(32 * math.max(acid, base, neutral), 32, 128)
    end

    if base > 0 and acid > 0 then
        cfg.skin = 2
    elseif base > 0 and neutral > base and acid == 0 then
        cfg.skin = 3
    elseif neutral > base and neutral > acid then
        cfg.skin = 1
    else
        cfg.skin = 0
    end
	
    cfg.ammoTake = math.Clamp(math.floor(1 + ((base + neutral + acid) / 6)), GunmanShared.WeaponAmmoUsage.Chemgun.Min, GunmanShared.WeaponAmmoUsage.Chemgun.Max)

    return cfg
end

function SWEP:ChemProjectile(projectile_effect)
	if not SERVER then return end

	local trace = self.Owner:GetEyeTrace()
    local targetPos = trace.HitPos

    local spawnPos = GunmanShared.ClampedTracerHitPos(self, 32) + self.Owner:GetRight() * 8

	local baseDirection = (targetPos - spawnPos):GetNormalized()
	local finalDirection = baseDirection:Angle()

	local ent = GunmanShared.EntityCreate(self, projectile_effect, spawnPos)

	local config = self:ChemConfig(self.CurrentBase, self.CurrentNeutral, self.CurrentAcid)

	if config then
		ent:SetSkin(          config.skin          or self.chemDefault.skin)
		ent.explode 	    = config.explode       or self.chemDefault.explode
		ent.bounce          = config.bounce        or self.chemDefault.bounce
		ent.stick           = config.stick         or self.chemDefault.stick
		ent.smokeBurn       = config.smokeBurn     or self.chemDefault.smokeBurn
		ent.airExpireTime   = config.airExpireTime or self.chemDefault.airExpireTime 
		ent.damageArea 	    = config.damageArea    or self.chemDefault.damageArea 
		
		self.ammoCost       = config.ammoTake      or self.chemDefault.ammoTake
		self.Owner:RemoveAmmo(config.ammoTake      or self.chemDefault.ammoTake, self.Primary.Ammo)
	end

	ent:GetPhysicsObject():ApplyForceCenter(finalDirection:Forward() * 1000 * self.CurrentPressure)
end

function SWEP:LaunchChemBall()
	local cone = -self.CurrentPressure
	self.Owner:ViewPunch(Angle(cone, cone * 0.5, 0))

	if self.CurrentPressure > 1 and self.Owner:GetVelocity():Length() < 10 then
		local kickbackForce = 40 * self.CurrentPressure
		local direction = -self.Owner:GetForward() 
		self.Owner:SetVelocity( self.Owner:GetVelocity() + direction * kickbackForce)
	end

	self:ChemProjectile("gunman_weapon_chembomb")
end

function SWEP:PrimaryAttack()
	if not self:IsMenuOpen() then 
		self.isFiring = true

		if self:Ammo1() <= self.ammoCost or self.Owner:WaterLevel() == 3 then 
			self:DryFire() 
			return 
		end
		
		timer.Simple(2, function() 
			self.isFiring = false
		end)
		
		self.Owner:SetAnimation( PLAYER_ATTACK1 )
		self:playSequence("Shoot")

		if self.CurrentBase == 0 and self.CurrentNeutral == 0 and self.CurrentAcid == 0 then 
			self.Owner:EmitSound("gunman_chemgun_empty")
			self:SetNextPrimaryFire(CurTime() + 0.8)
			return 
		end

		self:LaunchChemBall()
		self.Owner:EmitSound("gunman_chemgun_cg-fire")
		self:SetNextPrimaryFire(CurTime() + 0.8)
	else
		self:HudEmitSound("gunman_HudOff")
		self:SetNextPrimaryFire(CurTime() + 0.5)	
	end
		
	self:SetNW2Bool("FireModeMenuToggle", false)
end

function SWEP:SecondaryAttack()
	if not self:IsMenuOpen() then
		self:HudEmitSound("gunman_HudOn")
	end

	self:SetNW2Bool("FireModeMenuToggle", true)

	self:SetNextSecondaryFire(CurTime() + 0.5)	
end

function SWEP:CustomThink()
	self:SetClip1(self:Ammo1())
end

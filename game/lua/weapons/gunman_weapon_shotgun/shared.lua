AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "Shotgun"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModelFOV = 100
SWEP.ViewModel	= "models/gunman/v_shotgun.mdl"
SWEP.WorldModel = "models/gunman/w_shotgun.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 2
SWEP.SlotPos = 0

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "shotgun"

SWEP.Primary.Sound = Sound( "gp_fire1" )
SWEP.Primary.ClipSize = 90
SWEP.Primary.DefaultClip = 16
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = false
SWEP.Primary.Ammo = "gunman_ammo_buckshot"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Shotgun
SWEP.Primary.Spread = 0
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.NumberofShots = 1
SWEP.Primary.Force = 4

SWEP.Secondary.Ammo = "none"

SWEP.isFiring = false
SWEP.isFiringDelay = 1

SWEP.CustomizeAnimation = "customize"
SWEP.CustomizeSound = "gunman_shotgunCock"
SWEP.CustomizeSoundDelay = 0.8

SWEP.FireModes = { 
	ShellUsage   = 1, 
	SpreadAdjust = 2,  
} 

SWEP.ammoCosts = GunmanShared.WeaponAmmoUsage.Shotgun.ShellLevels

SWEP.FireSpread = { 
	Rifle   = 1, 
	Shotgun = 2, 
	Riotgun = 3,  
}  

SWEP.fireAnimations = {
	"shoot",
	"shoot2",
	"shoot3",
	"shoot4",		
}

SWEP.FireModeMenuToggle = false
SWEP.keyIsDown = false

SWEP.CurrentShellCost = 2	
SWEP.CurrentWeaponSpread = 2
SWEP.FireModeMenuOption = 1

SWEP.IdleSequences = { "idle", "idleinspect" }

SWEP.MenuOptions = {
	[1] = {var = "CurrentShellCost",    min = 1, max = 4},
	[2] = {var = "CurrentWeaponSpread", min = 1, max = 3},
}

function SWEP:CustomDeploy()
	return true
end

function SWEP:CustomHolster()
	return true
end

function SWEP:FireAnimation(anim, rate)
	local vm = self.Owner:GetViewModel()
    if IsValid(vm) then
        vm:SendViewModelMatchingSequence(vm:LookupSequence(anim))
		vm:SetPlaybackRate(rate)
    end
end

function SWEP:DropShell()
    if CLIENT then return end 

    local rightVector = self.Owner:GetRight()  
    local forwardVector = self.Owner:GetForward() 
    local upVector = self.Owner:GetUp()  

    local spawnPos = self.Owner:EyePos() 
        + rightVector * 16      
        + forwardVector * 32 
        + upVector * -4           

    local vm = self.Owner:GetViewModel()
    if not IsValid(vm) then return end
    
    local effect = EffectData()
    effect:SetEntity(self.Owner) 
    effect:SetOrigin(spawnPos)    
    
    local ejectionAngle = self.Owner:EyeAngles()  
    ejectionAngle:RotateAroundAxis(self.Owner:GetRight(), 140)
    ejectionAngle.pitch = -30  
    
    effect:SetAngles(ejectionAngle)

	for i = 1, self.CurrentShellCost do 
    	util.Effect("ShotgunShellEject", effect, true, true)
	end
end

function SWEP:FireShell()
	self.isFiringDelay = 2

	if SERVER then
		self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Shotgun.ShellLevels[self.CurrentShellCost], self.Primary.Ammo)
	end
	
	local cone = math.Rand(0.02, 0.04) * (self.CurrentWeaponSpread ^ 1.5)

	local bullet = {}
	bullet.Num = 5 * self.CurrentShellCost
	bullet.Src = self.Owner:GetShootPos()
	bullet.Dir = self.Owner:GetAimVector()
	bullet.Spread = Vector(cone, cone, 0)
	bullet.Tracer = 0
	bullet.Force = self.Primary.Force
	bullet.Damage = self.Primary.Damage
	bullet.AmmoType = self.Primary.Ammo

	self.Owner:FireBullets(bullet)
end

function SWEP:ShootMuzzleEffect()
	local effectdata = EffectData()
	effectdata:SetOrigin(self:GetPos())
	effectdata:SetMagnitude(0.1)
	
	if IsFirstTimePredicted() then
		util.Effect("ElectricSpark", effectdata) -- fixes muzzleflash not showing correctly
	end

    local effect = EffectData()
    effect:SetEntity(self.Owner)   
	effect:SetFlags(30)
	effect:SetScale(self.CurrentShellCost ^ 0.5)

	if IsFirstTimePredicted() then
		util.Effect("gunman_shotgun_muzzleflash", effect)
	end
end

function SWEP:PrimaryAttack()
	if not self:IsMenuOpen() then
		self.isFiring = true

		timer.Remove("cock")

		timer.Simple(self.isFiringDelay, function() 
			self.isFiring = false
		end)

		if self:Ammo1() < self.ammoCosts[self.CurrentShellCost] or self.Owner:WaterLevel() == 3 then
			self:DryFire()
			return 
		end

		self:FireAnimation(self.fireAnimations[self.CurrentShellCost], 1.0)
		
		self:ShootMuzzleEffect()
		self:FireShell()
		self:DropShell()

		self.Owner:EmitSound("gunman_shotgunFire")

		self.Owner:SetAnimation(PLAYER_ATTACK1)

		self.Owner:ViewPunch(Angle(
			math.Rand(-1.5,-2) * (self.CurrentShellCost ^ 1.3), 
			math.Rand(0.5, 1) * (self.CurrentShellCost ^ 1.2), 0))

		self:SetNextPrimaryFire(CurTime() + 1)
	else
		self:HudEmitSound("gunman_HudOff")
		self:SetNextPrimaryFire(CurTime() + 0.2)
	end
	
	self:SetNW2Bool("FireModeMenuToggle", false)
end

function SWEP:SecondaryAttack()
	if not self:IsMenuOpen() then
		self:HudEmitSound("gunman_HudOn")
	end
	
	self:SetNW2Bool("FireModeMenuToggle", true)
end

function SWEP:CustomThink()
	self:SetClip1(self:Ammo1())
end
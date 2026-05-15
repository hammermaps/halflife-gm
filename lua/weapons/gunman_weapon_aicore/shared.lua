AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "AI Core"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModel	= "models/gunman/v_aicore.mdl"
SWEP.ViewModelFOV = 100
SWEP.WorldModel = "models/gunman/w_aicore.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 5
SWEP.SlotPos = 0

SWEP.DrawCrosshair = false
SWEP.DrawAmmo = true

SWEP.HoldType = "melee"
SWEP.DeploySequence = "coredraw"
SWEP.IdleSequences = { "coreidle" }

SWEP.Primary.ClipSize = -1
SWEP.Primary.DefaultClip = -1
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = false
SWEP.Primary.Ammo = "none"
SWEP.Primary.Damage = 0
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "none"
SWEP.Secondary.Automatic = false

SWEP.isFiring = false

function SWEP:DoImpactEffect(tr, dmgtype)
	local effectdata = EffectData()
	effectdata:SetOrigin(tr.HitPos)
	effectdata:SetMagnitude(1)

	util.Effect( "cball_explode", effectdata )	
	
	return true
end

function SWEP:Hit()
	local bullet = {}
	bullet.Num = 10
	bullet.Src = self.Owner:GetShootPos()
	bullet.Dir = self.Owner:GetAimVector()
	bullet.Spread = Vector(0.1, 0.1)
	bullet.Tracer = 0
	bullet.Force = 100
	bullet.Damage = GunmanShared.WeaponDamage.AICore.Bullet
	bullet.AmmoType = "none"

	local maxRange = 128
	
	local eyePos = self.Owner:EyePos()
	local tr = self.Owner:GetEyeTrace()
	local hitPos = tr.HitPos 

	local dir = (hitPos - eyePos):GetNormalized() 
	local distance = (hitPos - eyePos):Length()
	
	if distance < maxRange then
		self.Owner:FireBullets(bullet)

		self.Owner:EmitSound("gunman_beamgun_taze")
		
		if tr.Hit and IsValid(tr.Entity) then
			if tr.Entity:GetClass() == "gunman_aiwallplug" then
				self.Owner:StopSound("gunman_beamgun_taze")

				if not SERVER then return end
				timer.Simple(0.1, function() 
					self.Owner:StripWeapon(self:GetClass()) 
				end)
				
				return 
			end
		end
	end
end

function SWEP:PrimaryAttack()
	self.isFiring = true

	timer.Simple(2, function() 
		self.isFiring = false
	end)
		
	timer.Simple(0.6, function() 
		self.Owner:SetAnimation(PLAYER_ATTACK1)
		self:Hit()
	end)

	self:playSequence("coreplugin")

	self:SetNextPrimaryFire(CurTime() + 1)
end

function SWEP:SecondaryAttack()

end

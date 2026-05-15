AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "Mechagun"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModelFOV = 100
SWEP.ViewModel	= "models/gunman/v_mechagun.mdl"
SWEP.WorldModel = "models/gunman/w_mechagun.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 2
SWEP.SlotPos = 1

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "ar2"

SWEP.NextAnimTime = 1
SWEP.NextTempThinkTime = 1
SWEP.NextMalfunctionThinkTime = 1
SWEP.NextThinkClientTime = 1

SWEP.Primary.ClipSize = 200
SWEP.Primary.DefaultClip = 30
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = true
SWEP.Primary.Ammo = "gunman_ammo_minigunClip"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Mechagun
SWEP.Primary.Spread = 0
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.NumberofShots = 1
SWEP.Primary.Force = 5

SWEP.Secondary.Ammo = "none"

SWEP.isFiring = false
SWEP.isFiringDelay = 1

SWEP.TurboModeSequenceEnabled = false

SWEP.weaponTemp = 0
SWEP.Malfunction = false
SWEP.Malfunctioned = false

SWEP.muzzleFlashLifeTime = CurTime()

SWEP.FiringEnabled = false

SWEP.IdleSequences = { "idle", "idleinspect" }
SWEP.DeploySequence = "arming"

function SWEP:CustomInitialize()
	if SERVER then
		util.AddNetworkString("TriggerMuzzleFlash" .. tostring(self))
	end
end

function SWEP:CustomDeploy()
	self.FiringEnabled = true 
	return true
end

function SWEP:CustomHolster()
	self.FireModeTurbo = false
	self.TurboModeSequenceEnabled = false
	return true
end

function SWEP:DropShell()
    if CLIENT then return end 

    local rightVector = self.Owner:GetRight()  
    local forwardVector = self.Owner:GetForward() 
    local upVector = self.Owner:GetUp()  

    local spawnPos = self.Owner:EyePos() 
        + rightVector * 20      
        + forwardVector * 24 
        + upVector * -6           

    if not IsValid(self.CurrentViewModel) then return end
    
    local effect = EffectData()
    effect:SetEntity(self.Owner) 
    effect:SetOrigin(spawnPos)    
    
    local ejectionAngle = self.Owner:EyeAngles()  
    ejectionAngle:RotateAroundAxis(self.Owner:GetRight(), 140)
    ejectionAngle.pitch = -30  
    
    effect:SetAngles(ejectionAngle)

    util.Effect("RifleShellEject", effect, true, true)
end

function SWEP:PrepareBullet()
	local cone = math.Rand(0.02, 0.04) 

	local bullet = {}
	bullet.Num = 1
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
    local effect = EffectData()
    effect:SetEntity(self.Owner)   
	effect:SetFlags(32)  
	effect:SetScale(1)

	if IsFirstTimePredicted() then
		util.Effect("gunman_mechagun_muzzleflash", effect)
	end

	if self.FireModeTurbo then
		effect:SetFlags(36)  

		if IsFirstTimePredicted() then
    		util.Effect("gunman_mechagun_muzzleflash", effect)
		end
	end
end

function SWEP:FireWeapon()
	self:PrepareBullet()
	self:DropShell()
	self:ShootMuzzleEffect()

	local effectdata = EffectData()
	effectdata:SetOrigin(self:GetPos())
	effectdata:SetMagnitude(0)

	if IsFirstTimePredicted() then
		util.Effect("ElectricSpark", effectdata) -- fixes muzzleflash not showing correctly
	end

	self.Owner:EmitSound("gunman_mechagunFire")
	self.Owner:SetAnimation(PLAYER_ATTACK1)

	local cone = math.random(2, -2) * self.Primary.TakeAmmo

	self.Owner:ViewPunch(Angle(cone, 0, 0))

	if SERVER then
		self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Mechagun.Standard, self.Primary.Ammo)
	end
end

function SWEP:PrimaryAttack()
	if self.FiringEnabled == true then
		self.isFiring = true

		timer.Simple(self.isFiringDelay, function() 
			self.isFiring = false
		end)

		if self:Ammo1() <= 0 or self.Owner:WaterLevel() == 3 then
			self:DryFire()

			if self.FireModeTurbo == true then
				self:SecondaryAttack()	
			end
			
			return 
		end

		if self.TurboModeSequenceEnabled == false then
			self:playSequence("firenormal")
			self.allowFiring = true

			self:SetNextPrimaryFire(CurTime() + 0.2)
		else
			self:SetNextPrimaryFire(CurTime() + 0.1)
		end

		if self.allowFiring == true then
			self:FireWeapon()
		end
	else
		self:SetNextPrimaryFire(CurTime() + 0.2)	
	end
end

function SWEP:SecondaryAttack()
	if self.FiringEnabled == true then 

		self.FireModeTurbo = not self.FireModeTurbo
		self:SetNW2Bool("FireModeTurbo", self.FireModeTurbo)
	
		if self.FireModeTurbo == true then
			self.FiringEnabled = false
			self:HudEmitSound("gunman_HudOn")
			self.Owner:EmitSound("gunman_mechaSpinUp")
			
			self:playSequence("spinup")

			timer.Simple(self:sequenceLength("spinup") - 0.15, function() 
				self.TurboModeSequenceEnabled = true
				self.FiringEnabled = true
			end)
		else
			self.FiringEnabled = false
			self.TurboModeSequenceEnabled = false
			self:HudEmitSound("gunman_HudOff")
			self.Owner:EmitSound("gunman_mechaSpinDown")

			self:playSequence("spindown")

			self.NextIdleTime = CurTime() + self:sequenceLength("spindown") + 1

			-- Spins Gun Barrel 180 Degree in 1 frame so it's less noticeable 
			timer.Simple(self:sequenceLength("spindown"), function() 
				self.CurrentViewModel:SetPlaybackRate(-1000.0)
				self.CurrentViewModel:SetSequence("spindown")
				self.FiringEnabled = true

				timer.Simple(0.1, function()
					self:playSequence("idle")
				end)
			end)
		end
	end

	self:SetNextPrimaryFire(CurTime() + 0.5)	
end

function SWEP:UpdateWeaponTemperature()
    if self.weaponTemp <= 162 and self.Owner:KeyDown(IN_ATTACK) and self.TurboModeSequenceEnabled then
        self.weaponTemp = self.weaponTemp + 2
    elseif self.weaponTemp > 0 then
        self.weaponTemp = self.weaponTemp - 1
    end
end

function SWEP:MalfunctionPoint()
    if self.weaponTemp >= 162 then
        self.Malfunction = true
    end
end

function SWEP:MalfunctionReset()
	self.Malfunctioned = false
	self.TurboModeSequenceEnabled = false
	self:SecondaryAttack()
	self:SetNextSecondaryFire(CurTime() + 3)	
end

function SWEP:MalfunctionState()
	self:SetNW2Bool("WeaponMalfunctioned", true)

	self.weaponTemp = 160

	if self.Malfunctioned == false then
		self:PrimaryAttack()

		timer.Simple(5, function() 
			self.Malfunction = false 
			self.Malfunctioned = true
		end)
	end
end

function SWEP:CustomThink()
	self:SetClip1(self:Ammo1())

	if SERVER then
		self:SetNW2Bool("FiringEnabled", self.FiringEnabled)
		self:SetNW2Bool("allowFiring", self.allowFiring)
	else
		self.FiringEnabled = self:GetNW2Bool("FiringEnabled", false)
		self.allowFiring = self:GetNW2Bool("allowFiring", false)
	end

	if SERVER then
		if CurTime() > self.NextAnimTime then
			if self.TurboModeSequenceEnabled == true then
				local isFiring = (self.Owner:KeyDown(IN_ATTACK) or self.Malfunction) and self.Weapon:Ammo1() > 0
				local sequence = isFiring and "fireloop" or "idleloop"

				self:playSequence(sequence)
				self.NextAnimTime = CurTime() + self:sequenceLength(sequence) - 0.4

				self.allowFiring = isFiring
				self:SetNW2Bool("TurboModeSequenceFire", isFiring)

				if self.Malfunctioned or (isFiring and self.Weapon:Ammo1() < 1) then
					self:MalfunctionReset()
				end
			end
		end
		
		if CurTime() > self.NextTempThinkTime then
			self:UpdateWeaponTemperature()
			self:MalfunctionPoint()
			
			self:SetNW2Int("WeaponTemp", self.weaponTemp)

			if self.Malfunction == true then
				self:MalfunctionState()
			else
				self.Malfunctioned = false
				self:SetNW2Bool("WeaponMalfunctioned", false)
			end

			self.NextTempThinkTime = CurTime() + 0.1
		end
	else
		if not game.SinglePlayer() then 
			if CurTime() > self.NextMalfunctionThinkTime then
				if self.Malfunction == true then
					self:PrimaryAttack()
				end
				
				self.NextMalfunctionThinkTime = CurTime() + 0.1
			end
		end
	end
end

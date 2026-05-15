AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "Beamgun"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModel	= "models/gunman/v_beam.mdl"
SWEP.ViewModelFOV = 100
SWEP.WorldModel = "models/gunman/w_beam.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 3
SWEP.SlotPos = 0

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "pistol"
SWEP.IdleSequences = { "idleblade", "idleinspect", "idlesubduel" }

SWEP.NextTazerFireTime = 1

SWEP.Primary.ClipSize = 100
SWEP.Primary.DefaultClip = 40
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = true
SWEP.Primary.Ammo = "gunman_ammo_beamgunClip"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Beamgun.Standard
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "none"
SWEP.Secondary.Automatic = false

SWEP.isFiring = false

SWEP.FireModeMenuToggle = false

SWEP.FireModeMenuOption = 1
SWEP.FireModeMenuIsOn = false
SWEP.delayMalfunctionReset = 0

SWEP.weaponTemp = 0
SWEP.Malfunction = false
SWEP.Malfunctioned = false
SWEP.CurrentViewModel = nil

SWEP.ChargeTime = 0.75
SWEP.ChargeStartTime = 0 
SWEP.IsCharging = false 
SWEP.HasFired = false 

SWEP.CurrentRange = GunmanShared.DefaultWeaponSettings.Beamgun.Range
SWEP.CurrentPowerAndAccuracy = GunmanShared.DefaultWeaponSettings.Beamgun.PowerAndAccuracy
SWEP.CurrentLightning = GunmanShared.DefaultWeaponSettings.Beamgun.Lightning
SWEP.NextChainSpawnTime = 2
SWEP.NextBallLaunchTick = 1
SWEP.BallLaunched = true

SWEP.gotTazerEntity = false

SWEP.BallWasLaunched = false
SWEP.DelayBallLaunch = 0

SWEP.CustomizeAnimation = "config"

SWEP.MenuOptions = {
	[1] = {var = "CurrentRange",            max = 4},
	[2] = {var = "CurrentPowerAndAccuracy", max = 4},
	[3] = {var = "CurrentLightning",        max = 3}
}

SWEP.weaponTemp  = 0
SWEP.NextUpdate = CurTime()


function SWEP:CustomDeploy()
	self:playSequence("arm")

	self:SetNextPrimaryFire(CurTime() + 2)
	self:SetNextSecondaryFire(CurTime() + 2)

	return true
end

function SWEP:CustomHolster()
	return true
end

function SWEP:TracerOverRange(maxRange)
	local eyePos = self.Owner:EyePos()
	local tr = self.Owner:GetEyeTrace()
	local hitPos = tr.HitPos
	
	local distance = (hitPos - eyePos):Length()
	
	if distance > maxRange then
		return false
	end
	
	return true
end

function SWEP:TracerDistance(maxRange)
	local eyePos = self.Owner:EyePos()
	local tr = self.Owner:GetEyeTrace()
	local hitPos = tr.HitPos

	return (hitPos - eyePos):Length()
end

function SWEP:Projectile(projectile_effect)
	local trace = self.Owner:GetEyeTrace()
    local targetPos = trace.HitPos

    local spawnPos = GunmanShared.ClampedTracerHitPos(self, 128)

	local baseDirection = (targetPos - spawnPos):GetNormalized()
	
	local finalDirection = baseDirection:Angle()

	if SERVER then
		local ent = GunmanShared.EntityCreate(self, projectile_effect, spawnPos, finalDirection)

		ent:GetPhysicsObject():ApplyForceCenter(finalDirection:Forward() * 500)
	end
end

function SWEP:FireWeapon()
	local cone = math.Rand(0.3, -0.3)
	self.Owner:ViewPunch(Angle(cone, cone * 0.5, 0))

	if SERVER then
		local ammoTake = math.ceil((5 - self.CurrentPowerAndAccuracy) * 0.5) * GunmanShared.WeaponAmmoUsage.Beamgun.Standard
		self.Owner:RemoveAmmo(ammoTake, self.Primary.Ammo)
	end

	self.lightingLength = 128 * (self.CurrentRange ^ 2)
	
	local damage = GunmanShared.WeaponDamage.Beamgun.Modes

	self.lightingDamage = damage[self.CurrentPowerAndAccuracy]
	self.lightingDamageSize = 16
	self.lightingAccuracy = self:TracerDistance(self.lightingLength) * 0.05 / (self.CurrentPowerAndAccuracy ^ 2)

	if self.CurrentRange <= 2 then
		local eyePos = self.Owner:EyePos()
		local tr = self.Owner:GetEyeTrace()

		local dir = (tr.HitPos- eyePos):GetNormalized() 
		local distance = (tr.HitPos - eyePos):Length()

		local ent = nil

		if distance < self.lightingLength then
			ent = tr.Entity
		end

		if ent and ent ~= game.GetWorld() and ent:GetClass() ~= "gunman_weapon_beamgun_chain" and self.gotTazerEntity == false then
			if not SERVER then return end

			self.gotTazerEntity = true
			self.tazerTazing = true
			self:SetNW2Bool("tazerTazing", self.tazerTazing)

			self.Owner:EmitSound("gunman_beamgun_taze")

			util.BlastDamage(
				self, 
				self.Owner, 
				GunmanShared.ClampedTracerHitPos(self, self.lightingLength, self.lightingAccuracy), 
				32, 
				GunmanShared.WeaponDamage.Beamgun.TazerBurst / self.CurrentPowerAndAccuracy 
			)

			timer.Simple(1, function() 
				self.tazerTazing = false
				self:SetNW2Bool("tazerTazing", self.tazerTazing)

				self.tazerTazingReset = true
			end)

			timer.Simple(2, function() 
				self.gotTazerEntity = false
				self.tazerTazingReset = false
			end)
		end
	end

	if self.CurrentLightning ~= 3 then
		local effectdata = EffectData()
		effectdata:SetOrigin(GunmanShared.ClampedTracerHitPos(self, self.lightingLength, self.lightingAccuracy))
		effectdata:SetMagnitude(1)

		if self:TracerOverRange(self.lightingLength) then
			util.Effect("cball_bounce", effectdata)
		end	
	end

	if self.CurrentLightning ~= 3 and not self.tazerTazing then
		if SERVER then
			util.BlastDamage(
				self, 
				self.Owner, 
				GunmanShared.ClampedTracerHitPos(self, self.lightingLength, self.lightingAccuracy), 
				self.lightingDamageSize, 
				self.lightingDamage
			)
		end
	end

	if not SERVER then return end
	
	if self.CurrentLightning == 2 then 
		if CurTime() > self.NextChainSpawnTime then
			GunmanShared.EntityCreate(self, "gunman_weapon_beamgun_chain", GunmanShared.ClampedTracerHitPos(self, self.lightingLength), nil)

			self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Beamgun.Standard, self.Primary.Ammo)

			local adjustSpawnRate = (self.CurrentPowerAndAccuracy ^ 0.5) * 0.25
			self.NextChainSpawnTime = CurTime() + adjustSpawnRate
		end	
	elseif self.CurrentLightning == 3 and self.BallWasLaunched == false then 
		if CurTime() > self.NextBallLaunchTick then
			self.DelayBallLaunch = self.DelayBallLaunch + 0.5
			self.NextBallLaunchTick = CurTime() + 0.25
		end

		if self.DelayBallLaunch > 0.5 then
			self.DelayBallLaunch = 0
			self.BallWasLaunched = true

			if self:Ammo1() < 15 then 
				self.Owner:EmitSound("gunman_beamgun_off")
				return 
			end
			
			if self.weaponTemp < 100 then 
				self.weaponTemp = 130
			elseif self.weaponTemp < 140 then
				self.weaponTemp = self.weaponTemp + 20
			end

			if self.weaponTemp > 140 then
				self.weaponTemp = 150
			end

			self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Beamgun.PowerBall, self.Primary.Ammo)

			self:SetNW2Bool("BallWasLaunched", self.BallWasLaunched)	

			self:Projectile("gunman_weapon_beamgun_ball")
			self.Owner:EmitSound("gunman_beamgun_off")
		end
	end
end

function SWEP:PrimaryAttack()
	if game.SinglePlayer() then self:CallOnClient("PrimaryAttack") end
	if self.BallWasLaunched == true or self.Malfunction == true then return end

	if not self:IsMenuOpen() then 
		if self.HasFired == true then
			if self:Ammo1() <= 0 or self.Owner:WaterLevel() == 3 then 
				self:DryFire() 
				return 
			end
	
			self:playSequence("fire")

			self:SetNextPrimaryFire(CurTime() + self:sequenceLength("fire") - 0.1)	

			self:FireWeapon()
		end
	else
		self:HudEmitSound("gunman_HudOff")
		self:SetNextPrimaryFire(CurTime() + 1)	
		self:SetNW2Bool("FireModeMenuToggle", false)
		
		-- Start the safety tap counter when closing the menu
		self.MenuSafetyTaps = 1 
		self.LastAttackDown = true -- Consume this click so it doesn't count as the 2nd tap in Think
	end
end

function SWEP:SecondaryAttack()
	if game.SinglePlayer() then self:CallOnClient("SecondaryAttack") end
	
	if self.isFiring == true then return end
	if self.tazerTazing then return end

	if not self:IsMenuOpen() then
		self:HudEmitSound("gunman_HudOn")
	end
	
	self:SetNW2Bool("FireModeMenuToggle", true)
	self.MenuSafetyTaps = 0 -- Reset safety taps when menu is opened

	self:SetNextSecondaryFire(CurTime() + 0.5)	
end

function SWEP:CustomThink()
	self:SetClip1(self:Ammo1())

	if self.tazerTazing and CurTime() > self.NextTazerFireTime then
		self:PrimaryAttack()
		self.NextTazerFireTime = CurTime() + self:sequenceLength("fire") - 0.1
	end


	if self.tazerTazing then
		if SERVER then
			self.Owner:ScreenFade(SCREENFADE.IN, Color(255, 255, 255, 50), 0.5, 0)
		end
	end

	if not SERVER then return end

	if not self:IsMenuOpen() then

		local attackDown = self.Owner:KeyDown(IN_ATTACK)
		if self.MenuSafetyTaps and self.MenuSafetyTaps < 2 then
			if attackDown and not self.LastAttackDown then
				self.MenuSafetyTaps = self.MenuSafetyTaps + 1
			end
		end
		self.LastAttackDown = attackDown

		if (self.Owner:KeyDown(IN_ATTACK) or self.tazerTazing) 
		and (not self.MenuSafetyTaps or self.MenuSafetyTaps >= 2) 
		and (self:Ammo1() > 0 and self.Owner:WaterLevel() ~= 3)
		and not self.Malfunction and not self.tazerTazingReset then

			if self.IsCharging == false then
				if self.weaponTemp > 1 then
					self.weaponTempStartTime = CurTime() + (self.weaponTempStartTime - CurTime()) 
				else
					self.weaponTempStartTime = CurTime() 
				end

				self.ChargeStartTime = CurTime()
				self.isFiring = true
				self.IsCharging = true
				self.HasFired = false

				if self.CurrentPowerAndAccuracy >= 2 and IsFirstTimePredicted() then
					self.Owner:EmitSound("gunman_beamgun_windup")
				else
					self.Owner:EmitSound("gunman_beamgun_windup2")
				end

				self:playSequence("charge")
			end

			local chargeDuration = CurTime() - self.ChargeStartTime

			if chargeDuration >= self.ChargeTime 
				and self.HasFired == false then

				self.HasFired = true
			end

		elseif self.IsCharging == true then
			if (self.CurrentLightning ~= 3 or self.BallWasLaunched == false) and self.Malfunction == false then

				self.weaponTempCurrent = CurTime() + (self.weaponTemp / 2)

				if not self.tazerTazingReset then 
					self.Owner:EmitSound("gunman_beamgun_off")
				end
			end

			self.DelayBallLaunch = 0
			self.isFiring = false
			self.IsCharging = false
			self.HasFired = false
		end
	end

	if self.Owner:KeyReleased(IN_ATTACK) and self.CurrentLightning == 3 and self.BallWasLaunched == true then
		self.BallWasLaunched = false
		self:SetNW2Bool("BallWasLaunched", self.BallWasLaunched)	
	end

	if self.BallWasLaunched == true then
		self.HasFired = false
	end

	if self.HasFired == true and self.BallWasLaunched == false and self.weaponTemp < 140 then
		self.weaponTemp = self.weaponTemp + 0.2
		self:SetNW2Float("weaponTemp", math.Clamp(self.weaponTemp, 0, 140))

	elseif self.HasFired == false and self.BallWasLaunched == false and self.weaponTemp > 1 then
		self.weaponTemp = self.weaponTemp - 0.1
		self:SetNW2Float("weaponTemp", self.weaponTemp)
	end

	if self.Malfunction == false and self.weaponTemp >= 140 then
		self.Malfunction = true 
		self:SetNW2Bool("WeaponMalfunctioned", self.Malfunction)
		self.tazerTazing = false

		self.Owner:EmitSound("gunman_beamgun_electro")
	end
	
	if self.Malfunction == true and self.weaponTemp < 130 then
		self.delayMalfunctionReset = self.delayMalfunctionReset + 1

		if self.delayMalfunctionReset > 5 then
			self.delayMalfunctionReset = 0
			self.Malfunction = false
			self:SetNW2Bool("WeaponMalfunctioned", self.Malfunction)
		end

	elseif self.delayMalfunctionReset ~= 0 then 
		self.delayMalfunctionReset = 0
	end

	self:SetNW2Int("HasFired", self.HasFired)
	self:SetNW2Int("CurrentPowerAndAccuracy", self.CurrentPowerAndAccuracy)
end

AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "M.U.L.E"	
SWEP.ClassName = "gunman_weapon_mule"

SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModelFOV = 100
SWEP.ViewModel	= "models/gunman/v_dml.mdl"
SWEP.WorldModel = "models/gunman/w_dml.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 4
SWEP.SlotPos = 1

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.IdleSequences = { "idle", "idlefidget" }

SWEP.Primary.ClipSize = 2
SWEP.Primary.DefaultClip = 2
SWEP.Primary.MaxAmmo = 8
SWEP.Primary.Automatic = false
SWEP.Primary.Ammo = "gunman_ammo_dmlclip"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Mule
SWEP.Primary.Spread = 0
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.NumberofShots = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "gunman_ammo_dmlClip"
SWEP.Secondary.Automatic = false

SWEP.Secondary.ClipSize = -1
SWEP.Secondary.DefaultClip = 2
SWEP.Secondary.MaxAmmo = 8
SWEP.Secondary.Automatic = false
SWEP.Secondary.Ammo = "none"
SWEP.Secondary.Damage = GunmanShared.WeaponDamage.Mule
SWEP.Secondary.Spread = 0
SWEP.Secondary.TakeAmmo = 1
SWEP.Secondary.NumberofShots = 1
SWEP.Secondary.Force = 7

SWEP.isFiring = false
SWEP.weaponFire = false

SWEP.CustomizeAnimation = "customize"
SWEP.CustomizeSound = "gunman_dml_customize"

SWEP.CurrentViewModel = nil

SWEP.MenuOptions = {
	[1] = {var = "CurrentLaunchType",      min = 1, max = 2},
	[2] = {var = "CurrentFlightPathType",  min = 1, max = 3},
	[3] = {var = "CurrentDetonationType",  min = 1, max = 3},
	[4] = {var = "CurrentPayloadType",     min = 1, max = 2},
}

SWEP.CurrentLaunchType = GunmanShared.DefaultWeaponSettings.Mule.LaunchType
SWEP.CurrentFlightPathType = GunmanShared.DefaultWeaponSettings.Mule.FlightPathType
SWEP.CurrentDetonationType = GunmanShared.DefaultWeaponSettings.Mule.DetonationType
SWEP.CurrentPayloadType = GunmanShared.DefaultWeaponSettings.Mule.PayloadType

function SWEP:CustomInitialize()
	self:SetWeaponHoldType(self.HoldType)
	self.CurrentAmountLoaded = 2
	
	self.CurrentLaunchType = GunmanShared.DefaultWeaponSettings.Mule.LaunchType
	self.CurrentFlightPathType = GunmanShared.DefaultWeaponSettings.Mule.FlightPathType
	self.CurrentPayloadType = GunmanShared.DefaultWeaponSettings.Mule.PayloadType
	self.CurrentDetonationType = GunmanShared.DefaultWeaponSettings.Mule.DetonationType
	self.FireModeMenuOption = 1

	self.lastTargetSwitchTime = 0
	self.lerpProgress = 0
	self.currentHomingTarget = nil

	self.lastTargetSwitchTime2 = 0
	self.lerpProgress2 = 0
	self.currentRangeTarget = nil
end

function SWEP:CustomDeploy()
	self.CurrentViewModel = self.Owner:GetViewModel()

	timer.Simple(0.1, function() 
		if IsValid(self) and IsValid(self.Owner) and self:Ammo1() <= 0 then
			self.Owner:GetViewModel():SetBodyGroups("010") 
		end
	end)
	
	return true
end

function SWEP:Projectile(projectile_effect, spiralInvert)
    if not SERVER then return end

    local tr = self.Owner:GetEyeTrace()
    local targetPos = tr.HitPos

	local spawnPos = self:GetViewModelBonePos(37) 

	if self.reloadFlipFlop then
    	spawnPos = self:GetViewModelBonePos(38) 
	end

	if spiralInvert == true then
		spawnPos = self:GetViewModelBonePos(38) 
	end

	spawnPos = spawnPos + self.Owner:GetViewModel():GetUp() * 68
	
    local baseDirection = (targetPos - spawnPos):GetNormalized()
    local finalDirection = baseDirection:Angle()

    local ent = GunmanShared.EntityCreate(self, projectile_effect, spawnPos, finalDirection)

    if self.CurrentPayloadType == 2 then
        ent.doCluster = true
    end

    if self.CurrentDetonationType == 1 then
        ent.doDetonateOnImpact = true
    end

	if self.CurrentDetonationType == 3 then
        ent.doDeployTripMine = true
    end

	if spiralInvert == true then
		ent.doSpiralReverse = true
	end

	if self.homingTarget and self.homingTarget:IsValid() then
		ent.homingTarget = self.homingTarget
	end

	if self.CurrentFlightPathType == 1 then
		ent.doGuided = true
	end

	if self.CurrentFlightPathType == 3 then
		ent.doSpiral = true
	end
end

function SWEP:FireMissile()
	local cone = math.Rand(0.3, -0.3)
	self.Owner:ViewPunch(Angle(cone, cone * 0.5, 0))

	if self.CurrentFlightPathType == 3 then
		self:TakePrimaryAmmo(GunmanShared.WeaponAmmoUsage.Mule.LaunchSpiral)
	else
		self:TakePrimaryAmmo(GunmanShared.WeaponAmmoUsage.Mule.LaunchStandard)
	end

	if self.CurrentAmountLoaded > 0 and self.CurrentFlightPathType ~= 3 then
		self.CurrentAmountLoaded = self.CurrentAmountLoaded - 1
	end

	if self.CurrentFlightPathType == 3 then
		self.CurrentAmountLoaded = 0
	end

	if self.CurrentFlightPathType == 3 then
		self:Projectile("gunman_weapon_missile_armed", false)
		self:Projectile("gunman_weapon_missile_armed", true)
	else
		self:Projectile("gunman_weapon_missile_armed", false)
	end
end

function SWEP:GetPlayerAimTraceLine()
    local playerPos = self:GetOwner():GetShootPos()  
    local playerAim = self:GetOwner():GetAimVector()  

    local traceEndPos = playerPos + playerAim * 128

    return util.TraceLine({
        start = playerPos,  
        endpos = traceEndPos,  
        filter = self:GetOwner()  
    })
end

function SWEP:CheckValidDistance()
	if not SERVER then return end

	local trace = self:GetPlayerAimTraceLine()

    if trace.Hit then
		return true
	end

	return false
end

function SWEP:DeployTripMine()
	if not SERVER then return end

	local trace = self:GetPlayerAimTraceLine()

    if trace.Hit then
		local offset = (trace.HitPos - trace.StartPos):GetNormalized() * 8 
		local spawnPos = trace.HitPos - offset
		local angle = trace.HitNormal:Angle()
		
		local ent = GunmanShared.EntityCreate(self, "gunman_weapon_grenade_tripmine", spawnPos, angle)

		if self.CurrentPayloadType == 2 then
			ent.doCluster = true
		end
	
		self:TakePrimaryAmmo(self.Primary.TakeAmmo)
    end
end

function SWEP:DropShell(ent, right)
	if not SERVER then return end
	
	local spawnPos = self:GetPos() + self:GetRight() * right + self:GetUp() * 48
	local ent = GunmanShared.EntityCreate(self, ent, spawnPos, Angle(0, 0, 0), model)

	ent:GetPhysicsObject():ApplyForceCenter(self:GetRight() * right * 8)
end

function SWEP:DryFire()
	if (self:Clip1() == 0 and self:Ammo1() == 0) 
	or (self.CurrentFlightPathType == 3 and self:Clip1() < GunmanShared.WeaponAmmoUsage.Mule.LaunchSpiral)
	then
		if not SERVER then return end

		self.Owner:EmitSound("gunman_DryFire")
	end

	self:SetNextPrimaryFire(CurTime() + 0.1)
	self:SetNextSecondaryFire(CurTime() + 0.1)
end

function SWEP:PrimaryAttack()
	self:DryFire()
	if self.allowFire == false then return end

	if not self:IsMenuOpen() then 
		if self.weaponEmpty == true then return end

		self.weaponFire = true

		self.isFiring = true
	else
		self:HudEmitSound("gunman_HudOff")
		self:SetNextPrimaryFire(CurTime() + 0.5)	
	end

	self:SetNW2Bool("FireModeMenuToggle", false)
end

function SWEP:SecondaryAttack()
	if self.isFiring == true then return end

	if not self:IsMenuOpen() then
		self:HudEmitSound("gunman_HudOn")
	end

	self:SetNW2Bool("FireModeMenuToggle", true)

	self:SetNextSecondaryFire(CurTime() + 0.5)	
end

function SWEP:UpdateHomingTarget(
	owner, rays, conesize, dist, debugColor, 
	currentHomingTarget, lastTargetSwitchTime, targetSwitchInterval, lerpProgress, lerpDuration)

    local currentTime = CurTime()
	lastTargetSwitchTime = lastTargetSwitchTime or 0

    local newTarget = self:coneTrace(owner, rays, conesize, dist, debugColor)

    if not newTarget or not newTarget:IsValid() then
        if currentHomingTarget and (currentTime - lastTargetSwitchTime >= lerpDuration) then
            currentHomingTarget = nil  
            return nil, currentHomingTarget, lastTargetSwitchTime  
        end
    else
        if currentTime - lastTargetSwitchTime >= targetSwitchInterval then
            currentHomingTarget = newTarget
            lerpProgress = 0  
            lastTargetSwitchTime = currentTime  
        end
    end

    if currentHomingTarget and currentHomingTarget:IsValid() then
        lerpProgress = math.min(1, lerpProgress + FrameTime() / lerpDuration)

        local targetPosition = currentHomingTarget:GetPos()
        local currentPosition = owner:GetPos()  
        local newPosition = Lerp(lerpProgress, currentPosition, targetPosition)
    end

    return currentHomingTarget, currentHomingTarget, lastTargetSwitchTime, lerpProgress
end

function SWEP:CustomThink()
	if self.CurrentFlightPathType == 2 then 
		self.homingTarget, self.currentHomingTarget, self.lastTargetSwitchTime, self.lerpProgress = self:UpdateHomingTarget(
			self.Owner,  
			32,          
			128,         
			8192,        
			Color(255, 0, 0), 
			self.currentHomingTarget, 
			self.lastTargetSwitchTime, 
			0.5,           
			self.lerpProgress, 
			0.5            
		)

		self.homingRange, self.currentRangeTarget, self.lastTargetSwitchTime2, self.lerpProgress2 = self:UpdateHomingTarget(
			self.Owner,  
			32,          
			1024,        
			8192,        
			Color(0, 255, 0),  
			self.currentRangeTarget, 
			self.lastTargetSwitchTime2, 
			0.5,           
			self.lerpProgress2, 
			0.5            
		)
	end

	if not self.Owner:IsValid() then return end
    self.CurrentViewModel = self.Owner:GetViewModel()

	if self:Ammo1() > 0 and self:Clip1() <= 0 and self.emptyReload then
		self:PrimaryAttack()
		self.emptyReload = false
	end

	if self:Ammo1() <= 0 then
		self.emptyReload = true
	else
		self.CurrentViewModel:SetBodyGroups("000")
	end

	if SERVER then
		if self.CurrentLaunchType == 2 and not (self.homingTarget and self.homingTarget:IsValid()) then return end

		if self.weaponFire == false then return end 
			self.weaponFire = false

		self.reloadFlipFlop = not self.reloadFlipFlop		

		local reloadDelay = 2
		local fireDelay = 0.75

		if self.CurrentAmountLoaded < 2 then 
			reloadDelay = 1
		end

		timer.Create("reload" .. self:GetCreationID(), reloadDelay, 1, function() 
			self.allowFire = false
			self.isFiring = true

			if self:Ammo1() >= GunmanShared.WeaponAmmoUsage.Mule.ReloadDouble and self:Clip1() == 0 then 
				self.CurrentAmountLoaded = 2
				self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Mule.ReloadDouble, self.Primary.Ammo)
				self:playSequence("reloadboth")

				self.Owner:EmitSound("gunman_dml_dualreload")

			elseif self:Ammo1() >= GunmanShared.WeaponAmmoUsage.Mule.ReloadSingle then
				self.CurrentAmountLoaded = self:Clip1() + 1
				self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Mule.ReloadSingle, self.Primary.Ammo)

				if self.reloadFlipFlop then
					self:playSequence("reloadright")
				else
					self:playSequence("reloadleft")
				end

				self.Owner:EmitSound("gunman_dml_reload")
			end

			self:SetClip1(self.CurrentAmountLoaded)

			timer.Simple(0.8, function() 
				if self:Ammo1() <= 0 then
					self:setBodyGroups("010")
				end
			end)

			timer.Create("reset" .. self:GetCreationID(), self:sequenceLength("reloadboth"), 1, function() 
				self.allowFire = true
				self.isFiring = false
			end)
		end)

		if self:Clip1() > 0 then 
			if self.CurrentFlightPathType == 3 and self:Clip1() < GunmanShared.WeaponAmmoUsage.Mule.LaunchSpiral then 
				self.isFiring = false
				fireDelay = 0.5
				return 
			end

			self:playSequence("fire")
			self.Owner:EmitSound("gunman_muleFire")
			self:FireMissile()
		end

		self:SetNextPrimaryFire(CurTime() + fireDelay)
	end
end

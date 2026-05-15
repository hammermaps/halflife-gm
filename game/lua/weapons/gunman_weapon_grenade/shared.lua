AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "Grenade Core"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModelFOV = 100
SWEP.ViewModel	= "models/gunman/v_grenadecore.mdl"
SWEP.WorldModel = "models/gunman/w_grenadecore.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 4
SWEP.SlotPos = 0

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "grenade"
SWEP.IdleSequences = { "idle", "idle2", "idlefidget" }

SWEP.Primary.ClipSize = 8
SWEP.Primary.DefaultClip = 1
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = false
SWEP.Primary.Ammo = "gunman_ammo_dmlClip"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Grenade
SWEP.Primary.Spread = 0
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.NumberofShots = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "none"
SWEP.Secondary.Automatic = false

SWEP.isFiring = false

SWEP.FireModeMenuToggle = false
SWEP.FireModeMenuOption = 1

SWEP.CurrentViewModel = nil
SWEP.HasFired = false 

SWEP.isThrowing = false

SWEP.cancelAttackAnimation = false

SWEP.MenuOptions = {
	[1] = {var = "CurrentDetonationType", min = 1, max = 3},
	[2] = {var = "CurrentPayloadType",    min = 1, max = 2},
}

SWEP.CurrentDetonationType = GunmanShared.DefaultWeaponSettings.Grenade.DetonationType
SWEP.CurrentPayloadType = GunmanShared.DefaultWeaponSettings.Grenade.PayloadType

function SWEP:Equip()
	if self.Primary.DefaultClip > 0 then
		self.Owner:GiveAmmo(self.Primary.DefaultClip, self.Primary.Ammo, true)
	end
	self.Owner:EmitSound("gunman_gunPickup")
end

function SWEP:CustomDeploy()
	return true
end

function SWEP:CustomHolster()
	timer.Remove("arm" .. self:GetCreationID())
	timer.Remove("throw" .. self:GetCreationID())
	timer.Remove("takeout" .. self:GetCreationID())
end

function SWEP:Projectile(projectile_effect)
	if not SERVER then return end

	local tr = self.Owner:GetEyeTrace()

    local aimVector = self.Owner:GetAimVector()
    local rightVector = self.Owner:GetRight()
    local upVector = self.Owner:GetUp()

	local trace = self.Owner:GetEyeTrace()
    local targetPos = trace.HitPos

    local spawnPos = GunmanShared.ClampedTracerHitPos(self, 16, 32)

	local baseDirection = (targetPos - spawnPos):GetNormalized()
	
	local finalDirection = baseDirection:Angle()

	local throwAngle = self:GetAngles() + Angle(0,0,-90)

	local ent = GunmanShared.EntityCreate(self, projectile_effect, spawnPos, throwAngle)

	if self.CurrentPayloadType == 2 then 
		ent.doCluster = true 
	end

	if self.CurrentDetonationType == 1 then 
		ent.doDetonateOnImpact = true
	end

	ent:GetPhysicsObject():ApplyForceCenter(finalDirection:Forward() * 800)
end

function SWEP:ThrowGrenade()
	local cone = math.Rand(0.3, -0.3)
	self.Owner:ViewPunch(Angle(cone, cone * 0.5, 0))

	if SERVER then
		self.Owner:RemoveAmmo(1, self.Primary.Ammo)
	end

	self:Projectile("gunman_weapon_grenade_armed")
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

	self.isFiring = false

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
	
		if SERVER then
			self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.Grenade.Standard, self.Primary.Ammo)
		end

		self.CurrentViewModel:SetNoDraw(true)

		timer.Simple(0.1, function() 
			if IsValid(self) and IsValid(self.CurrentViewModel) then
				self.CurrentViewModel:SetNoDraw(false)
				self:PrimaryAttack()
			end
		end)
    end
end

function SWEP:DropShell(modelPath, right)
	if not SERVER then return end
	
	local spawnPos = self:GetPos() + self:GetRight() * right + self:GetUp() * 48
	local ent = GunmanShared.EntityCreate(self, "gunman_physics_object", spawnPos, Angle(0, 0, 0), modelPath)

    if IsValid(ent) then
	    ent:GetPhysicsObject():ApplyForceCenter(self:GetRight() * right * 8)
    end
end

function SWEP:PrimaryAttack()
	if not self:IsMenuOpen() then 
		self.isFiring = true

		local function takeoutSequence()
			if self:Clip1() < 1 then 
				timer.Remove("dropCovers") 
				timer.Remove("takeout") 
				self:SetWeaponHoldType("normal")
				return 
			else
				self:SetWeaponHoldType("grenade")
			end

			self:playSequence("takeout")	
			self:setBodyGroups("01")

			timer.Create("dropCovers" .. self:GetCreationID(), 1, 1, function()
				if IsValid(self) then
					self:DropShell("models/gunman/shelltip.mdl", 10)
				end
			end)

			timer.Create("takeout" .. self:GetCreationID(), self:sequenceLength("takeout"), 1, function() 
				if IsValid(self) then
					self:DropShell("models/gunman/shellengine.mdl", -10)
					self:setBodyGroups("00")
					self.isFiring = false
					self.grenadeReady = true
				end
			end)	
		end

		if not self.grenadeReady then
			takeoutSequence()
		end

		if not self.grenadeReady then return end

		if self.CurrentDetonationType ~= 3 then
			self:playSequence("arming")

			timer.Create("arm" .. self:GetCreationID(), self:sequenceLength("arming"), 1, function() 
				if not IsValid(self) then return end
				self:playSequence("throw")
				self:setBodyGroups("02")
				self:ThrowGrenade()
				self.Owner:SetAnimation(PLAYER_ATTACK1)
				self.grenadeReady = false

				timer.Create("throw" .. self:GetCreationID(), self:sequenceLength("throw"), 1, function() 
					if IsValid(self) then
						self:PrimaryAttack()
					end
				end)
			end)
			
			local time = self:sequenceLength("arming") + self:sequenceLength("throw") + self:sequenceLength("takeout") 
			self:SetNextPrimaryFire(CurTime() + time)	
		else
			if self:CheckValidDistance() == false then return end

			self:playSequence("tripmine")

			timer.Create("tripmine" .. self:GetCreationID(), self:sequenceLength("tripmine") * 0.68, 1, function() 
				if not IsValid(self) then return end
				if self:CheckValidDistance() == false then return end
					
				self:DeployTripMine()
				self.Owner:SetAnimation(PLAYER_ATTACK1)
				self.grenadeReady = false
			end)	

			local time = self:sequenceLength("tripmine") + self:sequenceLength("takeout") 
			self:SetNextPrimaryFire(CurTime() + time)			
		end
	else
		self:SetNW2Bool("FireModeMenuToggle", false)

		self:HudEmitSound("gunman_HudOff")
		self:SetNextPrimaryFire(CurTime() + 0.5)	
	end
end

function SWEP:SecondaryAttack()
	if self.isFiring == true then return end

	if not self:IsMenuOpen() then
		self:HudEmitSound("gunman_HudOn")
	end

	self:SetNW2Bool("FireModeMenuToggle", true)

	self:SetNextSecondaryFire(CurTime() + 0.5)	
end

function SWEP:CustomThink()
	self:SetClip1(self:Ammo1())

	if not SERVER then return end 

	if self:Clip1() < 1 then 
		if IsValid(self.Owner) then
			self.Owner:StripWeapon(self:GetClass())
		end
		self.readyGreande = false
	end

	if self:Clip1() > 0 and not self.readyGreande and not self.grenadeReady then
		self.readyGreande = true

		self:PrimaryAttack()
		self:SetNextPrimaryFire(CurTime() + 2)	
	end
end
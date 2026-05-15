AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.PrintName	= "Gauss Pistol"	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModel	= "models/gunman/v_gausspistol.mdl"
SWEP.ViewModelFOV = 100
SWEP.WorldModel = "models/gunman/w_gausspistol.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 1
SWEP.SlotPos = 0

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "pistol"
SWEP.DeploySequence = "draw"
SWEP.IdleSequences = { "idle", "idlerestless" }
SWEP.IdleSequencesSniper = { "sniperidle", "sniperidlerestless", "sniperidlefidget" }

SWEP.Primary.ClipSize = 150
SWEP.Primary.DefaultClip = 35
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = false
SWEP.Primary.Ammo = "gunman_ammo_guassClip"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.GaussPistol.Pulse
SWEP.Primary.TakeAmmo = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "none"

SWEP.isFiringDelay = 1

SWEP.CustomizeAnimation = "customize"
SWEP.CustomizeSound = "gunman_gausspistol_customize"
SWEP.CustomizeSoundDelay = 0.8

SWEP.FireModes = { 
	Pulse  = 1, 
	Charge = 2, 
	Rapid  = 3, 
	Sniper = 4, 
} 

SWEP.ammoCosts = {
	[SWEP.FireModes.Pulse]   = GunmanShared.WeaponAmmoUsage.GaussPistol.Pulse,
	[SWEP.FireModes.Charge]  = GunmanShared.WeaponAmmoUsage.GaussPistol.Charge,
	[SWEP.FireModes.Rapid]   = GunmanShared.WeaponAmmoUsage.GaussPistol.Rapid,
	[SWEP.FireModes.Sniper]  = GunmanShared.WeaponAmmoUsage.GaussPistol.Sniper,
}

SWEP.CurrentFireMode = GunmanShared.DefaultWeaponSettings.GaussPistol.FireMode

SWEP.MenuOptions = {
	[1] = {var = "CurrentFireMode", max = 4}
}

SWEP.FireModeMenuToggle = false
SWEP.pulseBeamDrawTime = 0
SWEP.bullet = {}
SWEP.sniperMode = false
SWEP.sniperModeTransitionReady = true
SWEP.sniperModeZoom = 0
SWEP.sniperModeZoomFrame = 0
SWEP.startPosSniperBeam = Vector()
SWEP.ToggleKeyPress = false
SWEP.muzzleFlashLifeTime = 0
SWEP.beamEndPos = Vector()
SWEP.CurrentFireModePublic = 1
SWEP.sniperModeZoomFrameStartTime = CurTime()
SWEP.scaleBeamRingStartTime = CurTime()

function SWEP:GetIdleSequences()
	if self.sniperMode then
		return self.IdleSequencesSniper
	end
	return self.IdleSequences
end

function SWEP:CustomDeploy()
	if self.sniperMode == true then
		self.CurrentViewModel:SetBodyGroups("11")
		self:playSequence("sniperdraw")
	else
		self.CurrentViewModel:SetBodyGroups("10")
		self:playSequence("draw")
	end

	self:SetNW2Entity("CurrentViewModel", self.CurrentViewModel)

	return true
end

function SWEP:CustomHolster()
	hook.Remove("PlayerButtonDown", "Down" .. self:GetCreationID())
	timer.Remove("FOVZoomDelay" .. self:GetCreationID())
	timer.Remove("FOVZoomEndDelay" .. self:GetCreationID())

	self.SkipViewModelFOVUpdate = false

	return true
end

function SWEP:ShootMuzzleEffect()
	local effect = EffectData()
	effect:SetEntity(self.Owner)   
	effect:SetFlags(45)
	effect:SetScale(0.5)
	
	if self.sniperMode then
		effect:SetFlags(46)
	end

	if not IsFirstTimePredicted() then return end
	util.Effect("gunman_gausspistol_muzzleflash", effect)
end

function SWEP:DoImpactEffect(tr, nDamageType)
	if tr.HitSky then return end

	local effectdata = EffectData()
	effectdata:SetOrigin(tr.HitPos + tr.HitNormal)
	effectdata:SetNormal(tr.HitNormal)
	effectdata:SetMagnitude(1)

	if not IsFirstTimePredicted() then return end
	util.Effect( "ElectricSpark", effectdata )
end

function SWEP:BaseFireMode(isAutomatic, idleDelay, nextFire, sequence, sequenceRate, viewPunchAngle)
	if game.SinglePlayer() and not SERVER then return end

	self.Primary.Automatic = isAutomatic
	self.isFiringDelay = idleDelay

	self:SetNextPrimaryFire(CurTime() + nextFire)

	self:playSequence(sequence)
	self.CurrentViewModel:SetPlaybackRate(sequenceRate)

	self.Owner:ViewPunch(viewPunchAngle)
	self.Owner:SetAnimation(PLAYER_ATTACK1)

	self:ShootMuzzleEffect()
end

function SWEP:BaseBulletData(num, spread, damage)
	self.bullet.Num = num
	self.bullet.Src = self.Owner:GetShootPos()
	self.bullet.Dir = self.Owner:GetAimVector()
	self.bullet.Spread = Vector(spread, spread)
	self.bullet.Tracer = 0
	self.bullet.Force = self.Primary.Force
	self.bullet.Damage = damage
	self.bullet.AmmoType = self.Primary.Ammo

	return self.bullet
end

function SWEP:FireModePulse()
	if game.SinglePlayer() and not SERVER then return end
	
	self.bullet = self:BaseBulletData(1, 0.01, 25)

	self.bullet.Callback = function(attacker, tr, dmginfo)
		local ed = EffectData()
		ed:SetOrigin(tr.HitPos)      
		ed:SetNormal(tr.HitNormal)      

		if IsFirstTimePredicted() then 
			util.Effect("gausspistol_pulse_impact", ed)
		end

		self.beamEndPos = tr.HitPos
		self:SetNW2Vector("beamEndPos", self.beamEndPos)

		self.pulseBeamDrawTime = CurTime() + 0.05
		self:SetNW2Float("pulseBeamDrawTime", self.pulseBeamDrawTime)
	end

	if SERVER then
		self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.GaussPistol.Pulse, self.Primary.Ammo)
	end

	self.Owner:FireBullets(self.bullet)

	if IsFirstTimePredicted() then 
		self.Owner:EmitSound("gunman_gaussFirePulse")
	end

	self:BaseFireMode(false, 2, 0.2, "singleshot", 1.5, Angle(math.Rand(0.5, -0.5), math.Rand(0.5, -0.5), 0))
end

function SWEP:FireModeCharge()
	if SERVER then
        self:Projectile("gunman_weapon_gausspistol_projectile", Angle(0,0,0), 1500, 45, 65, function(ent)
            ent.ProjectileType = "Charge"
        end)
 
		self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.GaussPistol.Charge, self.Primary.Ammo)
	end

	if IsFirstTimePredicted() then 
		self.Owner:EmitSound("gunman_gaussFireCharge")
	end

	self:BaseFireMode(false, 5, 2.1, "chargefire", 1.0, Angle(-4, -1, 0))
end

function SWEP:FireModeRapid()
	local effectdata = EffectData()
	effectdata:SetOrigin(self:GetPos())
	effectdata:SetMagnitude(0.1)

	if IsFirstTimePredicted() then
		util.Effect("ElectricSpark", effectdata) -- fixes muzzleflash not showing correctly
	end

	if SERVER then
        self:Projectile("gunman_weapon_gausspistol_projectile", Angle(math.Rand(-2, 2), math.Rand(-2, 2), 0), 2500, 45, 65, function(ent)
            ent.ProjectileType = "Rapid"
        end)

		self.Owner:RemoveAmmo(1, self.Primary.Ammo)
	end

	if IsFirstTimePredicted() then 
		self.Owner:EmitSound("gunman_gaussFireRapid")
	end

	self:BaseFireMode(true, 1, 0.1, "rapidfire", 1.0, Angle(math.Rand(0.2,-0.2), math.Rand(0.2,-0.2), 0))
end

function SWEP:FireModeSniper()
	self.pulseBeamDrawTime = CurTime() + 1.5

	local damage = GunmanShared.WeaponDamage.GaussPistol.SniperQuick
	if self.sniperModeZoom > 1 then 
		damage = GunmanShared.WeaponDamage.GaussPistol.Sniper
	end

	self.bullet = self:BaseBulletData(1, 0, damage)

	self.bullet.Callback = function(attacker, tr, dmginfo)
		local ed = EffectData()
		ed:SetOrigin(tr.HitPos)      
		ed:SetNormal(tr.HitNormal)    

		if self.sniperModeZoom > 1 then
			ed:SetMagnitude(2)  
		else
			ed:SetMagnitude(1)
		end  

		if IsFirstTimePredicted() then 
			util.Effect("gausspistol_sniper_impact", ed)
		end

		self.beamEndPos = tr.HitPos
		self:SetNW2Vector("beamEndPos", self.beamEndPos)

		self.pulseBeamDrawTime = CurTime() + 1.5
		self:SetNW2Float("pulseBeamDrawTime", self.pulseBeamDrawTime)

		GunmanShared.BlastDamage(self.Owner, self, tr.HitPos, 64, damage, DMG_ENERGYBEAM)
	end

	self.Owner:FireBullets(self.bullet)

	if self.sniperModeZoom > 0 then
		self:ShootMuzzleEffect()

		self.startPosSniperWorld = self.Owner:GetShootPos() 
		+ self.Owner:GetForward() * 36 
		- self.Owner:GetUp() * 13
		+ self.Owner:GetRight() * 8

		self:SetNW2Vector("startPosSniperWorld", self.startPosSniperWorld)
	end

	if self.sniperModeZoom > 1 then 
		self.doBeamRings = true
		self:SetNW2Bool("doBeamRings", self.doBeamRings)

		if SERVER then
			self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.GaussPistol.SniperZoomed, self.Primary.Ammo)
		end
	else
		if SERVER then
			self.Owner:RemoveAmmo(GunmanShared.WeaponAmmoUsage.GaussPistol.Sniper, self.Primary.Ammo)
		end
	end

	timer.Simple(1.5, function()
		self.doBeamRings = false
		self:SetNW2Bool("doBeamRings", self.doBeamRings)
	end)

	self:BaseFireMode(false, 2, 1, "snipershoot", 1.5, Angle(-1, -1, 0))
end

function SWEP:SniperModeSequence(bodygroupId)
	self.sniperModeTransitionReady = false
	self.isFiring = true
	local vm = self:GetNW2Entity("CurrentViewModel", nil)

	local oldAnim = self.sniperMode and "holster" or "sniperholster"
	local newAnim = self.sniperMode and "sniperdraw" or "draw"

	vm:SetSequence(oldAnim)
	vm:SetPlaybackRate(0.75)
	
	timer.Simple(1.5, function() 
		vm:SetBodyGroups(bodygroupId)
		
		vm:SetSequence(newAnim)
		vm:SetPlaybackRate(0.75)

		timer.Simple(2, function() 
			self.isFiring = false 
			self.sniperModeTransitionReady = true
		end)
	end)
end

function SWEP:PrimaryAttack()
	if game.SinglePlayer() then self:CallOnClient("PrimaryAttack") end

	if self.sniperModeTransitionReady == false then return end

	if not self:IsMenuOpen() then
		self.isFiring = true

		if self:Ammo1() <= 0 or self.Owner:WaterLevel() == 3 then 
			self:DryFire() 
			return 
		end

		if self:Ammo1() < GunmanShared.WeaponAmmoUsage.GaussPistol.Sniper and self.sniperMode == true then 
			self:DryFire() 
			return 
		end

		timer.Simple(self.isFiringDelay, function() 
			self.isFiring = false
		end)

		local damage = GunmanShared.WeaponDamage.GaussPistol.Rapid
		if self.CurrentFireMode == self.FireModes.Sniper or self.CurrentFireMode == self.FireModes.Charge then
			damage = GunmanShared.WeaponDamage.GaussPistol.Charge
		end

		local fireFuncs = {
			[self.FireModes.Pulse]  = self.FireModePulse,
			[self.FireModes.Charge] = self.FireModeCharge,
			[self.FireModes.Rapid]  = self.FireModeRapid,
		}
		
		local func = fireFuncs[self.CurrentFireMode]
		if fireFuncs[self.CurrentFireMode] then
			func(self) 
		end
	else
		if self.CurrentFireMode == 4 and self.sniperMode == false then
			self.sniperMode = true
			self:SniperModeSequence("11")

		elseif self.sniperMode == true and self.CurrentFireMode != 4 then
			self.sniperMode = false
			self:SniperModeSequence("10")
			self.SkipViewModelFOVUpdate = false
		end

		self:HudEmitSound("gunman_HudOff")
		self:SetNextPrimaryFire(CurTime() + 0.2)
	end
	
	self:SetNW2Bool("FireModeMenuToggle", false)
end

function SWEP:SecondaryAttack()
	if game.SinglePlayer() then self:CallOnClient("SecondaryAttack") end
	if CurTime() < self.pulseBeamDrawTime then return end

	if not self:IsMenuOpen() then
		self:HudEmitSound("gunman_HudOn")
	end

	self:SetNW2Bool("FireModeMenuToggle", true)
end

function SWEP:CustomThink()
	self:SetClip1(self:Ammo1())

	if CurTime() < self.pulseBeamDrawTime + 0.1 then
		self.scaleBeamRing = (CurTime() - self.scaleBeamRingStartTime) * 6
	else
		self.scaleBeamRing = 0
		self.scaleBeamRingStartTime = CurTime() 
	end

	if SERVER then
		self:SetNW2Float("scaleBeamRing", self.scaleBeamRing)
		self:SetNW2Int("CurrentFireMode", self.CurrentFireMode) 
		self:SetNW2Int("sniperModeZoom", self.sniperModeZoom)
	end

	if self.CurrentFireMode == 4 then 
		if self.sniperModeZoom > 0 then
			if self.sniperModeZoom > 1 and self.sniperModeZoom ~= 3 then
				self.sniperModeZoomFrame = (CurTime() - self.sniperModeZoomFrameStartTime) * 18
				self:SetNW2Float("sniperModeZoomFrame", self.sniperModeZoomFrame) 
			end
		else
			self.sniperModeZoomFrame = 0
			self.sniperModeZoomFrameStartTime = CurTime()
			self:SetNW2Float("sniperModeZoomFrame", self.sniperModeZoomFrame)
		end
	end

	if self.sniperModeTransitionReady and self.sniperMode and not self:IsMenuOpen() then

		self.SkipViewModelFOVUpdate = true

		if CurTime() > self.pulseBeamDrawTime then

			if self:Ammo1() < GunmanShared.WeaponAmmoUsage.GaussPistol.Sniper then return end

			if self.Owner:KeyDown(IN_ATTACK) and self.ToggleKeyPress == false and not self:GetNW2Bool("SelectionMenuOpen") then
				self.ToggleKeyPress = true
				self.isFiring = true

				self:playSequence("sniperidle")

				self.Owner:SetFOV(80, 0.1)
				self.sniperModeZoom = 1

				if IsFirstTimePredicted() then 
					self.Owner:EmitSound("gunman_gaussSniperZoom")
				end

				timer.Create("FOVZoomDelay" .. self:GetCreationID(), 0.5, 1, function() 
					self.Owner:SetFOV(20, 0.5)
					self.sniperModeZoom = 2

					timer.Create("FOVZoomEndDelay" .. self:GetCreationID(), 0.5, 1, function() 
						self.sniperModeZoom = 3
					end)
				end)
			end

			if self.Owner:KeyReleased(IN_ATTACK) and self.ToggleKeyPress == true then
				self.ToggleKeyPress = false
				self.isFiring = false
				
				timer.Remove("FOVZoomDelay" .. self:GetCreationID())
				timer.Remove("FOVZoomEndDelay" .. self:GetCreationID())

				self.Owner:SetFOV(0, 0.1)
				
				self:FireModeSniper()

				if self.sniperModeZoom == 1 then
					self.Owner:EmitSound("gunman_gaussSniperRunZoomFire")
				else
					self.Owner:EmitSound("gunman_gaussSniperZoomFire")
				end
					
				self.sniperModeZoom = 0
			end 
		else
			self.Owner:SetFOV(0, 0)
		end
	else
		self.SkipViewModelFOVUpdate = false
	end
end

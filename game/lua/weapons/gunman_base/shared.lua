AddCSLuaFile("cl_init.lua")
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

SWEP.Base = "weapon_base"
SWEP.Category = "Gunman Weaponry"
SWEP.Spawnable = false
SWEP.AdminOnly = false

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "pistol" 
SWEP.DeploySequence = "draw"

SWEP.NextThinkTime = 0
SWEP.NextFOVAdjustThinkTime = 0

SWEP.StopIdleSequence = false
SWEP.SkipViewModelFOVUpdate = false
SWEP.isFiring = false

function SWEP:Initialize()
	self:SetWeaponHoldType(self.HoldType)
    self:InitializeLastSettings()
    self:CustomInitialize()
end

function SWEP:CustomInitialize()
end

function SWEP:Equip()
    if self.Primary.DefaultClip > 0 then
	    self.Owner:GiveAmmo(self.Primary.DefaultClip, self.Primary.Ammo, true)
    end
	self.Owner:EmitSound("gunman_gunPickup")
end

function SWEP:EquipAmmo(ply)
	ply:EmitSound("gunman_ammoPickup")
end

function SWEP:Deploy()
	self.CurrentViewModel = self.Owner:GetViewModel()
	
	self.CurrentViewModel:SetPlaybackRate(1.0)
	self.CurrentViewModel:SetSequence(self.DeploySequence)
	
	local duration = self.CurrentViewModel:SequenceDuration()
	self:SetNextPrimaryFire(CurTime() + duration)
	self:SetNextSecondaryFire(CurTime() + duration)

    self.NextIdleTime = CurTime() + duration + 2
    
    if self.MenuOptions then
        hook.Add("PlayerButtonDown", "Down" .. self:GetCreationID(), function(ply, button)
            if not IsValid(self) or not IsValid(self.Owner) then return end
            if ply ~= self.Owner then return end

            if self:IsMenuOpen() then
                self:MenuSelectionInput(self.MenuOptions, button)
            end
        end)
    end

    self:CustomDeploy()

	return true
end

function SWEP:CustomDeploy()
end

function SWEP:IsMenuOpen()
    return self:GetNW2Bool("FireModeMenuToggle", false)
end

function SWEP:IsSelectionHUDActive()
    return false
end

function SWEP:Holster()
	timer.Remove("Thinker" .. self:GetCreationID())
    hook.Remove("PlayerButtonDown", "Down" .. self:GetCreationID())
    hook.Remove("HUDPaint", "WeaponHud" .. self:GetCreationID())

    self:CustomHolster()

	if IsValid(self.Owner) and IsValid(self.Owner:GetViewModel()) then
		self.Owner:GetViewModel():SetColor(Color(255, 255, 255))
        self.Owner:GetViewModel():SetSubMaterial(3, "") 
	end
	
	return true
end

function SWEP:CustomHolster()
end

function SWEP:OnRemove()
	self:Holster()
end

function SWEP:OnDrop()
	self:Holster()
end

function SWEP:HudEmitSound(snd)
	if game.SinglePlayer() then
		self.Owner:EmitSound(snd)
	elseif CLIENT then
		self.Owner:EmitSound(snd)
	end
end

function SWEP:DryFire()
    if SERVER then
	    self.Owner:EmitSound("gunman_DryFire")
    end
	self:SetNextPrimaryFire(CurTime() + 0.2)
	self:SetNextSecondaryFire(CurTime() + 0.2)
end

function SWEP:AccumulateValueByKeyPress(var, button, key1, key2, min, max) 
	if button == key1 or button == key2 then
		var = var or min
		local delta = (button == key1) and 1 or -1
		return math.Clamp(var + delta, min, max)	
	end
	return var
end

function SWEP:MenuSelectionInput(options, button)
    if not self:IsMenuOpen() then return end

	self.FireModeMenuOption = self:AccumulateValueByKeyPress(
		self.FireModeMenuOption or 1, button, KEY_DOWN, KEY_UP, 1, #options)
	self:SetNW2Int("FireModeMenuOption", self.FireModeMenuOption)

	local option = options[self.FireModeMenuOption]
	if option then
		self[option.var] = self:AccumulateValueByKeyPress(
			self[option.var], button, KEY_RIGHT, KEY_LEFT, option.min or 1, option.max)
		self:SetNW2Int(option.var, self[option.var])
	end
end

function SWEP:InitializeLastSettings()
    if not self.MenuOptions then return end
    self.LastMenuSettings = {}
    for _, option in pairs(self.MenuOptions) do
        self.LastMenuSettings[option.var] = self[option.var]
    end
end

function SWEP:SyncMenuVariables()
	self.FireModeMenuToggle = self:GetNW2Bool("FireModeMenuToggle", false)
	self.FireModeMenuOption = self:GetNW2Int("FireModeMenuOption", 1)

	if self.MenuOptions then
		for _, opt in ipairs(self.MenuOptions) do
			if opt.var then
				self[opt.var] = self:GetNW2Int(opt.var, self[opt.var] or (opt.min or 1))
			end
		end
	end
end

function SWEP:HandleMenuChanges()
    if not self.MenuOptions or not self.LastMenuSettings then return end

    local changed = false
    for _, option in pairs(self.MenuOptions) do
        local var = option.var
        if self[var] ~= self.LastMenuSettings[var] then
            changed = true
            break
        end
    end

    if changed then
        for _, option in pairs(self.MenuOptions) do
            self.LastMenuSettings[option.var] = self[option.var]
        end

        if self.isFiring then return end 

        if self.CustomizeAnimation then
            self:playSequence(self.CustomizeAnimation)
            self.isFiring = true
            
            local duration = self:sequenceLength(self.CustomizeAnimation)
            timer.Create("CustomizeLockout" .. self:GetCreationID(), duration, 1, function()
                if IsValid(self) then
                    self.isFiring = false
                end
            end)

            if self.CustomizeSound then
                if self.CustomizeSoundDelay and self.CustomizeSoundDelay > 0 then
                    timer.Simple(self.CustomizeSoundDelay, function()
                        if IsValid(self) and IsValid(self.Owner) then
                            self.Owner:EmitSound(self.CustomizeSound)
                        end
                    end)
                else
                    self.Owner:EmitSound(self.CustomizeSound)
                end
            end
        end
    end
end

function SWEP:Reload()
    return false
end

function SWEP:GetViewModelBonePos(bone)
	local spawnPos = self.Owner:GetViewModel():GetBonePosition(bone)
	if spawnPos == self.Owner:GetViewModel():GetPos() then
		spawnPos = self.Owner:GetViewModel():GetBoneMatrix(0):GetTranslation()
	end

	return spawnPos
end

function SWEP:coneTrace(player, numRays, radius, maxDistance, debugcolor)
    local centerPos = player:GetShootPos()
    local forward = player:GetForward()
    local up = player:GetUp()
    local right = forward:Cross(up):GetNormalized()

    for i = 0, numRays - 1 do
        local angle = math.Rand(0, 360)  
        local radiusOffset = math.Rand(0, radius) 
        local radian = math.rad(angle)
        local direction = forward + right * math.cos(radian) * radiusOffset + up * math.sin(radian) * radiusOffset
        local endPos = centerPos + direction + forward * maxDistance

        local trace = util.TraceLine({
            start = centerPos,
            endpos = endPos,
			filter = function(ent)
				if ent == player or ent == game.GetWorld() then
					return false
				end
				return true
			end
        })

        if trace.Entity:IsValid() then
            return trace.Entity
        end
    end
    return nil
end

function SWEP:Projectile(entityName, angleOffset, velocity, bone, heightAdjust, setupCallback)
    if SERVER then
        local ang = self.Owner:EyeAngles() + angleOffset
        local pos = self:GetViewModelBonePos(bone) 

        pos = pos + self.Owner:GetViewModel():GetUp() * heightAdjust

        local ent = GunmanShared.EntityCreate(self, entityName, pos, ang, nil, setupCallback)

        if not IsValid(ent) then return end

        local phys = ent:GetPhysicsObject()
        if IsValid(phys) then
            phys:SetVelocity(ent:GetForward() * velocity)
        end

        return ent
    end
end

function SWEP:IsVMSequenceFinished()
    local vm = self.Owner:GetViewModel()
    if not IsValid(vm) then return true end
    return vm:GetCycle() >= 1
end

function SWEP:playSequence(anim)
    local sequence = self.CurrentViewModel:LookupSequence(anim)
    if sequence == -1 then return end
	return self.CurrentViewModel:SendViewModelMatchingSequence(sequence)
end

function SWEP:sequenceLength(anim)
    if type(anim) == "string" then
        return self.CurrentViewModel:SequenceDuration(self.CurrentViewModel:LookupSequence(anim))
    end
	return self.CurrentViewModel:SequenceDuration(anim)
end

function SWEP:setBodyGroups(subModelIds)
	return self.CurrentViewModel:SetBodyGroups(subModelIds)
end

function SWEP:GetIdleSequences()
    return self.IdleSequences
end

function SWEP:PlayIdleSound(seqName)
end

function SWEP:PerformIdleSequence()
    if self.StopIdleSequence then return end
    
    local sequences = self:GetIdleSequences()
    
    if not sequences or #sequences == 0 then return end

    local choose = math.random(1, #sequences)
    local seqName = sequences[choose]

    self.CurrentViewModel:SetPlaybackRate(1.0)
    
    local seqId = self.CurrentViewModel:LookupSequence(seqName)
    if seqId ~= -1 then
        self.CurrentViewModel:SendViewModelMatchingSequence(seqId)
    end
    
    self:PlayIdleSound(seqName)
    
    return self.CurrentViewModel:SequenceDuration()
end

function SWEP:UpdateViewModelFOV()
	if CLIENT then
		if self.SkipViewModelFOVUpdate then return end

		if CurTime() > self.NextFOVAdjustThinkTime then
			self.ViewModelFOV = self.Owner:GetFOV()
			self.NextFOVAdjustThinkTime = CurTime() + 5
		end
	end
end

function SWEP:CustomThink()
end

function SWEP:Think()
    if CLIENT then
        self:SyncMenuVariables()
    end
    
    self:UpdateViewModelFOV()

    if IsValid(self.Owner) then
        self.CurrentViewModel = self.Owner:GetViewModel() 
    end
    
    if CLIENT then
        self:SetNW2Bool("SelectionMenuOpen", self:IsSelectionHUDActive())
    end

    if self.isFiring then
        self.NextIdleTime = CurTime() + 2
    end

    if SERVER and self.NextIdleTime and CurTime() > self.NextIdleTime and not self.isFiring then
        if self:IsVMSequenceFinished() then
            local duration = self:PerformIdleSequence() 
            if duration then
                self.NextIdleTime = CurTime() + math.max(duration, math.Rand(1, 3))
            else
                self.NextIdleTime = CurTime() + 0.1
            end
        else
            self.NextIdleTime = CurTime() + 0.1
        end
    end

    if not self:IsMenuOpen() then
        self:HandleMenuChanges()
    end

    self:CustomThink()
end

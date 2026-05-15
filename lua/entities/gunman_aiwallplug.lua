ENT.Type = "anim"
ENT.Base = "base_anim"
ENT.Category = "Gunman Items"
ENT.PrintName	= "AI Core Outlet"	
ENT.Spawnable = true
ENT.AdminOnly = false

ENT.physObject = nil

ENT.cycle = 0
ENT.activateCycle = false
ENT.DeactiveDelay = CurTime()
ENT.SkinDelay = CurTime()
ENT.allowExplosion = false 

AddCSLuaFile("gunman_aiwallplug.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self:SetModel("models/gunman/aiwallplug.mdl")
        
    self:SetSolid(SOLID_VPHYSICS)
    self:PhysicsInit(MOVETYPE_VPHYSICS)
    self:SetMoveType(MOVETYPE_VPHYSICS)
    self:SetSolidFlags(FSOLID_TRIGGER)
 
    if SERVER then
        self.physObject = self:GetPhysicsObject()

        if (IsValid(phys)) then
            self.physObject:Wake()
            self.physObject:EnableGravity(true)
            self.physObject:SetDragCoefficient(1)
        end

        if WireAddon then 
            self.Inputs = Wire_CreateOutputs(self, { "Activated" }) 
        end
    end

    self:SetBodyGroups(11)
end

function ENT:OnRestore()
    Wire_Restored(self)
end

function ENT:OnTakeDamage(dmg)
    local attacker = dmg:GetAttacker()
    if attacker:IsPlayer() and attacker:GetActiveWeapon():GetClass() == "gunman_weapon_aicore" then
        self:SetBodyGroups(10)
        self:SetSequence(1)  
        self.activateCycle = true
        self.DeactiveDelay = CurTime() + 2
        self.SkinDelay = CurTime() + 4
        self:EmitSound("gunman_aicore_activate")
    end

    if self.allowExplosion then
        self:Explode()
    end
end

function ENT:Use(activator, caller)
    if CurTime() > self.SkinDelay and self.activateCycle then
        self:StopSound("gunman_aicore_activated")
        self.activateCycle = false
        self.allowExplosion = false
        self.cycle = 0
        self:SetSkin(0)
        GunmanShared.EntityCreate(self, "gunman_weapon_aicore", self:GetPos() + self:GetForward() * 32 + self:GetUp() * 32)
        self:SetBodyGroups(11)
        Wire_TriggerOutput(self, "Activated", 0)
    end
end

function ENT:ListSequences()
    local sequenceCount = self:GetSequenceCount()
    for i = 0, sequenceCount - 1 do
        print("Sequence " .. i .. ": " .. self:GetSequenceName(i))
    end
end

function ENT:Draw()
    self:DrawModel()
end

function ENT:Explode()
    local ent = ents.Create("gunman_weapon_grenade_armed") 

	ent:SetOwner(self:GetOwner())  
	ent:SetPos(self:GetPos())  

	ent:Spawn()  
	ent:Activate() 	

    timer.Simple(0.05, function()
        ent:Detonate()
    end)

    timer.Simple(0.1, function()
        if not self:IsValid() then return end 
        self:StopSound("gunman_aicore_activated")
        self:Remove()
        SafeRemoveEntity(self)
    end)
end

function ENT:Think()
    if not self.activateCycle then return end
    if self.cycle < 1 then 
        self.cycle = self.cycle + 0.0075
    end 

    if CurTime() > self.DeactiveDelay and self.cycle >= 1 then
        self.DeactiveDelay = CurTime() + 100
        self:SetSequence(3)  
        self.cycle = 0
        self:EmitSound("gunman_aicore_deactivate")
    end

    if CurTime() > self.SkinDelay then
        self:SetSkin(1)
        self:SetSequence(0) 
    end

    self:SetCycle(self.cycle)

    if SERVER then
        if CurTime() > self.SkinDelay and self.activateCycle then
            if WireAddon then
                Wire_TriggerOutput(self, "Activated", 1)
            end
            self.allowExplosion = true
            self:EmitSound("gunman_aicore_activated")
        end
    end

    self:NextThink(CurTime())
    return true
end
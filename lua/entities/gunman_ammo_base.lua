ENT.Type = "anim"
ENT.Base = "base_anim"
ENT.PrintName = "Gunman Base Ammo"
ENT.Category = "Gunman Items"
ENT.Spawnable = false
ENT.AdminOnly = false

ENT.ammoAmount = 0
ENT.ammoType = ""
ENT.ammoModel = ""

AddCSLuaFile()

function ENT:Initialize()
    self:SetModel(self.ammoModel)
    self:PhysicsInit(SOLID_VPHYSICS)
    self:SetMoveType(MOVETYPE_VPHYSICS)
    self:SetSolid(SOLID_VPHYSICS)

    self:SetCollisionGroup(COLLISION_GROUP_WEAPON)

    if SERVER then
        self:SetTrigger(true)
    end

    local phys = self:GetPhysicsObject()

    if IsValid(phys) then 
        phys:Sleep() 
    end
end

function ENT:Touch(ent)
    if IsValid(ent) and ent:IsPlayer() then
        local given = ent:GiveAmmo(self.ammoAmount, self.ammoType, true)
        if given > 0 then
            self:EmitSound("gunman_ammoPickup")
            self:Remove()
        end
    end
end

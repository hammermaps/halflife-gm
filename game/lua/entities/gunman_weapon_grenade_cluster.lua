ENT.Type = "anim"
ENT.Base = "gunman_weapon_grenade_base"

ENT.detonated = false
ENT.numbers = { 0.5, 0.75, 1, 1.5, 2 }  

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self:SetModel("models/gunman/dmlcluster.mdl")

    self:SetSolid(SOLID_VPHYSICS)
    self:PhysicsInit(MOVETYPE_VPHYSICS)
    self:SetSolidFlags(FSOLID_TRIGGER)
    self:SetCollisionGroup(COLLISION_GROUP_PROJECTILE)   
    
    if SERVER then
        timer.Simple(1 + GunmanShared.CustomBellCurveRandom(self.numbers, 3, 1), function() 
            if self:IsValid() then
                self:Explode("gunman_explosion_small")
            end
        end)

        util.SpriteTrail(self, 0, Color(150, 150, 150, 120), true, 8, 0, 1, 4, "sprites/smoke.vmt")
    end
end



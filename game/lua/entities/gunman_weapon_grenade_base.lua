ENT.Type = "anim"
ENT.Base = "base_anim"

ENT.doCluster = false

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Cluster()
    self:EmitSound("gunman_dml_fragment")

    for i = 1, 6 do
        local angle = (i / 6) * math.pi * 2
        local x = math.cos(angle) * math.Rand(1, 2)
        local y = math.sin(angle) * math.Rand(1, 2)

        local ent = GunmanShared.EntityCreate(self, "gunman_weapon_grenade_cluster", self:GetPos())

        local phys = ent:GetPhysicsObject()

        if IsValid(phys) then
            local force = Vector(x, y, 1):GetNormalized() * 300 * math.Rand(0.9, 1.1)
            phys:ApplyForceCenter(force)
        end
    end

    self:Remove()
    SafeRemoveEntity(self)
end

function ENT:Explode(explosionTypeEnt)
    GunmanShared.EntityCreate(self, explosionTypeEnt, self:GetPos())

    self:Remove()
    SafeRemoveEntity(self)
end

function ENT:Detonate()
    if self.doCluster then
        self:Cluster()
    else
        self:Explode("gunman_explosion")
    end
end

function ENT:PhysicsCollide( data, physobj )
    if data.Speed > 100 then
        self:EmitSound("gunman_GrenadeHit")
    else
        physobj:Sleep()
    end
end

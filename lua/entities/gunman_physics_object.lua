ENT.Type = "anim"
ENT.Base = "base_anim"

AddCSLuaFile()

function ENT:Initialize()
    self:SetSolid(SOLID_VPHYSICS)
    self:PhysicsInit(MOVETYPE_VPHYSICS)
    self:SetMoveType(MOVETYPE_VPHYSICS)
    self:SetSolidFlags(FSOLID_TRIGGER)

    if IsValid(self:GetOwner()) then
        self:SetCollisionGroup(COLLISION_GROUP_PROJECTILE)
    end       
 
    if SERVER then
        timer.Simple(4, function() 
            if self:IsValid() then
                self:Remove()
                SafeRemoveEntity(self)
            end
        end)
    end
end

function ENT:Draw()
    self:DrawModel()
end

function ENT:PhysicsCollide( data, physobj )
    if data.Speed > 100 then
        self:EmitSound("gunman_sshell")
    else
        physobj:Sleep()
    end
end

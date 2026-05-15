ENT.Type = "anim"
ENT.Base = "gunman_weapon_grenade_base"

ENT.detonated = false
ENT.doDetonateOnImpact = false
ENT.doCluster = false

AddCSLuaFile() 

function ENT:Initialize()
    self:SetModel( "models/gunman/w_grenadecore_prop.mdl" )
        
    self:SetSolid(SOLID_VPHYSICS)
    self:PhysicsInit(MOVETYPE_VPHYSICS)
    self:SetMoveType(MOVETYPE_VPHYSICS)
    self:SetSolidFlags(FSOLID_TRIGGER)

    if IsValid(self:GetOwner()) then
        self:SetCollisionGroup(COLLISION_GROUP_PROJECTILE)
    end       
 
    if SERVER then
        timer.Simple(3, function() 
            if self:IsValid() then
                self:Detonate()
            end
        end)
    end
end

function ENT:PhysicsCollide( data, physobj )
    if self.doDetonateOnImpact == true and data.DeltaTime == 1 then
        timer.Simple(0.05, function()
            if self:IsValid() then
                self:Detonate()
            end
        end)

        return
    end

    if data.Speed > 100 then
        self:EmitSound("gunman_GrenadeHit")
    else
        physobj:Sleep()
    end
end

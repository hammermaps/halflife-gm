ENT.Type = "anim"
ENT.Base = "gunman_weapon_grenade_base"

ENT.detonated = false
ENT.doCluster = false
ENT.lineLength = 0
ENT.activated = false

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

    hook.Add("PreDrawEffects", "gunman_weapon_grenade_tripmine" .. self:GetCreationID(), function()
        local trace = util.TraceLine({
            start = self:GetPos(), 
            endpos = self:GetPos() + self:GetForward() * 1024, 
            filter = { self }
        })

        if self.activated == false then return end

        render.SetMaterial(Material("gunman/sprites/xbeam1"))
        render.DrawBeam(self:GetPos(), trace.HitPos, 1, 8, 8, Color(255, 0, 0))
    end)

    self:EmitSound("gunman_mineDeploy")

    timer.Simple(0.2, function() 
        self:EmitSound("gunman_mineCharge")
    end)

    timer.Simple(3, function() 
        self:EmitSound("gunman_mineActive")
        self.activated = true
    end)
end

function ENT:OnRemove()
    hook.Remove("PreDrawEffects", "gunman_weapon_grenade_tripmine" .. self:GetCreationID())
end

function ENT:OnTakeDamage(damage)
    if self.activated == true then
        timer.Simple(0.05, function()
            if self:IsValid() then
                self:Detonate()
            end
        end)
    end
    
    return false
end

function ENT:Think()
    self:NextThink(CurTime() + 0.01)

    if SERVER then 
        if self.activated == false then return end

        local trace = util.TraceLine({
            start = self:GetPos(),  
            endpos = self:GetPos() + self:GetForward() * 1024, 
            filter = { self },
            ignoreworld = true
        })

        if self.lineLength == 0 then
            self.lineLength = trace.Fraction 
        end

        if trace.Fraction ~= self.lineLength then
            self:Detonate()
        end
    end

    return true
end

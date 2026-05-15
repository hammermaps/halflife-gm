ENT.Type = "anim"
ENT.Base = "base_anim"

ENT.ProjectileType = "Rapid"

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self:SetModel("models/gunman/tubeball.mdl")
    
    self:SetSolid(SOLID_VPHYSICS)
    self:PhysicsInit(MOVETYPE_VPHYSICS)
    self:SetMoveType(MOVETYPE_VPHYSICS)
    self:SetSolidFlags(FSOLID_TRIGGER)

    if IsValid(self:GetOwner()) then
        self:SetCollisionGroup(COLLISION_GROUP_PROJECTILE)
    end           

    if SERVER then
        local phys = self:GetPhysicsObject()

        if (IsValid(phys)) then
            phys:Wake()
            phys:EnableGravity(false)
            phys:SetDragCoefficient(0.01)
            phys:SetMass(1)
            phys:AddGameFlag(FVPHYSICS_NO_NPC_IMPACT_DMG)
        end
        
        if self.ProjectileType == "Rapid" then
            self.TouchDmg = GunmanShared.EntityDamage.GaussPistol.Rapid.Touch
            self.BlastDmg = GunmanShared.EntityDamage.GaussPistol.Rapid.Blast
            self.BlastArea = GunmanShared.EntityDamage.GaussPistol.Rapid.Radius

            util.SpriteTrail(self, 0, Color(255, 255, 255, 255), false, 3, 0, 0.1, 0.05, "gunman/sprites/gaussbeam2")

        elseif self.ProjectileType == "Charge" then
            self.TouchDmg = GunmanShared.EntityDamage.GaussPistol.Charge.Touch
            self.BlastDmg = GunmanShared.EntityDamage.GaussPistol.Charge.Blast
            self.BlastArea = GunmanShared.EntityDamage.GaussPistol.Charge.Radius

            util.SpriteTrail(self, 0, Color(255, 255, 255, 255), false, 16, 0, 0.3, 0.05, "gunman/sprites/gaussbeam2")
        end
    end

    self.PlasmaLife = CurTime() + 5
end

function ENT:Draw()
    render.SetMaterial(Material("gunman/sprites/gausswomp"))

    if self.ProjectileType == "Rapid" then
        render.DrawSprite(self:GetPos(), 16, 16, Color(155, 255, 155))
    elseif self.ProjectileType == "Charge" then
        render.DrawSprite(self:GetPos(), 64, 64, Color(255, 255, 255))
    end
end

function ENT:ProjectileHit(hitNormal, start) 
    if SERVER then
        if hitNormal and start then
            local effectdata = EffectData()
            effectdata:SetOrigin(start)
            effectdata:SetNormal(hitNormal)

            if self.ProjectileType == "Rapid" then
                util.Effect("gausspistol_impact_fx", effectdata)
                self:EmitSound("gunman_gaussPistol_spriteHit")
                
            elseif self.ProjectileType == "Charge" then
                util.Effect("gausspistol_impact_charge_fx", effectdata)
                self:EmitSound("gunman_gaussPistol_spriteBigHit")
            end
        end

        SafeRemoveEntity(self)
    end
end

function ENT:PhysicsCollide( data, physobj )
    if self.hit then return end
    self.hit = true

    local target = data.HitEntity

    data.HitNormal = data.HitNormal * -1
    local start = data.HitPos + data.HitNormal
    local endpos = data.HitPos - data.HitNormal

    if self.ProjectileType == "Rapid" then
        util.Decal("gunman_gauss_rapid_hit", start, endpos)
    elseif self.ProjectileType == "Charge" then
        util.Decal("gunman_gauss_charge_hit", start, endpos)
    end
            
    self:ProjectileHit(data.HitNormal, start) 

    GunmanShared.BlastDamage(self.Owner, self, self:GetPos(), self.BlastArea, self.BlastDmg, DMG_ENERGYBEAM)
end

function ENT:Touch(ent)
    if ent == self:GetOwner() then return end 
    ent:TakeDamage(self.TouchDmg, self.Owner, self)
    
    if ent:GetSolid() == SOLID_VPHYSICS then return end

    local startPos = self:GetPos() - self:GetVelocity() * FrameTime()
    local endPos   = self:GetPos()

    local tr = util.TraceHull({
        start  = startPos,
        endpos = endPos,
        mins   = Vector(-2,-2,-2),   
        maxs   = Vector( 2, 2, 2),
        mask   = MASK_SHOT,         
        filter = function(ent)
            return ent ~= self and ent ~= self:GetOwner()
        end
    })

    if ent:GetClass() ~= "func_wall" then
        self:ProjectileHit(self:GetForward(), tr.HitPos) 
    end
end

function ENT:Think()
    if self:WaterLevel() > 0 then
        self:ProjectileHit(self:GetForward(), self:GetPos()) 
    end

    if self.PlasmaLife < CurTime() then
        self:ProjectileHit(self:GetForward(), self:GetPos()) 
    end
end

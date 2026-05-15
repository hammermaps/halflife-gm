ENT.Type = "anim"
ENT.Base = "base_anim"

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self:SetModel("models/gunman/tubeball.mdl")
        
    self:SetSolid(SOLID_NONE)
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
    end

	self.beamStartPos = self:GetPos() 
	self.beamEndPos = self:GetPos() + VectorRand(-math.random(32, 96), math.random(32, 96))

    self.LastPos = self:GetPos()

    timer.Simple(3, function()
        if self:IsValid() then 
            self:ProjectileHit() 
        end
    end)
end

function ENT:Draw()
    render.SetMaterial(Material("gunman/sprites/ballspark"))
    render.DrawSprite(self:GetPos(), 32, 32, Color( 155, 255, 155 ))
end

function ENT:PhysicsCollide( data, physobj )
    local target = data.HitEntity

    data.HitNormal = data.HitNormal * -1
    local start = data.HitPos + data.HitNormal
    local endpos = data.HitPos - data.HitNormal

    util.Decal("gunman_gauss_charge_hit", start, endpos)
    
    self:ProjectileHit(data.HitNormal) 
end

function ENT:ProjectileHit(pHitNormal) 
    if SERVER then
        local hitNormal = pHitNormal or Vector(0,0,1)

		local effectdata = EffectData()
		effectdata:SetOrigin(self:GetPos())
		effectdata:SetMagnitude(1)

		util.Effect("cball_bounce", effectdata)

        self:Remove()
        SafeRemoveEntity(self)
    end
end

function ENT:Touch(ent)
    if ent == self:GetOwner() or ent:GetSolid() == SOLID_VPHYSICS then return end
    if self.Hitting then return end
    self.Hitting = true

    ent:TakeDamage(GunmanShared.EntityDamage.Beamgun.BallSmall.Touch, IsValid(self:GetOwner()), self)
    self:ProjectileHit(self:GetForward()) 
end

function ENT:Think()
    if SERVER then
        local tr = util.TraceHull({
            start  = self.LastPos,
            endpos = self:GetPos(),
            mins   = Vector(-12, -12, -12),   
            maxs   = Vector( 12,  12,  12),
            mask   = MASK_SHOT,         
            filter = function(ent)
                return ent ~= self and ent ~= self:GetOwner()
            end
        })

        if tr.Hit and IsValid(tr.Entity) then
            self:Touch(tr.Entity)
        end

        self.LastPos = self:GetPos()

        util.BlastDamage(self, self.Owner, self:GetPos(), GunmanShared.EntityDamage.Beamgun.BallSmall.Radius, GunmanShared.EntityDamage.Beamgun.BallSmall.Blast)

        self:NextThink(CurTime() + 0.1)
        return true
    end
end

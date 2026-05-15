ENT.Type = "anim"
ENT.Base = "base_anim"
ENT.ThinkCount = 0
ENT.beamStartPos = Vector()
ENT.beamEndPos = Vector()
ENT.NextThinkTime = 1

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
    
    self:EmitSound("gunman_beamgun_residual")

    self.beamStartPos = self:GetPos() + VectorRand(-64, 64)
    self.beamEndPos = self:GetPos() + VectorRand(-64, 64)
end

function ENT:Draw()
    GunmanShared.DoSegmentedBeam(self.beamStartPos, self.beamEndPos, 4, 16, 64, 1, "gunman/sprites/xbeam1",  Color(255, 255, 100, 255))
end

function ENT:Think()
    if CurTime() > self.NextThinkTime then
        self.beamStartPos = self:GetPos() + VectorRand(-math.random(32, 96), math.random(32, 96))
        self.beamEndPos = self:GetPos() + VectorRand(-math.random(32, 96), math.random(32, 96))
        
        local sparkEffect = EffectData()
        sparkEffect:SetMagnitude(1)
        
        sparkEffect:SetOrigin(self.beamStartPos)
        util.Effect("ElectricSpark", sparkEffect)

        sparkEffect:SetOrigin(self.beamEndPos)
        util.Effect("ElectricSpark", sparkEffect)

		self.NextThinkTime = CurTime() + 1
	end

    if SERVER then
        self:NextThink(CurTime() + 0.5)
        self.ThinkCount = self.ThinkCount + 1

        if self.ThinkCount > 10 then
            self:Remove()
        end

        self.lightingDamageSize = (self.beamStartPos - self.beamEndPos):Length()
        self.lightingDamage = GunmanShared.EntityDamage.Beamgun.Chain

        util.BlastDamage(
			self, 
			self.Owner, 
			self:GetPos(), 
			self.lightingDamageSize, 
			self.lightingDamage
		)
    end
    
    return true
end


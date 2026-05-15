ENT.Type = "anim"
ENT.Base = "base_anim"

ENT.NextThinkTime = 1
ENT.ProjectileIndex = 0
ENT.ChainIndex = 0

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self:SetModel( "models/gunman/tubeball.mdl" )
        
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
    end

    timer.Simple(10, function()
        if self:IsValid() then 
            self:ProjectileHit() 
        end
    end)
    
    self:EmitSound("gunman_beamgun_ballfly")

    self.LastPos = self:GetPos()
end

function ENT:Draw()
    for i = 1, 18 do
        local ent = self:GetNW2Entity("Projectile_" .. i)
        if IsValid(ent) then
            local startPos = self:GetPos()
            local endPos = ent:GetPos()

            GunmanShared.DoSegmentedBeam(startPos, endPos, 4, 12, 16, 0, "gunman/sprites/xbeam1", Color(200, 100, 255))
        end
    end

    render.SetMaterial(Material("gunman/sprites/ball_light"))
    render.DrawSprite(self:GetPos(), 64, 64, Color( 155, 255, 155 ))
end

function ENT:PhysicsCollide( data, physobj )
    local target = data.HitEntity

    data.HitNormal = data.HitNormal * -1
    local start = data.HitPos + data.HitNormal
    local endpos = data.HitPos - data.HitNormal

    util.Decal("gunman_beamgun_ball_hit", start, endpos)
    
    self:ProjectileHit(data.HitNormal) 

    util.BlastDamage(self, self.Owner, self:GetPos(), GunmanShared.EntityDamage.Beamgun.Ball.Radius, GunmanShared.EntityDamage.Beamgun.Ball.Blast)
end

function ENT:Projectile(projectile_effect, index)
    if SERVER then
        local ent = ents.Create(projectile_effect)
        if not IsValid(ent) then return end

        ent:SetOwner(self.Owner)
        ent:SetPos(self:GetPos() - self:GetForward() * 64)
        ent:SetAngles(AngleRand(-180, 180))
        ent:Spawn()
        ent:Activate()

        local phys = ent:GetPhysicsObject()
        if IsValid(phys) then
            phys:ApplyForceCenter(-ent:GetForward() * 300)
        end

        self:SetNW2Entity("Projectile_" .. index, ent)
        self:DeleteOnRemove(ent)
    end
end

function ENT:ProjectileHit(pHitNormal) 
    if SERVER then
        local hitNormal = pHitNormal or Vector(0,0,1)

		local effectdata = EffectData()
		effectdata:SetOrigin(self:GetPos())
		effectdata:SetMagnitude(10)

		util.Effect("cball_explode", effectdata)

        self:EmitSound("gunman_beamgun_balldie")

        self:Remove()
        SafeRemoveEntity(self)
    end
end

function ENT:Touch(ent)
    if ent == self:GetOwner() or ent:GetSolid() == SOLID_VPHYSICS then return end
    if self.Hitting then return end
    self.Hitting = true

    self:ProjectileHit(self:GetForward()) 

    ent:TakeDamage(GunmanShared.EntityDamage.Beamgun.Ball.Touch, self.Owner, self)
end

function ENT:Think()
    if SERVER then
        local size = 24
        local tr = util.TraceHull({
            start  = self.LastPos,
            endpos = self:GetPos(),
            mins   = Vector(-size, -size, -size),   
            maxs   = Vector( size,  size,  size),
            mask   = MASK_SHOT,         
            filter = function(ent)
                return ent ~= self and ent ~= self:GetOwner()
            end
        })

        if tr.Hit and IsValid(tr.Entity) then
            self:Touch(tr.Entity)
        end

        self.LastPos = self:GetPos()

        util.BlastDamage(self, self.Owner, self:GetPos(), GunmanShared.EntityDamage.Beamgun.Ball.ThinkRadius, GunmanShared.EntityDamage.Beamgun.Ball.ThinkBlast)

        if CurTime() > self.NextThinkTime then
            if self.ProjectileIndex < 18 then
                self.ProjectileIndex = self.ProjectileIndex + 1
                self:Projectile("gunman_weapon_beamgun_ball_small", self.ProjectileIndex)
            end

            if self.ChainIndex < 3 then
                self.ChainIndex = self.ChainIndex + 1

                if SERVER then
                    local ent = ents.Create("gunman_weapon_beamgun_chain") 
                    ent:SetPos(self:GetPos() + self:GetForward() * 192)  
                    ent:SetOwner(self:GetOwner())  

                    ent:Spawn()  
                    ent:Activate() 
                end
            end
            
            self.NextThinkTime = CurTime() + 0.25
        end
    
        self:NextThink(CurTime() + 0.05)
        return true
    end
end

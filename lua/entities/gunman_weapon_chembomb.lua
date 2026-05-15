ENT.Type = "anim"
ENT.Base = "base_anim"
ENT.physObject = nil
ENT.thinkTime = 0.1

ENT.colors = {
    Color(0, 145, 0),
    Color(170, 190, 0),
    Color(160, 90, 0),
    Color(130, 15, 0),
    Color(0, 0, 0),
}
ENT.decals = {
    "gunman_cg_green",
    "gunman_cg_lime",
    "gunman_cg_brown",
    "gunman_cg_red",
}

ENT.blastRadius = 0
ENT.collided = false
ENT.sleep = false

ENT.hitDecal = "gunman_cg_green"

ENT.explode = false
ENT.bounce = true
ENT.damageArea = 32
ENT.ammoTake = 1
ENT.color = Color(0, 145, 0)
ENT.stick = true
ENT.smokeBurn = false
ENT.airExpireTime = 1
ENT.airExpireTime = 1

ENT.startReaction = true

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
        self.physObject = self:GetPhysicsObject()

        if (IsValid(phys)) then
            self.physObject:Wake()
            self.physObject:EnableGravity(true)
            self.physObject:SetDragCoefficient(10)
            self.physObject:SetMass(50)
            self.physObject:SetMaterial("gmod_bouncy") 
        end
    end
    
    if CLIENT then
        self.emitter = ParticleEmitter(self:GetPos())
    end
end

function ENT:Draw()
    self.collided = self:GetNWBool("collided", false)
    self.sleep = self:GetNWBool("sleep", false)
    self.smokeBurn = self:GetNWBool("smokeBurn", false)

    self:DrawModel()

    if CurTime() > self.thinkTime then
        if self.emitter:GetNumActiveParticles() > 100 then return end

        if self.sleep == false then
            local particle = self.emitter:Add("particles/smokey", self:GetPos())
            
            if particle then
                particle:SetVelocity(VectorRand() * 32)  
                particle:SetLifeTime(0)  
                particle:SetDieTime(1) 
                particle:SetStartAlpha(64) 
                particle:SetEndAlpha(4) 
                particle:SetStartSize(8) 
                particle:SetEndSize(24) 
                particle:SetColor(self.colors[self:GetSkin() + 1]:Unpack())  
                particle:SetCollide(true) 
                particle:SetGravity(Vector(0, 0, -100))  
            end
        end

        if self.smokeBurn == true and self.collided == true then 
            local particle2 = self.emitter:Add("particles/smokey", self:GetPos())
            
            if particle2 then
                particle2:SetVelocity(VectorRand() * 12)  
                particle2:SetLifeTime(0)  
                particle2:SetDieTime(1) 
                particle2:SetStartAlpha(64) 
                particle2:SetEndAlpha(4) 
                particle2:SetStartSize(4) 
                particle2:SetEndSize(16) 
                particle2:SetColor(50, 50, 0)  
                particle2:SetCollide(true) 
                particle2:SetGravity(Vector(0, 0, 100))  
            end
        end
        
        self.thinkTime = CurTime() + 0.01
    end
end


function ENT:Explode()
    local ent = GunmanShared.EntityCreate(self, "gunman_explosion_chem", self:GetPos(), nil, nil, function(ent)
        ent.scale = self.damageArea / 128
    end)

    if not self:IsValid() then return end
    self:Remove()
    SafeRemoveEntity(self)
end

function ENT:PhysicsCollide( data, physobj )
    if data.Speed > 100 then
        self:EmitSound("gunman_chemgun_cgbounce")

        local effectdata = EffectData()
        effectdata:SetOrigin(self:GetPos())   
        effectdata:SetAngles(self:GetAngles())   
        effectdata:SetEntity(self)   
        effectdata:SetColor(self:GetSkin())
        util.Effect( "chembomb_splash_effect", effectdata )
        
        self:HitDecal()

        self.collided = true
        self:SetNWBool("collided", self.collided)
    else
        physobj:Sleep()
        self.sleep = true
        self:SetNWBool("sleep", self.sleep)
    end

    if self.stick == true then
        self:HitDecal()
        physobj:SetVelocity(Vector(0, 0, 0))
        physobj:SetAngles(Angle(0, 0, 0))
        physobj:EnableMotion(false)
        self.sleep = true
        self:SetNWBool("sleep", self.sleep)
    end

    if self.bounce == true then
        local LastSpeed = math.max(data.OurOldVelocity:Length(), data.Speed)
        local NewVelocity = physobj:GetVelocity()
        NewVelocity:Normalize()

        LastSpeed = math.max(NewVelocity:Length(), LastSpeed)
        local TargetVelocity = NewVelocity * LastSpeed * 0.9
        physobj:SetVelocity(TargetVelocity)

    elseif self.stick == false then
        timer.Simple(0, function() 
            if not self:IsValid() then return end

            if self.explode == true then
                GunmanShared.EntityCreate(self, "gunman_explosion_chem", self:GetPos(), nil, nil, function(ent)
                    ent.scale = self.damageArea / 128
                end)
            else
                GunmanShared.BlastDamage(
                    self.Owner, 
                    self, 
                    self:GetPos(), 
                    self.damageArea, 
                    self.damageArea * 0.2,
                    bit.bor(DMG_BLAST, DMG_PREVENT_PHYSICS_FORCE)
                )
            end
            
            self:Remove()
            SafeRemoveEntity(self)
        end)
    end
end

function ENT:Touch(ent)
    if ent == self:GetOwner() then return end
    
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

    self.hitEnt = ent

    local class = ent:GetClass()
    local solid = ent:GetSolid()

    if string.StartWith(class, "func_") and (solid == SOLID_VPHYSICS or solid == SOLID_BSP) then return end

    if string.StartWith(class, "prop_vehicle_") then
        if self.explode == true then
            GunmanShared.EntityCreate(self, "gunman_explosion_chem", self:GetPos(), nil, nil, function(ent)
                ent.scale = self.damageArea / 128
            end)
        end
        
        self:Remove()
        SafeRemoveEntity(self)
    end

    if self.stick then 
        self:SetParent(ent)

    elseif self.stick == false then
        if ent:GetSolid() ~= SOLID_BBOX then return end

        local effectdata = EffectData()
        effectdata:SetOrigin(self:GetPos())   
        effectdata:SetAngles(self:GetAngles())   
        effectdata:SetEntity(self)   
        effectdata:SetColor(self:GetSkin())
        util.Effect( "chembomb_splash_effect", effectdata )

        if self.explode == true then
            GunmanShared.EntityCreate(self, "gunman_explosion_chem", self:GetPos(), nil, nil, function(ent)
                ent.scale = self.damageArea / 128
            end)
        else
            GunmanShared.BlastDamage(
                self.Owner, 
                self, 
                self:GetPos(), 
                self.damageArea, 
                self.damageArea * 0.2, 
                bit.bor(DMG_BLAST, DMG_PREVENT_PHYSICS_FORCE)
            )
        end
        
        self:Remove()
        SafeRemoveEntity(self)
    end
end

function ENT:HitDecal()
    local hitPos, hitNormal = GunmanShared.GetSurfaceInfo(self)

    if hitPos ~= nil and hitNormal ~= nil then
        util.Decal(self.decals[self:GetSkin() + 1], hitPos + hitNormal, hitPos - hitNormal)
    end
end

function ENT:ChemReaction()
    if not SERVER then return end 

    timer.Simple(self.airExpireTime, function() 
        if not self:IsValid() then return end
        
        local effectdata = EffectData()
        effectdata:SetOrigin(self:GetPos())   
        effectdata:SetAngles(self:GetAngles())   
        effectdata:SetEntity(self)   
        effectdata:SetColor(self:GetSkin())
        util.Effect( "chembomb_explosion_effect", effectdata )

        if self.explode == true then
            GunmanShared.EntityCreate(self, "gunman_explosion_chem", self:GetPos(), nil, nil, function(ent)
                ent.scale = self.damageArea / 256
            end)
        else
            util.BlastDamage(self, self.Owner, self:GetPos(), 256, 10)
        end

        timer.Simple(0, function() 
            if not self:IsValid() then return end
            self:Remove()
            SafeRemoveEntity(self)
        end)
    end)
end

function ENT:DetachFromParent()
    self:SetParent(nil)                 
    self:SetMoveType(MOVETYPE_VPHYSICS) 
    self:SetCollisionGroup(COLLISION_GROUP_NONE)

    local phys = self:GetPhysicsObject()
    if IsValid(phys) then
        phys:EnableMotion(true)
        phys:Wake()
    end
end

function ENT:Think()
    if (self.sleep or self.hitEnt) and self.smokeBurn then
        if not SERVER then return end
        GunmanShared.BlastDamage(self.Owner, self, self:GetPos(), self.damageArea, self.damageArea * 0.2, bit.bor(DMG_BLAST, DMG_PREVENT_PHYSICS_FORCE))
    end

    if self.startReaction then
        self:ChemReaction()
        self.startReaction = false 
    end

    local parent = self:GetParent()
    if IsValid(parent) then
        local validHealth = parent:GetInternalVariable("m_iHealth") 
        if validHealth and validHealth > 0 then
            if parent:Health() <= 0 then
                self:DetachFromParent()
            end
        end

        if (parent:IsNPC() or parent:IsNextBot()) and parent:Health() <= 0 then
            self:DetachFromParent()
        end
    end

    if not SERVER then return end
    self:SetNWBool("smokeBurn", self.smokeBurn)
end

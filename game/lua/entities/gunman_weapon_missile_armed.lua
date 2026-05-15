ENT.Type = "anim"
ENT.Base = "gunman_weapon_grenade_base"

ENT.doDetonateOnImpact = false
ENT.doDeployTripMine = false
ENT.doCluster = false
ENT.doSpiral = false
ENT.doGuided = false
ENT.doSpiralReverse = false
ENT.homingTarget = nil

ENT.Collided = false
ENT.DetonationTime = 0
ENT.active = false

ENT.physObject = nil
ENT.detonated = false

ENT.xspeed = 0
ENT.spiralTarget = nil
ENT.spiralStartPos = nil
ENT.traceStart = nil

AddCSLuaFile("gunman_shared.lua")
AddCSLuaFile("gunman_weapon_missile_armed.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self:SetModel("models/gunman/dmlrocket.mdl")
    self:SetSolid(SOLID_VPHYSICS)
    self:PhysicsInit(MOVETYPE_VPHYSICS)
    self:SetMoveType(MOVETYPE_VPHYSICS)
    self:SetSolidFlags(FSOLID_TRIGGER)

    if IsValid(self:GetOwner()) then
        self:SetCollisionGroup(COLLISION_GROUP_PROJECTILE)
    end

    if SERVER then
        self.physObject = self:GetPhysicsObject()

        if self.physObject:IsValid() then
            self.physObject:Wake()
            self.physObject:EnableGravity(false)
            self.physObject:SetDragCoefficient(0.1)
            self.physObject:SetMass(10)
            self.physObject:AddGameFlag(FVPHYSICS_NO_NPC_IMPACT_DMG)
        end

        self.SpriteTrailSmoke = util.SpriteTrail(self, 0, Color(100, 100, 100, 150), true, 16, 0, 4, 0.01, "sprites/smoke.vmt")
        self.SpriteTrailFlame = util.SpriteTrail(self, 0, Color(255, 150, 0, 255), true, 8, 0, 2, 0.01, "sprites/xbeam2.vmt")

        self:EmitSound("gunman_muleRocketFly")
    end

    self.spiralStartPos = self:GetOwner():EyePos()
    self.spiralTarget = self:GetOwner():EyePos() + self:GetOwner():GetAimVector() * 600000
    
    local trace = util.TraceLine({
        start = self.spiralStartPos,
        endpos = self.spiralTarget,
        filter = function(ent)
            return ent ~= self and ent ~= owner
        end
    })

    self.traceStart = trace.HitPos
end

function ENT:Draw()
    self:DrawModel()

    if self:GetNWBool("Collided", false) then return end

    local pos = self:GetPos() - self:GetForward() * 16

    render.SetMaterial(Material("sprites/glow04_noz"))
    render.DrawSprite(pos, 128, 128, Color(255, 200, 0))
    render.DrawSprite(pos, 64, 64, Color(255, 255, 255))
end

function ENT:DeployTripMine()
	if not SERVER then return end

    local pos = self:GetPos()
    local aim = self:GetForward()

    local traceEndPos = pos + aim * 256

    local trace = util.TraceLine({
        start = pos,  
        endpos = traceEndPos,  
        filter = self
    })

    if trace.Hit then
		local offset = (trace.HitPos - trace.StartPos):GetNormalized() * 8 
		local spawnPos = trace.HitPos - offset 
		local angle = trace.HitNormal:Angle()
		
		local ent = GunmanShared.EntityCreate(self, "gunman_weapon_grenade_tripmine", spawnPos, angle)

		if self.doCluster then
			ent.doCluster = true
		end

        ent = GunmanShared.EntityCreate(self, "gunman_physics_object", spawnPos - self:GetForward() * 8, angle, "models/gunman/shellengine.mdl")
        if IsValid(ent) then
           ent:GetPhysicsObject():ApplyForceCenter(-self:GetForward() * 100 + self:GetRight() * 20) 
        end

        ent = GunmanShared.EntityCreate(self, "gunman_physics_object", spawnPos - self:GetForward() * 8, angle, "models/gunman/shelltip.mdl")
        if IsValid(ent) then
            ent:GetPhysicsObject():ApplyForceCenter(-self:GetForward() * 100 - self:GetRight() * 20) 
        end 
    end

    self:Remove()
    SafeRemoveEntity(self)
end

function ENT:PhysicsCollide(data, physobj)
    if self.doDetonateOnImpact and data.Speed > 50 then
        timer.Simple(0.05, function()
            if self:IsValid() then
                self:Detonate()
            end
        end)
        return
    end

    if not self.doDetonateOnImpact then
        if data.Speed > 50 then
            
            self:EmitSound("gunman_GrenadeHit")
        else
            physobj:Sleep()
        end

        self.physObject:EnableGravity(true)
    end

    if self.SpriteTrailFlame:IsValid() and self.SpriteTrailSmoke:IsValid() then
        self.SpriteTrailFlame:Remove()
        self.SpriteTrailSmoke:Remove()
        util.SpriteTrail(self, 0, Color(100, 100, 100, 150), true, 16, 0, 4, 0.01, "sprites/smoke.vmt")
    end 

    self.Collided = true
    self:SetNWBool("Collided", self.Collided)

    if self.doDeployTripMine == true then
        timer.Simple(0.05, function()
            if self:IsValid() then
                self:DeployTripMine()
            end
        end)
    end
end

function ENT:Steer(targetPos, missilePos, turnLerp, maxAimDistance)
    local direction = (targetPos - missilePos):GetNormalized()
    local missileAngles = direction:Angle()

    local distance = self:GetPos() - self:GetOwner():GetPos()

    if distance:Length() < maxAimDistance then
        self:SetAngles(LerpAngle(turnLerp, self:GetAngles(), missileAngles))
    end
end

function ENT:Think()
    if self.doSpiral == true and self.Collided == false then
        local traceSpiral = util.TraceLine({
            start = self:GetPos(),
            endpos = self.spiralTarget,
            filter = function(ent)
                return ent ~= self and ent ~= owner
            end
        })

        local currentDistance = (self:GetPos() - traceSpiral.HitPos):Length()
        local distanceAdjust = 4096 
        local fraction = math.Clamp(currentDistance / distanceAdjust, 0, 1)

        local upSin = self:GetUp() * 1000 * (math.sin( CurTime() * 8)) * fraction 
        local rightCos = self:GetRight() * 1000 * (math.cos( CurTime() * 8)) * fraction 

        if self.doSpiralReverse == true then
            rightCos = rightCos
        else
            rightCos = -rightCos
        end

        local missilePos = self:GetPos() + upSin + rightCos

        local owner = self:GetOwner() 
        local maxAimDistance = 8192 * 2

        local trace = util.TraceLine({
            start = self.spiralStartPos,
            endpos = self.spiralTarget,
            filter = function(ent)
                return ent ~= self and ent ~= owner
            end
        })

        local targetPos = trace.HitPos
        local turnLerp = 0.1

        if self.homingTarget ~= nil then
            targetPos = self.homingTarget:GetPos()
            turnLerp = 0.5
        end

        self:Steer(targetPos, missilePos, turnLerp, maxAimDistance)
    end

    if self.doGuided == true and self.Collided == false then
        local missilePos = self:GetPos() 

        local owner = self:GetOwner()
        local maxAimDistance = 4096
        local endPos = owner:EyePos() + owner:GetAimVector() * maxAimDistance

        local trace = util.TraceLine({
            start = owner:GetShootPos(),
            endpos = endPos,
            filter = function(ent)
                return ent ~= self and ent ~= owner
            end
        })

        local targetPos = trace.HitPos
        local turnLerp = 0.1

        self:Steer(targetPos, missilePos, turnLerp, maxAimDistance)
    end

    if self.homingTarget ~= nil and self.Collided == false then
        if not IsValid(self.homingTarget) then return end
        
        local missilePos = self:GetPos() 
        local maxAimDistance = 4096
        local targetPos = self.homingTarget:GetPos()
        local turnLerp = 0.5

        self:Steer(targetPos, missilePos, turnLerp, maxAimDistance)
    end

    if self.Collided == false then
        if not SERVER then return end
        self.physObject:ApplyForceCenter(self:GetForward() * 10000)
        self:SetAngles(self:GetAngles())
    end

    if self.doDetonateOnImpact == false and self.doDeployTripMine == false then
        self.DetonationTime = self.DetonationTime + 0.1

        if self.DetonationTime > 20 then
            self:Detonate()
        end
    end

    local filterList = {
        "npc_",
    }

	local sphere = ents.FindInSphere(self:GetPos(), 32)

	if #sphere > 0 then
		for _, ent in pairs(sphere) do
            
            local classValid = false
            for _, prefix in ipairs(filterList) do
                if string.StartWith(ent:GetClass(), prefix) then
                    classValid = true
                    break
                end
            end
            
            if classValid and ent ~= self:GetOwner() then
                print("gunman_weapon_missle", ent)
                self:Detonate()
            end
		end
	end	

    self:NextThink(CurTime())
    return true
end

ENT.Type = "anim"
ENT.Base = "base_anim"

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self.startTime = CurTime()  
    self.frame = 0
    self.maxFrames = 7
    self.fps = 2

    if CLIENT then
        self.emitter = ParticleEmitter(self:GetPos())
    end

    self:Explode()
end

function ENT:Draw()
    local Mat = Material("gunman/sprites/part1")
    render.SetMaterial(Material("gunman/sprites/part1"))

    Mat:SetInt("$frame", self.frame) 
    render.DrawSprite(self:GetPos(), 320, 320, Color(255, 255, 255))

    if self.emitter:GetNumActiveParticles() > 100 then return end

    local particle = self.emitter:Add("particles/smokey", self:GetPos())

    if particle then
        particle:SetVelocity(VectorRand() * 100)  
        particle:SetLifeTime(0)  
        particle:SetDieTime(1) 
        particle:SetStartAlpha(20) 
        particle:SetEndAlpha(5) 
        particle:SetStartSize(10) 
        particle:SetEndSize(80) 
        particle:SetColor(100, 100, 100)  
        particle:SetCollide(true) 
        particle:SetGravity(Vector(0, 0, 100))   
    end
end

function ENT:Explode(data)
    if not self.Owner:IsValid() then return end

    local effectdata = EffectData()

    timer.Simple(0.1, function() 
        for i = 1, 8 do 
            local randVec = VectorRand(-4 * i, 4 * i)
            randVec.z = 8 * i
            effectdata:SetOrigin(self:GetPos() + randVec)
            effectdata:SetMagnitude(1.5)
            effectdata:SetScale(1)
            util.Effect("ElectricSpark", effectdata)
        end
    end)

    effects.BeamRingPoint(self:GetPos(), 0.1, 16, 320, 128, 0, Color(255, 50, 0, 10),
    {
        speed = 0,
        spread = 0,
        delay = 0,
        framerate = 2,
        material = "sprites/lgtning.vmt"
    })

    if SERVER then
        util.ScreenShake(self:GetPos(), 100, 50, 1, 1024)

        util.BlastDamage(self, self.Owner, self:GetPos(), GunmanShared.EntityDamage.Explosions.Standard.Radius, GunmanShared.EntityDamage.Explosions.Standard.Damage)
    end

    self:EmitSound("gunman_kabam")

    local hitPos, hitNormal = GunmanShared.GetSurfaceInfo(self)

    if hitPos ~= nil and hitNormal ~= nil then
        util.Decal("gunman_scorch1", hitPos + hitNormal, hitPos - hitNormal)
    end
end

function ENT:Remove()
    self.emitter:Finish()
end

function ENT:Think()
    self.frame = (CurTime() - self.startTime) * self.maxFrames * self.fps

    if self.frame > self.maxFrames then
        if SERVER then
            self:Remove()
            SafeRemoveEntity(self)
        end
    end
end
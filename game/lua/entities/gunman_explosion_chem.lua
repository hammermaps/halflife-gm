ENT.Type = "anim"
ENT.Base = "base_anim"

ENT.scale = 1

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    if SERVER then
        self:SetNW2Float("ExplosionScale", self.scale or 1)
    end

    self.startTime = CurTime()  
    self.decayTime = CurTime() + 1
    self.frame = 0
    self.maxFrames = 10
    self.fps = 2
    self.alphaDecay = 255

    if CLIENT then
        self.emitter = ParticleEmitter(self:GetPos())
    end

    self:Explode()
end

function ENT:Draw()
    Mat = Material("gunman/sprites/explosion3")
    render.SetMaterial(Mat)

    local scale = self:GetNW2Float("ExplosionScale", 1)
    Mat:SetInt("$frame", self.frame) 
    render.DrawSprite(self:GetPos(), 192 * scale, 192 * scale,  Color(255, 255, 255))

    if self.emitter:GetNumActiveParticles() > 100 then return end

    local particle = self.emitter:Add("particles/smokey", self:GetPos())

    if particle then
        particle:SetVelocity(VectorRand() * 100)  
        particle:SetLifeTime(0)  
        particle:SetDieTime(0.5) 
        particle:SetStartAlpha(10) 
        particle:SetEndAlpha(1) 
        particle:SetStartSize(10) 
        particle:SetEndSize(50) 
        particle:SetColor(100, 100, 100)  
        particle:SetCollide(true) 
        particle:SetGravity(Vector(0, 0, 50))    
    end
end

function ENT:Explode()
    local effectdata = EffectData()

    local scale = self:GetNW2Float("ExplosionScale", 1)

    for i = 1, 8 do 
        local randVec = VectorRand(-4 * i, 4 * i) 
        randVec.z = 8 * i 
        effectdata:SetOrigin(self:GetPos() + (randVec * scale))
        effectdata:SetMagnitude(1)
        effectdata:SetScale(1)
        util.Effect("ElectricSpark", effectdata)
    end

    if SERVER then
        util.BlastDamage(self, self.Owner, self:GetPos(), GunmanShared.EntityDamage.Explosions.Chem.Radius * scale, GunmanShared.EntityDamage.Explosions.Chem.Damage)

        util.ScreenShake(self:GetPos(), 50, 50, 1, 1024)
	end

    local hitPos, hitNormal = GunmanShared.GetSurfaceInfo(self)

    if hitPos ~= nil and hitNormal ~= nil then
        util.Decal("gunman_scorch2", hitPos + hitNormal, hitPos - hitNormal)
    end

    self:EmitSound("gunman_explode")
end

function ENT:Think()
    self.alphaDecay = (self.decayTime - CurTime()) * 255

    self.frame = (CurTime() - self.startTime) * self.maxFrames * self.fps

    if self.frame > self.maxFrames then
        if SERVER then
            self:Remove()
            SafeRemoveEntity(self)
        end
    end
end
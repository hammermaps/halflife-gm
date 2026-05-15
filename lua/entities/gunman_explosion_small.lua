ENT.Type = "anim"
ENT.Base = "base_anim"

ENT.scale = 1
ENT.frame = 0

AddCSLuaFile()
AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

function ENT:Initialize()
    self.startTime = CurTime()  
    self.decayTime = CurTime() + 1
    self.frame = 0
    self.maxFrames = 12
    self.fps = 2
    self.alphaDecay = 255

    self:Explode()
end

function ENT:Draw()
    local Mat = Material("gunman/sprites/kabam")
    render.SetMaterial(Mat)

    Mat:SetInt("$frame", self.frame) 
    render.DrawSprite(self:GetPos(), 192, 192, Color(255, 255, 255))

    render.SetMaterial(Material("gunman/sprites/smokering"))
    render.DrawSprite(self:GetPos(), self.scale, self.scale, Color(255, 255, 255, self.alphaDecay))
end

function ENT:Explode()
    local effectdata = EffectData()

    for i = 1, 8 do 
        local randVec = VectorRand(-4 * i, 4 * i)
        randVec.z = 8 * i
        effectdata:SetOrigin(self:GetPos() + randVec)
        effectdata:SetMagnitude(1)
        effectdata:SetScale(1)
        util.Effect("ElectricSpark", effectdata)
    end

    if SERVER then
        util.BlastDamage(self, self.Owner, self:GetPos(), GunmanShared.EntityDamage.Explosions.Small.Radius, GunmanShared.EntityDamage.Explosions.Small.Damage)

        util.ScreenShake(self:GetPos(), 50, 50, 1, 1024)
	end

	effects.BeamRingPoint(self:GetPos(), 1, 12, 256, 64, 0, Color(255, 150, 0, 50),
    {
		speed = 0,
		spread = 0,
		delay = 0,
		framerate = 2,
		material = "sprites/lgtning.vmt"
	})

    local hitPos, hitNormal = GunmanShared.GetSurfaceInfo(self)

    if hitPos ~= nil and hitNormal ~= nil then
        util.Decal("gunman_scorch2", hitPos + hitNormal, hitPos - hitNormal)
    end

    self:EmitSound("gunman_kaboom")
end

function ENT:Think()
    self.scale = (CurTime() - self.startTime) * 256
    self.alphaDecay = (self.decayTime - CurTime()) * 255

    self.frame = (CurTime() - self.startTime) * self.maxFrames * self.fps

    if self.frame > self.maxFrames then
        if SERVER then
            self:Remove()
            SafeRemoveEntity(self)
        end
    end
end
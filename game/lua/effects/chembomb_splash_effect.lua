
EFFECT.thinkTime = 0
EFFECT.colors = {
    Color(0, 145, 0),
    Color(170, 190, 0),
    Color(160, 90, 0),
    Color(130, 15, 0),
}

EFFECT.colorIndex = 1

function EFFECT:Init(data)
    self.AliveTime = 0.5
    self.Ang = data:GetAngles()
    self.Pos = data:GetOrigin()
    self.Ent = data:GetEntity()
    self.emitter = ParticleEmitter(self.Pos)

    self.colorIndex = data:GetColor() + 1

    if data:GetEntity():IsValid() then
        self.Ent = data:GetEntity()
        self.PosForward = self.Ent:GetForward()
    end

end

function EFFECT:Render()
        if self.emitter:GetNumActiveParticles() > 100 then return end

        local particle = self.emitter:Add("particles/smokey", self:GetPos())
        local particle2 = self.emitter:Add("particles/smokey", self:GetPos())

        if particle then
            particle:SetVelocity(VectorRand() * 128)  
            particle:SetLifeTime(0)  
            particle:SetDieTime(2) 
            particle:SetStartAlpha(200) 
            particle:SetEndAlpha(4) 
            particle:SetStartSize(1) 
            particle:SetEndSize(4) 

            particle:SetColor(
                self.colors[self.colorIndex].r, 
                self.colors[self.colorIndex].g, 
                self.colors[self.colorIndex].b
            )  

            particle:SetCollide(true) 
            particle:SetGravity( Vector(0, 0, -500))  
        end

        if particle2 then
            particle2:SetVelocity(VectorRand() * 128)  
            particle2:SetLifeTime(0)  
            particle2:SetDieTime(2) 
            particle2:SetStartAlpha(200) 
            particle2:SetEndAlpha(4) 
            particle2:SetStartSize(1) 
            particle2:SetEndSize(4) 

            particle2:SetColor(
                self.colors[self.colorIndex].r * 0.7,
                self.colors[self.colorIndex].g * 0.7, 
                self.colors[self.colorIndex].b * 0.7
            )  

            particle2:SetCollide(true) 
            particle2:SetGravity(Vector(0, 0, -500))  
        end
end

function EFFECT:Think()
    self.AliveTime = self.AliveTime - FrameTime()
    
    if self.AliveTime <= 0 then
        return false
    end

    return true
end
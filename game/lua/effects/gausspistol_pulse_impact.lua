function EFFECT:Init( data )
	self.position = data:GetOrigin()
	self.normal = data:GetNormal()
    self.startTime = CurTime()       
    self.lifeTime = 0.2

    self.scale = 0
end

function EFFECT:Render()
    render.SetMaterial(Material("gunman/sprites/gaussshock"))
    render.DrawQuadEasy(self.position, self.normal, self.scale, self.scale, Color(255, 255, 255, 150), 15)

    local scales = { 0.2, 0.3, 0.4 }
    
    for i, mul in ipairs(scales) do
        local s = math.min(self.scale * mul, 8)
        local pos = self.position + self.normal * s
    
        render.DrawQuadEasy(pos, self.normal, s, s, Color(255, 255, 255, (i == 1) and 150 or 255), 15)
    end
end

function EFFECT:Think()
    self.scale = (CurTime() - self.startTime) * 72

    return (CurTime() - self.startTime) < self.lifeTime
end
function EFFECT:Init( data )
	self.position = data:GetOrigin();
	self.normal = data:GetNormal();
	self.startTime = CurTime()  
	self.decayTime = CurTime() + 0.8
	
	self.lifeTime = 1
	self.scale = 0
	self.alphaDecay = 255
    self.maxFrames = 8
    self.fps = 10
    self.frame = 0
end

function EFFECT:Render()
	render.SetMaterial(Material("gunman/sprites/gaussshock"))
    render.DrawQuadEasy(self.position, self.normal, self.scale, self.scale, Color(255, 255, 255, self.alphaDecay), 15)

	Mat = Material("gunman/sprites/gausspuff")
    render.SetMaterial(Mat)

    Mat:SetInt("$frame", math.Round(self.frame)) 

	render.DrawSprite(self:GetPos(), 16, 16, Color(255, 255, 255, self.alphaDecay))
end

function EFFECT:Think()
    self.frame = (CurTime() - self.startTime) * self.fps
	self.alphaDecay = (self.decayTime - CurTime()) * 255
	self.scale = (CurTime() - self.startTime) * 12

    return self.frame < self.maxFrames 
end
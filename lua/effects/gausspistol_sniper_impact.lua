 function EFFECT:Init( data )
	self.position = data:GetOrigin()
	self.normal = data:GetNormal()
    self.magnitude = data:GetMagnitude() 

    self.startTime = CurTime()  
    self.alphaDecay = 0
    self.decayTime = CurTime() + 1
    self.maxFrames = 8
    self.fps = 10
    self.frame = 0
end

function EFFECT:Render()
    Mat = Material("gunman/sprites/gausspoof")
    render.SetMaterial(Mat)

    Mat:SetInt("$frame", math.Round(self.frame)) 
    render.DrawQuadEasy(self.position, self.normal, 32 * self.magnitude, 32 * self.magnitude, Color(255, 255, 255, self.alphaDecay), 15)
end

function EFFECT:Think()
    self.frame = (CurTime() - self.startTime) * self.fps
	self.alphaDecay = (self.decayTime - CurTime()) * 255

    return self.frame < self.maxFrames 
end
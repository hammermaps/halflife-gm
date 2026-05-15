include('shared.lua')
	
SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_aicore")  

function SWEP:DrawWorldModel()
	self:DrawModel()

	if IsValid(self) then
		local lightColor = render.GetLightColor(self:GetPos()) 
		self:SetColor(Color( 
			(lightColor.x ^ 0.25) * 255, 
			(lightColor.y ^ 0.25) * 255, 
			(lightColor.z ^ 0.25) * 255 
		))
	end
end

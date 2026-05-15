AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
include('shared.lua')

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_knife")  

function SWEP:DrawWorldModel()
	self.enableKnife = self:GetNW2Bool("enableKnife", true)
	if self.enableKnife then
		self.Weapon:DrawModel()
	end
end



include('shared.lua')
local GunmanShared = include("gunman_shared.lua")

SWEP.MenuLabels = {
	Payload = "PAYLOAD",
	Detonation = "DETONATION",
}

SWEP.Payload = {
	"EXPLOSIVE",
	"CLUSTER",
}

SWEP.Detonation = {
	"ON IMPACT",
	"TIMED",
	"WHEN TRIPPED",
}

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_grenade")  

function SWEP:CustomDrawHUD()	
	local menuLabels = {
		{ label = self.MenuLabels.Detonation, value = self.Detonation[self.CurrentDetonationType], offset = 0 },
		{ label = self.MenuLabels.Payload, value = self.Payload[self.CurrentPayloadType], offset = 60 },
	}

	self:DrawMenuHUD(menuLabels)
end

function SWEP:DrawWorldModel()
	if self:Clip1() == 0 then return end
	self.Weapon:DrawModel()
end
	

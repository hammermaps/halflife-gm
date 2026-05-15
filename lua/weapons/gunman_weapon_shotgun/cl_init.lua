local GunmanShared = include("gunman_shared.lua")
include('shared.lua')

SWEP.CrosshairMaterial = "gunman/sprites/shotgun_crosshair"

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_shotgun")  

SWEP.MenuLabels = {
	Shells = "SHELLS",
	Spread = "SPREAD",
}

SWEP.MenuSpreadTypes = {
	"RIFLE",
	"SHOTGUN",
	"RIOTGUN",
}

function SWEP:DrawHUD()
	self.CurrentShellCost = self:GetNW2Int("CurrentShellCost", 2)
	self.CurrentWeaponSpread = self:GetNW2Int("CurrentWeaponSpread", 2)
	self.FireModeMenuOption = self:GetNW2Int("FireModeMenuOption", 1)
	self.FireModeMenuToggle = self:GetNW2Bool("FireModeMenuToggle", false)
	
	local menuLabels = {
		{ label = self.MenuLabels.Shells, value = self.CurrentShellCost, offset = 0 },
		{ label = self.MenuLabels.Spread, value = self.MenuSpreadTypes[self.CurrentWeaponSpread], offset = 60 },
	}

	self:DrawMenuHUD(menuLabels)
end



local GunmanShared = include("gunman_shared.lua")
include('shared.lua')

SWEP.CrosshairMaterial = nil

function SWEP:DrawWeaponSelection(x, y, wide, tall, alpha)
    self.LastSelectionHUDTime = CurTime()

    if self.WepSelectIcon then
        surface.SetDrawColor(255, 255, 255, alpha)
        surface.SetTexture(self.WepSelectIcon)
        surface.DrawTexturedRect(x, y, wide, tall * 0.75)
    else
        draw.SimpleText(
            self.PrintName or "?", 
            "Default", 
            x + wide / 2, y + tall / 2, 
            Color(255, 255, 255, alpha), 
            TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER
        )
    end
end

function SWEP:IsSelectionHUDActive()
    return (self.LastSelectionHUDTime or 0) > CurTime() - 0.1
end

function SWEP:DoDrawCrosshair(x, y)
	if not self.CrosshairMaterial then return true end

	local textureSize = 96
	local crosshairWidth = 48
	local crosshairHeight = 48

	surface.SetDrawColor(255, 255, 255, 255)
	surface.SetMaterial(Material(self.CrosshairMaterial))
	surface.DrawTexturedRect(x - crosshairWidth, y - crosshairHeight, textureSize, textureSize) 

	return true
end

function SWEP:ViewModelDrawn()
	GunmanShared.ViewModelColorCorrection(self)
    self:CustomViewModelDrawn()
end

function SWEP:CustomViewModelDrawn()
end

function SWEP:CustomAmmoDisplay()
	self.AmmoDisplay = self.AmmoDisplay or {} 
	self.AmmoDisplay.Draw = (self.Primary.Ammo ~= "none") 

	if self.Primary.ClipSize > 0 then
		self.AmmoDisplay.PrimaryClip = math.max(self:Clip1(), 0)
		if self.Primary.MaxAmmo ~= -1 then
			self.AmmoDisplay.PrimaryAmmo = self:Ammo1()
		end
	else
		self.AmmoDisplay.PrimaryClip = -1
		self.AmmoDisplay.PrimaryAmmo = self:Ammo1()
	end

	return self.AmmoDisplay
end

function SWEP:HudBackground(xSize, ySize)
	local targetRes = ScrW() / 1920
	local resWidthScale = 6 / (targetRes ^ 1.333)
	local rectH = (ScrH() / resWidthScale) 
	local firstLabelOffset = 85
	local heightAdjust = 20
	surface.SetDrawColor(10, 10, 10, 100)
	surface.DrawRect(0, rectH - heightAdjust, xSize, ySize + firstLabelOffset)
end

function SWEP:TextLabel(string, var, offset, selected)
	local targetRes = ScrW() / 1920
	local resWidthScale = 6 / (targetRes ^ 1.333)
	local resHeightScale = 2.2 / (targetRes ^ 0.17)
	local textLabelPos_W = 440
	local textVarOffset = 384

	local textLabel_W = (ScrW() - textLabelPos_W) / resHeightScale
	local textLabel_H = (ScrH() / resWidthScale) + offset
	local textVar_W = textLabel_W + textVarOffset

	local font = selected and "Gunman_HUDFont_Selected" or "Gunman_HUDFont_Unselected"
	local color = selected and Color(0, 255, 0) or Color(210, 255, 210)
	
	-- Add colon if it doesn't have one
	local labelText = string
	if not labelText:EndsWith(":") then
		labelText = labelText .. ":"
	end

	draw.SimpleText(labelText, font, textLabel_W, textLabel_H, color, TEXT_ALIGN_LEFT, TEXT_ALIGN_TOP)
	draw.SimpleText(var, font, textVar_W, textLabel_H, color, TEXT_ALIGN_LEFT, TEXT_ALIGN_TOP)
end


function SWEP:DrawMenuHUD(menuLabels)
	if not self:IsMenuOpen() then return end

	self:HudBackground(ScrW(), menuLabels[#menuLabels].offset)

	for i, menuLabel in ipairs(menuLabels) do
		-- If optionIndex is provided (e.g. Beamgun), use it for highlighting. 
		-- Otherwise, use the loop index i.
		local highlightIndex = menuLabel.optionIndex or i
		local isHighlighted = (self.FireModeMenuOption == highlightIndex)
		
		self:TextLabel(menuLabel.label, menuLabel.value, menuLabel.offset, isHighlighted)
	end
end

function SWEP:CustomDrawHUD()
end

function SWEP:DrawHUD()
	self:CustomDrawHUD()
end
